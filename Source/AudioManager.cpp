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
    juce::ScopedLock sl{ mCriticalSection };

    // clear buffers
    mInputBuffer.clear();
    mOutputBuffer.clear();

    // resize buffers if needed
    static auto const resizeOrClearBuffer
        = [](juce::AudioBuffer<float> & buffer, int const minInputChannels, int const numSamples) {
              if (buffer.getNumChannels() < minInputChannels || buffer.getNumSamples() < numSamples) {
                  buffer.setSize(minInputChannels, numSamples);
              } else {
                  buffer.clear();
              }
          };
    resizeOrClearBuffer(mInputBuffer, mInputs->size(), numSamples);
    resizeOrClearBuffer(mInputBuffer, mSpeakers->size(), numSamples);

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
    // the buffer is always filled in speaker id ascending order
    int bufferChannelIndex{};
    for (auto const * speaker : *mSpeakers) {
        auto const outputChannelIndex{ speaker->getOutputPatch().get() - 1 };
        jassert(outputChannelIndex >= 0);
        jassert(outputChannelIndex < mOutputBuffer.getNumChannels());
        if (outputChannelIndex < totalNumOutputChannels) {
            auto const * dataToCopyFrom{ mOutputBuffer.getReadPointer(bufferChannelIndex) };
            std::memcpy(outputChannelData[outputChannelIndex], dataToCopyFrom, sizeof(float) * numSamples);
        }
        ++bufferChannelIndex;
    }

    // Record
    auto stopRecordingAndDisplayError = [this]() {
        stopRecording();
        juce::AlertWindow::showMessageBox(
            juce::AlertWindow::AlertIconType::WarningIcon,
            "Error",
            "Recording stopped because samples were dropped.\nRecording on a faster disk might solve this issue.");
    };

    if (mIsRecording) {
        // copy samples for recorder

        /* TODO : this is very dirty. The problem comes from the fact that SpatGRIS allocates as many channels as the
         * max output patch. Changing this will require a lot of hard work, and I do not have time for this right now,
         * so the solution is to copy the active sections of the output buffer into an other buffer so that it is nicely
         * aligned for recording.
         */
        for (int i{}; i < mChannelsToRecord.size(); ++i) {
            auto const & outputPatch{ mChannelsToRecord[i] };
            mRecordingBuffer.copyFrom(i, 0, mOutputPortsBuffer.getReadPointer(outputPatch.get() - 1), numSamples);
        }

        if (mRecordingConfig == RecordingConfig::interleaved) {
            jassert(mRecorders.size() == 1);
            auto & recorder{ *mRecorders.getFirst() };
            jassert(recorder.audioFormatWriter->getNumChannels() == mRecordingBuffer.getNumChannels());
            auto const success{ recorder.threadedWriter->write(mRecordingBuffer.getArrayOfReadPointers(), numSamples) };
            jassert(success);
            if (!success) {
                stopRecordingAndDisplayError();
            }
        } else {
            jassert(mRecordingConfig == RecordingConfig::mono);
            jassert(mAudioProcessor->getMode() == SpatMode::hrtfVbap
                        ? mRecorders.size() == 2
                        : mRecorders.size() == mRecordingBuffer.getNumChannels());
            for (int channel{}; channel < mChannelsToRecord.size(); ++channel) {
                auto & recorder{ *mRecorders.getUnchecked(channel) };
                jassert(recorder.audioFormatWriter->getNumChannels() == 1);
                auto const * const channelData{ mRecordingBuffer.getArrayOfReadPointers()[channel] };
                auto const success{ recorder.threadedWriter->write(&channelData, numSamples) };
                jassert(success);
                if (!success) {
                    stopRecordingAndDisplayError();
                    break;
                }
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
                                          juce::Array<Input> const & inputs)
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

    // update channels to record
    static auto const getChannelsToRecord
        = [](Manager<Speaker, speaker_id_t> const & speakers, SpatMode const spatMode) -> juce::Array<output_patch_t> {
        juce::Array<output_patch_t> result{};
        if (spatMode == SpatMode::hrtfVbap) {
            result.add(output_patch_t{ 1 });
            result.add(output_patch_t{ 2 });
        } else {
            result.resize(speakers.size());
            std::transform(speakers.begin(),
                           speakers.end(),
                           result.begin(),
                           [](Speaker const * speaker) -> output_patch_t { return speaker->getOutputPatch(); });
            std::sort(result.begin(), result.end());
        }
        return result;
    };
    mChannelsToRecord = getChannelsToRecord(speakers, mAudioProcessor->getMode());

    // subroutine to build the SpeakerInfos
    auto makeRecordingInfo = [](juce::String const & filePath,
                                juce::AudioFormat & format,
                                int const numChannels,
                                double const recordingSampleRate,
                                int const recordingBufferSize,
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
        return result;
    };

    // build the recorders
    if (recordingOptions.config == RecordingConfig::interleaved) {
        auto const recordingBufferSize{ RECORDERS_BUFFER_SIZE_IN_SAMPLES * mOutputPortsBuffer.getNumChannels() };
        auto recorderInfo{ makeRecordingInfo(recordingOptions.path,
                                             *audioFormat,
                                             mOutputPortsBuffer.getNumChannels(),
                                             recordingOptions.sampleRate,
                                             recordingBufferSize,
                                             mRecordersThread) };
        if (!recorderInfo) {
            return false;
        }
        mRecorders.add(recorderInfo.release());
    } else {
        jassert(recordingOptions.config == RecordingConfig::mono);

        static auto const getFileNames = [](juce::String const & prefix,
                                            juce::String const & extension,
                                            juce::Array<output_patch_t> const & channelsToRecord) -> juce::StringArray {
            juce::StringArray result{};
            for (auto const outputPatch : channelsToRecord) {
                juce::String number{ outputPatch.get() };
                while (number.length() < 3) {
                    number = "0" + number;
                }

                result.add(prefix + "_" + number + extension);
            }
            return result;
        };

        auto const baseOutputFile{ juce::File{ recordingOptions.path }.getParentDirectory().getFullPathName() + '/'
                                   + juce::File{ recordingOptions.path }.getFileNameWithoutExtension() };
        auto const extension{ juce::File{ recordingOptions.path }.getFileExtension() };

        auto const fileNames{ getFileNames(baseOutputFile, extension, mChannelsToRecord) };

        bool deleteAllFiles{ false };
        for (auto const & fileName : fileNames) {
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
                                                      juce::String{ "Unable to overwrite file \"" }
                                                          + file.getFullPathName() + "\".");
                    return false;
                }
            }
            auto recorderInfo{ makeRecordingInfo(fileName,
                                                 *audioFormat,
                                                 1,
                                                 recordingOptions.sampleRate,
                                                 RECORDERS_BUFFER_SIZE_IN_SAMPLES,
                                                 mRecordersThread) };
            if (!recorderInfo) {
                return false;
            }
            mRecorders.add(recorderInfo.release());
        }
    }

    // allocate the recording buffer
    // 2048 is fine. Worst case is the audio callback will allocate at the first call.
    static constexpr auto BUFFER_SIZE{ 2048 };
    mRecordingBuffer.setSize(speakers.size(), BUFFER_SIZE);

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
    setBufferSizes(device->getCurrentBufferSizeSamples());
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
