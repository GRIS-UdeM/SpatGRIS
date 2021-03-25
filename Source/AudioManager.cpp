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
    jassert(numSamples <= mOutputBuffer.MAX_BUFFER_LENGTH);
    juce::ScopedLock sl{ mCriticalSection };

    // clear buffers
    mOutputBuffer.clear();

    // resize buffers if needed
    if (mInputBuffer.getNumChannels() != mInputs->size() || mInputBuffer.getNumSamples() != numSamples) {
        // TODO: should this be moved outside of the audio loop?
        mInputBuffer.setSize(mInputs->size(), numSamples);
    } else {
        mInputBuffer.clear();
    }

    // TODO: DO NOT PUT THIS IN THE AUDIO LOOP!
    mOutputBuffer.setSpeakers(*mSpeakers);

    // copy input data to buffers
    auto const numInputChannelsToCopy{ std::min(totalNumInputChannels, mInputBuffer.getNumChannels()) };
    for (int i{}; i < numInputChannelsToCopy; ++i) {
        mInputBuffer.copyFrom(i, 0, inputChannelData[i], numSamples);
    }

    // do the actual processing
    if (mAudioProcessor != nullptr) {
        mAudioProcessor->processAudio(mInputBuffer, mOutputBuffer);
    }

    // copy buffers to output
    mOutputBuffer.copyToPhysicalOutput(outputChannelData, totalNumOutputChannels, numSamples);

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
    mInstance.reset(new AudioManager{ deviceType, inputDevice, outputDevice, sampleRate, bufferSize });
}

//==============================================================================
AudioManager::~AudioManager()
{
    juce::ScopedLock const sl{ mCriticalSection };
    if (mIsRecording) {
        stopRecording();
        mRecorders.clear(true);
    }
}

//==============================================================================
void AudioManager::registerAudioProcessor(AudioProcessor * audioProcessor,
                                          Manager<Speaker, speaker_id_t> const & speakers,
                                          juce::OwnedArray<Input> const & inputs)
{
    juce::ScopedLock sl{ mCriticalSection };
    mAudioProcessor = audioProcessor;
    mSpeakers = &speakers;
    mInputs = &inputs;
}

//==============================================================================
juce::StringArray AudioManager::getAvailableDeviceTypeNames()
{
    juce::StringArray result{};
    for (auto const * deviceType : mAudioDeviceManager.getAvailableDeviceTypes()) {
        result.add(deviceType->getTypeName());
    }
    return result;
}

//==============================================================================
bool AudioManager::prepareToRecord(RecordingOptions const & recordingOptions,
                                   Manager<Speaker, speaker_id_t> const & speakers)
{
    static constexpr auto BITS_PER_SAMPLE = 24;
    static constexpr auto RECORD_QUALITY = 0;

    mRecordingConfig = recordingOptions.config;
    mNumSamplesRecorded = 0;
    mRecorders.clearQuick(true);

    auto * currentAudioDevice{ mAudioDeviceManager.getCurrentAudioDevice() };
    if (!currentAudioDevice) {
        return false;
    }

    // prepare audioFormat
    std::unique_ptr<juce::AudioFormat> audioFormat{};
    switch (recordingOptions.format) {
    case RecordingFormat::aiff:
        audioFormat.reset(new juce::AiffAudioFormat{});
        break;
    case RecordingFormat::wav:
        audioFormat.reset(new juce::WavAudioFormat{});
        break;
    }
    jassert(audioFormat);

    // subroutine to build the RecorderInfos
    auto makeRecordingInfo = [](juce::String const & filePath,
                                juce::AudioFormat & format,
                                int const numChannels,
                                double const recordingSampleRate,
                                int const recordingBufferSize,
                                juce::Array<float const *> dataToRecord,
                                juce::TimeSliceThread & timeSlicedThread) -> std::unique_ptr<RecorderInfo> {
        juce::StringPairArray const metaData{}; // lets leave this empty for now

        juce::File const outputFile{ filePath };
        auto outputStream{ outputFile.createOutputStream() };
        if (!outputStream) {
            return nullptr;
        }
        auto * audioFormatWriter{ format.createWriterFor(outputStream.release(),
                                                         recordingSampleRate,
                                                         numChannels,
                                                         BITS_PER_SAMPLE,
                                                         metaData,
                                                         RECORD_QUALITY) };
        if (!audioFormatWriter) {
            return nullptr;
        }
        std::unique_ptr<juce::AudioFormatWriter::ThreadedWriter> threadedWriter{
            new juce::AudioFormatWriter::ThreadedWriter{ audioFormatWriter, timeSlicedThread, recordingBufferSize }
        };
        auto result{ std::make_unique<RecorderInfo>() };
        result->audioFormatWriter = audioFormatWriter;
        result->threadedWriter = std::move(threadedWriter);
        result->dataToRecord = std::move(dataToRecord);
        return result;
    };

    struct ChannelInfo {
        output_patch_t outputPatch;
        speaker_id_t speakerId;
    };

    // Register channel info
    juce::Array<ChannelInfo> channelInfo{};
    {
        juce::ScopedLock const lock{ mSpeakers->getLock() };
        channelInfo.ensureStorageAllocated(mSpeakers->size());
        if (mAudioProcessor->getMode() == SpatMode::hrtfVbap) {
            auto const * const * firstSpeakerIt{ std::find_if(
                mSpeakers->cbegin(),
                mSpeakers->cend(),
                [](Speaker const * speaker) { return speaker->getOutputPatch().get() == 1; }) };
            auto const * const * secondSpeakerIt{ std::find_if(
                mSpeakers->cbegin(),
                mSpeakers->cend(),
                [](Speaker const * speaker) { return speaker->getOutputPatch().get() == 2; }) };
            jassert(firstSpeakerIt != mSpeakers->cend() && secondSpeakerIt != mSpeakers->cend());
            channelInfo.add(ChannelInfo{ output_patch_t{ 1 }, (*firstSpeakerIt)->getSpeakerId() });
            channelInfo.add(ChannelInfo{ output_patch_t{ 2 }, (*secondSpeakerIt)->getSpeakerId() });
        } else {
            channelInfo.resize(mSpeakers->size());
            std::transform(mSpeakers->cbegin(), mSpeakers->cend(), channelInfo.begin(), [](Speaker const * speaker) {
                return ChannelInfo{ speaker->getOutputPatch(), speaker->getSpeakerId() };
            });
            std::sort(channelInfo.begin(), channelInfo.end(), [](ChannelInfo const & a, ChannelInfo const & b) {
                return a.outputPatch < b.outputPatch;
            });
        }
    }

    // Compute file paths
    juce::StringArray filePaths{};
    auto const baseOutputFile{ juce::File{ recordingOptions.path }.getParentDirectory().getFullPathName() + '/'
                               + juce::File{ recordingOptions.path }.getFileNameWithoutExtension() };
    auto const extension{ juce::File{ recordingOptions.path }.getFileExtension() };
    if (recordingOptions.config == RecordingConfig::mono) {
        filePaths.ensureStorageAllocated(channelInfo.size());
        for (auto const & info : channelInfo) {
            auto pathWithoutExtension{ baseOutputFile + juce::String{ info.outputPatch.get() } };
            auto newPath{ pathWithoutExtension + extension };
            while (filePaths.contains(newPath)) {
                pathWithoutExtension += "-2";
                newPath = pathWithoutExtension + extension;
            }
            filePaths.add(newPath);
        }
    } else {
        jassert(recordingOptions.config == RecordingConfig::interleaved);
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
    auto const recordingBufferSize{ RECORDERS_BUFFER_SIZE_IN_SAMPLES * channelInfo.size() };
    if (recordingOptions.config == RecordingConfig::mono) {
        jassert(channelInfo.size() == filePaths.size());
        for (int i{}; i < channelInfo.size(); ++i) {
            auto const & speakerId{ channelInfo[i].speakerId };
            auto const & filePath{ filePaths[i] };
            juce::Array<float const *> dataToRecord{};
            dataToRecord.add(mOutputBuffer.getReadPointer(speakerId));
            auto recordingInfo{ makeRecordingInfo(filePath,
                                                  *audioFormat,
                                                  1,
                                                  recordingOptions.sampleRate,
                                                  recordingBufferSize,
                                                  std::move(dataToRecord),
                                                  mRecordersThread) };
            if (!recordingInfo) {
                return false;
            }
            mRecorders.add(std::move(recordingInfo));
        }
    } else {
        jassert(recordingOptions.config == RecordingConfig::interleaved);
        jassert(filePaths.size() == 1);
        auto const & filePath{ filePaths[0] };
        StaticVector<speaker_id_t, MAX_OUTPUTS> speakersToRecord{};
        speakersToRecord.resize(channelInfo.size());
        std::transform(channelInfo.begin(), channelInfo.end(), speakersToRecord.begin(), [](ChannelInfo const & info) {
            return info.speakerId;
        });
        auto const staticDataToRecord{ mOutputBuffer.getArrayOfReadPointers(speakersToRecord) };
        juce::Array<float const *> dataToRecord{};
        dataToRecord.resize(staticDataToRecord.size());
        std::copy(staticDataToRecord.cbegin(), staticDataToRecord.cend(), dataToRecord.begin());
        auto recordingInfo{ makeRecordingInfo(filePath,
                                              *audioFormat,
                                              speakersToRecord.size(),
                                              recordingOptions.sampleRate,
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
    juce::ScopedLock sl{ mCriticalSection };
    mIsRecording = true;
}

//==============================================================================
void AudioManager::stopRecording()
{
    juce::ScopedLock sl{ mCriticalSection };
    // threadedWriters will flush their data before going off
    mRecorders.clear(true);
    mRecordersThread.stopThread(-1);
    mIsRecording = false;
}

//==============================================================================
void AudioManager::audioDeviceError(juce::String const & /*errorMessage*/)
{
    jassertfalse;
}

//==============================================================================
void AudioManager::audioDeviceAboutToStart(juce::AudioIODevice * device)
{
    juce::ScopedLock sl{ mCriticalSection };
    // when AudioProcessor will be a real AudioSource, prepareToPlay() should be called here.
}

//==============================================================================
void AudioManager::audioDeviceStopped()
{
    juce::ScopedLock sl{ mCriticalSection };
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
    mInstance.reset();
}
