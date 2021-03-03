/*
 This file is part of SpatGRIS2.

 Developers: Samuel B�land, Olivier B�langer, Nicolas Masson

 SpatGRIS2 is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 SpatGRIS2 is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with SpatGRIS2.  If not, see <http://www.gnu.org/licenses/>.
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
    int channelIndex{};
    for (auto const & inputChannelName : audioDevice->getInputChannelNames()) {
        registerPort((juce::String{ "input " } + inputChannelName).toStdString().c_str(),
                     "system",
                     PortType::output,
                     channelIndex++);
    }
    channelIndex = 0;
    for (auto const & outputChannelName : audioDevice->getOutputChannelNames()) {
        registerPort((juce::String{ "output " } + outputChannelName).toStdString().c_str(),
                     "system",
                     PortType::input,
                     channelIndex++);
    }

    mRecordersThread.setPriority(9);

    mAudioDeviceManager.addAudioCallback(this);
}

//==============================================================================
void AudioManager::audioDeviceIOCallback(const float ** inputChannelData,
                                         int const totalNumInputChannels,
                                         float ** const outputChannelData,
                                         int const totalNumOutputChannels,
                                         int const numSamples)
{
    juce::ScopedLock sl{ mCriticalSection };

    // clear buffers
    mOutputPortsBuffer.clear();
    mInputPortsBuffer.clear();

    // copy input channels to input ports
    auto const numInputChannelsToCopy{ std::min(totalNumInputChannels, mVirtualInputPorts.size()) };
    for (int i{}; i < numInputChannelsToCopy; ++i) {
        auto const * dest{ inputChannelData[i] };
        if (dest) {
            mInputPortsBuffer.copyFrom(i, 0, dest, numSamples);
        }
    }

    // do the actual processing
    if (mAudioProcessor != nullptr) {
        mAudioProcessor->processAudio(numSamples);
    }

    // copy output ports to output channels
    auto const numOutputChannelsToCopy{ std::min(totalNumOutputChannels, mVirtualOutputPorts.size()) };
    for (int i{}; i < numOutputChannelsToCopy; ++i) {
        auto * dest{ outputChannelData[i] };
        if (dest) {
            std::memcpy(dest, mOutputPortsBuffer.getReadPointer(i), sizeof(float) * numSamples);
        }
    }

    auto stopRecordingAndDisplayError = [this]() {
        stopRecording();
        juce::AlertWindow::showMessageBox(
            juce::AlertWindow::AlertIconType::WarningIcon,
            "Error",
            "Recording stopped because samples were dropped.\nRecording on a faster disk might solve this issue.");
    };

    // record samples
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
void AudioManager::setBufferSizes(int const numSamples)
{
    juce::ScopedLock sl{ mCriticalSection };

    mInputPortsBuffer.setSize(mVirtualInputPorts.size(), numSamples, false, false, false);
    mOutputPortsBuffer.setSize(mVirtualOutputPorts.size(), numSamples, false, false, false);
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
audio_port_t * AudioManager::registerPort(char const * const newShortName,
                                          char const * const newClientName,
                                          PortType const newType,
                                          std::optional<int> newPhysicalPort)
{
    juce::ScopedLock sl{ mCriticalSection };

    auto newPort{
        std::make_unique<audio_port_t>(++mLastGivePortId, newShortName, newClientName, newType, newPhysicalPort)
    };

    audio_port_t * result;

    if (newType == PortType::input) {
        if (newPhysicalPort.has_value()) {
            result = mPhysicalInputPorts.add(newPort.release());
        } else {
            result = mVirtualInputPorts.add(newPort.release());
        }
    } else {
        if (newPhysicalPort.has_value()) {
            result = mPhysicalOutputPorts.add(newPort.release());
        } else {
            result = mVirtualOutputPorts.add(newPort.release());
        }
    }
    jassert(result != nullptr);

    auto * device{ mAudioDeviceManager.getCurrentAudioDevice() };
    if (device != nullptr) {
        if (device->isPlaying()) {
            setBufferSizes(device->getCurrentBufferSizeSamples());
        }
    }
    return result;
}

//==============================================================================
void AudioManager::unregisterPort(audio_port_t * port)
{
    juce::ScopedLock sl{ mCriticalSection };

    if (mConnections.contains(port)) {
        disconnect(port, mConnections[port]);
    }
    if (mConnections.containsValue(port)) {
        decltype(mConnections)::Iterator it{ mConnections };
        while (it.next()) {
            if (it.getValue() == port) {
                disconnect(it.getKey(), port);
            }
        }
    }

    mPhysicalOutputPorts.removeObject(port);
    mPhysicalInputPorts.removeObject(port);
    mVirtualOutputPorts.removeObject(port);
    mVirtualInputPorts.removeObject(port);
}

//==============================================================================
bool AudioManager::isConnectedTo(audio_port_t const * port, char const * port_name) const
{
    juce::ScopedLock sl{ mCriticalSection };

    if (!mConnections.contains(const_cast<audio_port_t *>(port))) {
        return false;
    }

    return strcmp(port_name, mConnections[const_cast<audio_port_t *>(port)]->fullName) == 0;
}

//==============================================================================
bool AudioManager::isConnectedTo(audio_port_t const * portA, audio_port_t const * portB) const
{
    juce::ScopedLock sl{ mCriticalSection };

    if (!mConnections.contains(const_cast<audio_port_t *>(portA))) {
        return false;
    }

    return mConnections[const_cast<audio_port_t *>(portA)] == portB;
}

//==============================================================================
void AudioManager::registerAudioProcessor(AudioProcessor * audioProcessor)
{
    juce::ScopedLock sl{ mCriticalSection };
    mAudioProcessor = audioProcessor;
}

//==============================================================================
juce::Array<audio_port_t *> AudioManager::getInputPorts() const
{
    juce::Array<audio_port_t *> result{};
    result.addArray(mPhysicalInputPorts);
    result.addArray(mVirtualInputPorts);
    return result;
}

//==============================================================================
juce::Array<audio_port_t *> AudioManager::getOutputPorts() const
{
    juce::Array<audio_port_t *> result{};
    result.addArray(mPhysicalOutputPorts);
    result.addArray(mVirtualOutputPorts);
    return result;
}

//==============================================================================
float * AudioManager::getBuffer(audio_port_t * port, [[maybe_unused]] size_t const nFrames) noexcept
{
    juce::ScopedLock sl{ mCriticalSection };

    jassert(!port->physicalPort.has_value());

    switch (port->type) {
    case PortType::input: {
        jassert(mInputPortsBuffer.getNumSamples() >= narrow<int>(nFrames));
        auto const index{ mVirtualInputPorts.indexOf(port) };
        return mInputPortsBuffer.getWritePointer(index);
    }
    case PortType::output: {
        jassert(mOutputPortsBuffer.getNumSamples() >= narrow<int>(nFrames));
        auto const index{ mVirtualOutputPorts.indexOf(port) };
        return mOutputPortsBuffer.getWritePointer(index);
    }
    }
    jassertfalse;
    return nullptr;
}

//==============================================================================
audio_port_t * AudioManager::getPort(char const * name) const
{
    juce::ScopedLock sl{ mCriticalSection };

    auto const find = [=](juce::OwnedArray<audio_port_t> const & portArray) -> audio_port_t * {
        for (auto * port : portArray) {
            if (strcmp(port->fullName, name) == 0) {
                return port;
            }
        }
        return nullptr;
    };

    auto * found{ find(mVirtualInputPorts) };
    if (found) {
        return found;
    }
    found = find(mVirtualOutputPorts);
    if (found) {
        return found;
    }
    found = find(mPhysicalOutputPorts);
    if (found) {
        return found;
    }
    found = find(mPhysicalInputPorts);
    jassert(found);
    return found;
}

//==============================================================================
audio_port_t * AudioManager::getPort(port_id_t const id) const
{
    for (auto * port : getInputPorts()) {
        if (port->id == id) {
            return port;
        }
    }
    for (auto * port : getOutputPorts()) {
        if (port->id == id) {
            return port;
        }
    }

    return nullptr;
}

//==============================================================================
std::vector<std::string> AudioManager::getPortNames(PortType const portType) const
{
    std::vector<std::string> result{};

    auto const ports{ portType == PortType::input ? getInputPorts() : getOutputPorts() };

    for (auto const * port : ports) {
        result.emplace_back(port->fullName);
    }

    return result;
}

//==============================================================================
void AudioManager::connect(char const * sourcePortName, char const * destinationPortName)
{
    juce::ScopedLock sl{ mCriticalSection };

    connect(getPort(sourcePortName), getPort(destinationPortName));
}

//==============================================================================
void AudioManager::connect(audio_port_t * sourcePort, audio_port_t * destinationPort)
{
    juce::ScopedLock sl{ mCriticalSection };

    jassert(sourcePort->type == PortType::output);
    jassert(destinationPort->type == PortType::input);

    jassert(!mConnections.contains(sourcePort));
    jassert(sourcePort->type == PortType::output);
    jassert(destinationPort->type == PortType::input);

    mConnections.set(sourcePort, destinationPort);
}

//==============================================================================
void AudioManager::disconnect(audio_port_t * sourcePort, [[maybe_unused]] audio_port_t * destinationPort)
{
    juce::ScopedLock sl{ mCriticalSection };

    jassert(mConnections.contains(sourcePort));
    jassert(mConnections[sourcePort] == destinationPort);

    mConnections.remove(sourcePort);
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
                                   ::juce::OwnedArray<Speaker> const & speakers)
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
        = [](juce::OwnedArray<Speaker> const & speakers, SpatMode const spatMode) -> juce::Array<output_patch_t> {
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
void AudioManager::audioDeviceError(const juce::String & /*errorMessage*/)
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
