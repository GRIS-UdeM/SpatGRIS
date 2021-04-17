/*
 This file is part of SpatGRIS.

 Developers: Samuel B�land, Olivier B�langer, Nicolas Masson

 SpatGRIS is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 SpatGRIS is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with SpatGRIS.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "AudioManager.h"

#include "AudioProcessor.h"
#include "constants.hpp"

//==============================================================================
std::unique_ptr<AudioManager> AudioManager::mInstance{ nullptr };

//==============================================================================
AudioManager::AudioManager(juce::String const & deviceType,
                           juce::String const & inputDevice,
                           juce::String const & outputDevice,

                           double const sampleRate,
                           int const bufferSize)
{
    JUCE_ASSERT_MESSAGE_THREAD;

    auto const success{ tryInitAudioDevice(deviceType, inputDevice, outputDevice, sampleRate, bufferSize) };

    if (!success) {
        mAudioDeviceManager.initialiseWithDefaultDevices(MAX_INPUTS, MAX_OUTPUTS);
    }

    // Register physical ports
    auto * audioDevice{ mAudioDeviceManager.getCurrentAudioDevice() };
    jassert(audioDevice != nullptr);
    mRecordersThread.setPriority(9);
    mAudioDeviceManager.addAudioCallback(this);
}

//==============================================================================
void AudioManager::audioDeviceIOCallback(float const ** inputChannelData,
                                         int const totalNumInputChannels,
                                         float ** const outputChannelData,
                                         int const totalNumOutputChannels,
                                         int const numSamples)
{
    jassert(numSamples <= mInputBuffer.MAX_NUM_SAMPLES);
    jassert(numSamples <= mOutputBuffer.MAX_NUM_SAMPLES);

    if (!mAudioProcessor) {
        return;
    }

    juce::ScopedTryLock const lock{ mAudioProcessor->getLock() };
    if (!lock.isLocked()) {
        return;
    }

    // clear buffers
    mInputBuffer.silence();
    mOutputBuffer.silence();

    // copy input data to buffers
    auto const numInputChannelsToCopy{ std::min(totalNumInputChannels, mInputBuffer.size()) };
    for (int i{}; i < numInputChannelsToCopy; ++i) {
        source_index_t const sourceIndex{ i + 1 };
        auto const * sourceData{ inputChannelData[i] };
        auto * destinationData{ mInputBuffer[sourceIndex].getWritePointer(0) };
        std::copy_n(sourceData, numSamples, destinationData);
    }

    // do the actual processing
    if (mAudioProcessor != nullptr) {
        mAudioProcessor->processAudio(mInputBuffer, mOutputBuffer);
    }

    // copy buffers to output
    mOutputBuffer.copyToPhysicalOutput(outputChannelData, totalNumOutputChannels);

    // Record
    if (mIsRecording) {
        auto const stopRecordingAndDisplayError = [this]() {
            stopRecording();
            juce::AlertWindow::showMessageBox(
                juce::AlertWindow::AlertIconType::WarningIcon,
                "Error",
                "Recording stopped because samples were dropped.\nRecording on a faster disk might solve this issue.");
        };

        for (auto const & recorder : mRecorders) {
            jassert(recorder->audioFormatWriter->getNumChannels() == recorder->dataToRecord.size());
            auto const success{ recorder->threadedWriter->write(recorder->dataToRecord.data(), numSamples) };
            jassert(success);
            if (!success) {
                stopRecordingAndDisplayError();
            }
        }
        mNumSamplesRecorded += numSamples;
    }
}

//==============================================================================
bool AudioManager::tryInitAudioDevice(juce::String const & deviceType,
                                      juce::String const & inputDevice,
                                      juce::String const & outputDevice,
                                      double const requestedSampleRate,
                                      int const requestedBufferSize)
{
    JUCE_ASSERT_MESSAGE_THREAD;

    if (deviceType.isEmpty() || inputDevice.isEmpty() || outputDevice.isEmpty()) {
        return false;
    }

    auto const & availableDeviceTypes{ mAudioDeviceManager.getAvailableDeviceTypes() };

    auto const * const foundDeviceType{ std::find_if(
        std::cbegin(availableDeviceTypes),
        std::cend(availableDeviceTypes),
        [&deviceType](juce::AudioIODeviceType const * audioDeviceType) -> bool {
            return audioDeviceType->getTypeName() == deviceType;
        }) };
    if (foundDeviceType == std::cend(availableDeviceTypes)) {
        return false;
    }

    mAudioDeviceManager.setCurrentAudioDeviceType(deviceType, true);
    auto * deviceTypeObject{ mAudioDeviceManager.getCurrentDeviceTypeObject() };
    jassert(deviceTypeObject);
    deviceTypeObject->scanForDevices();

    juce::BigInteger neededInputChannels{};
    neededInputChannels.setRange(0, MAX_INPUTS, true);
    juce::BigInteger neededOutputChannels{};
    neededOutputChannels.setRange(0, MAX_OUTPUTS, true);
    juce::AudioDeviceManager::AudioDeviceSetup const setup{ outputDevice,         inputDevice,
                                                            requestedSampleRate,  requestedBufferSize,
                                                            neededInputChannels,  false,
                                                            neededOutputChannels, false };
    auto const errorString{ mAudioDeviceManager.setAudioDeviceSetup(setup, true) };
    if (errorString.isNotEmpty()) {
        return false;
    }
    return true;
}

//==============================================================================
void AudioManager::init(juce::String const & deviceType,
                        juce::String const & inputDevice,
                        juce::String const & outputDevice,
                        double const sampleRate,
                        int const bufferSize)
{
    JUCE_ASSERT_MESSAGE_THREAD;
    mInstance.reset(new AudioManager{ deviceType, inputDevice, outputDevice, sampleRate, bufferSize });
}

//==============================================================================
AudioManager::~AudioManager()
{
    JUCE_ASSERT_MESSAGE_THREAD;
    jassert(mAudioProcessor);
    juce::ScopedLock const sl{ mAudioProcessor->getLock() };
    if (mIsRecording) {
        stopRecording();
        mRecorders.clear(true);
    }
}

//==============================================================================
void AudioManager::registerAudioProcessor(AudioProcessor * audioProcessor)
{
    JUCE_ASSERT_MESSAGE_THREAD;
    mAudioProcessor = audioProcessor;
}

//==============================================================================
juce::StringArray AudioManager::getAvailableDeviceTypeNames()
{
    JUCE_ASSERT_MESSAGE_THREAD;
    juce::StringArray result{};
    for (auto const * deviceType : mAudioDeviceManager.getAvailableDeviceTypes()) {
        result.add(deviceType->getTypeName());
    }
    return result;
}

//==============================================================================
bool AudioManager::prepareToRecord(RecordingParameters const & recordingParams)
{
    JUCE_ASSERT_MESSAGE_THREAD;

    static constexpr auto BITS_PER_SAMPLE = 24;
    static constexpr auto RECORD_QUALITY = 0;

    jassert(std::is_sorted(recordingParams.speakersToRecord.begin(), recordingParams.speakersToRecord.end()));
    mNumSamplesRecorded = 0;
    mRecorders.clearQuick(true);

    auto * currentAudioDevice{ mAudioDeviceManager.getCurrentAudioDevice() };
    jassert(currentAudioDevice);
    if (!currentAudioDevice) {
        return false;
    }

    // prepare audioFormat
    std::unique_ptr<juce::AudioFormat> audioFormat{};
    switch (recordingParams.options.format) {
    case RecordingFormat::aiff:
        audioFormat.reset(new juce::AiffAudioFormat{});
        break;
    case RecordingFormat::wav:
        audioFormat.reset(new juce::WavAudioFormat{});
        break;
    }
    jassert(audioFormat);

    // subroutine to build the RecorderInfos
    static auto const makeRecordingInfo
        = [](juce::String const & path,
             juce::AudioFormat & format,
             double const sampleRate_,
             int const bufferSize_,
             juce::Array<float const *> dataToRecord,
             juce::TimeSliceThread & timeSlicedThread) -> std::unique_ptr<RecorderInfo> {
        juce::StringPairArray const metaData{}; // lets leave this empty for now

        juce::File const outputFile{ path };
        auto outputStream{ outputFile.createOutputStream() };
        jassert(outputStream);
        if (!outputStream) {
            return nullptr;
        }
        auto * audioFormatWriter{ format.createWriterFor(outputStream.release(),
                                                         sampleRate_,
                                                         dataToRecord.size(),
                                                         BITS_PER_SAMPLE,
                                                         metaData,
                                                         RECORD_QUALITY) };
        jassert(audioFormatWriter);
        if (!audioFormatWriter) {
            return nullptr;
        }
        auto threadedWriter{
            std::make_unique<juce::AudioFormatWriter::ThreadedWriter>(audioFormatWriter, timeSlicedThread, bufferSize_)
        };
        jassert(threadedWriter);
        auto result{ std::make_unique<RecorderInfo>() };
        result->audioFormatWriter = audioFormatWriter;
        result->threadedWriter = std::move(threadedWriter);
        result->dataToRecord = std::move(dataToRecord);
        return result;
    };

    // Compute file paths
    juce::StringArray filePaths{};
    auto const baseOutputFile{ juce::File{ recordingParams.path }.getParentDirectory().getFullPathName() + '/'
                               + juce::File{ recordingParams.path }.getFileNameWithoutExtension() };
    auto const extension{ juce::File{ recordingParams.path }.getFileExtension() };
    if (recordingParams.options.fileType == RecordingFileType::mono) {
        filePaths.ensureStorageAllocated(recordingParams.speakersToRecord.size());
        for (auto const outputPatch : recordingParams.speakersToRecord) {
            auto pathWithoutExtension{ baseOutputFile + juce::String{ outputPatch.get() } };
            auto newPath{ pathWithoutExtension + extension };
            while (filePaths.contains(newPath)) {
                pathWithoutExtension += "-2";
                newPath = pathWithoutExtension + extension;
            }
            filePaths.add(newPath);
        }
    } else {
        jassert(recordingParams.options.fileType == RecordingFileType::interleaved);
        filePaths.add(baseOutputFile + extension);
    }

    // Delete files if needed
    auto deleteAllFiles{ false };
    for (auto const & fileName : filePaths) {
        // delete file if needed
        juce::File file{ fileName };
        if (file.existsAsFile()) {
            if (!deleteAllFiles) {
                juce::AlertWindow alertWindow{ "error",
                                               "File \"" + file.getFullPathName() + "\" will be deleted. Proceed ?",
                                               juce::AlertWindow::WarningIcon };
                alertWindow.addButton("No", 0);
                alertWindow.addButton("yes", 1);
                alertWindow.addButton("Yes for all", 2);
                auto const result{ alertWindow.runModalLoop() };
                if (result == 0) {
                    return false;
                }
                if (result == 2) {
                    deleteAllFiles = true;
                }
            }
            auto const success{ file.deleteFile() };
            if (!success) {
                juce::AlertWindow::showMessageBox(juce::AlertWindow::WarningIcon,
                                                  "Error",
                                                  juce::String{ "Unable to overwrite file \"" } + file.getFullPathName()
                                                      + "\".");
                return false;
            }
        }
    }

    // Make recorders
    auto const recordingBufferSize{ RECORDERS_BUFFER_SIZE_IN_SAMPLES * recordingParams.speakersToRecord.size() };
    if (recordingParams.options.fileType == RecordingFileType::mono) {
        jassert(recordingParams.speakersToRecord.size() == filePaths.size());
        for (int i{}; i < recordingParams.speakersToRecord.size(); ++i) {
            auto const outputPatch{ recordingParams.speakersToRecord[i] };
            auto const & filePath{ filePaths[i] };
            juce::Array<float const *> dataToRecord{};
            dataToRecord.add(mOutputBuffer[outputPatch].getReadPointer(0));
            auto recordingInfo{ makeRecordingInfo(filePath,
                                                  *audioFormat,
                                                  recordingParams.sampleRate,
                                                  recordingBufferSize,
                                                  std::move(dataToRecord),
                                                  mRecordersThread) };
            if (!recordingInfo) {
                return false;
            }
            mRecorders.add(std::move(recordingInfo));
        }
    } else {
        jassert(recordingParams.options.fileType == RecordingFileType::interleaved);
        jassert(filePaths.size() == 1);
        auto const & filePath{ filePaths[0] };
        auto dataToRecord{ mOutputBuffer.getArrayOfReadPointers(recordingParams.speakersToRecord) };
        auto recordingInfo{ makeRecordingInfo(filePath,
                                              *audioFormat,
                                              recordingParams.sampleRate,
                                              recordingBufferSize,
                                              std::move(dataToRecord),
                                              mRecordersThread) };
        if (!recordingInfo) {
            return false;
        }
        mRecorders.add(std::move(recordingInfo));
    }

    mRecordersThread.startThread();

    return true;
}

//==============================================================================
void AudioManager::startRecording()
{
    JUCE_ASSERT_MESSAGE_THREAD;
    mIsRecording = true;
}

//==============================================================================
void AudioManager::stopRecording()
{
    JUCE_ASSERT_MESSAGE_THREAD;
    jassert(mAudioProcessor);
    juce::ScopedLock const sl{ mAudioProcessor->getLock() };
    // threadedWriters will flush their data before going off
    mRecorders.clear(true);
    mRecordersThread.stopThread(-1);
    mIsRecording = false;
}

//==============================================================================
void AudioManager::initInputBuffer(juce::Array<source_index_t> const & sources)
{
    JUCE_ASSERT_MESSAGE_THREAD;
    jassert(mAudioProcessor);
    juce::ScopedLock const lock{ mAudioProcessor->getLock() };
    mInputBuffer.init(sources);
}

//==============================================================================
void AudioManager::initOutputBuffer(juce::Array<output_patch_t> const & speakers)
{
    JUCE_ASSERT_MESSAGE_THREAD;
    jassert(mAudioProcessor);
    juce::ScopedLock const lock{ mAudioProcessor->getLock() };
    mOutputBuffer.init(speakers);
}

//==============================================================================
void AudioManager::setBufferSize(int const newBufferSize)
{
    JUCE_ASSERT_MESSAGE_THREAD;
    jassert(mAudioProcessor);
    juce::ScopedLock const lock{ mAudioProcessor->getLock() };
    mInputBuffer.setNumSamples(newBufferSize);
    mOutputBuffer.setNumSamples(newBufferSize);
}

//==============================================================================
void AudioManager::audioDeviceError(juce::String const & /*errorMessage*/)
{
    jassertfalse;
}

//==============================================================================
void AudioManager::audioDeviceAboutToStart(juce::AudioIODevice * /*device*/)
{
    // when AudioProcessor will be a real AudioSource, prepareToPlay() should be called here.
}

//==============================================================================
void AudioManager::audioDeviceStopped()
{
    // when AudioProcessor will be a real AudioSource, releaseResources() should be called here.
}

//==============================================================================
AudioManager & AudioManager::getInstance()
{
    jassert(mInstance);
    return *mInstance;
}

//==============================================================================
void AudioManager::free()
{
    JUCE_ASSERT_MESSAGE_THREAD;
    mInstance.reset();
}
