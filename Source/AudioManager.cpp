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

static_assert(!USE_JACK);

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

    // record samples
    if (mIsRecording) {
        if (mRecordingConfig == RecordingConfig::interleaved) {
            jassert(mRecorders.size() == 1);
            auto & recorder{ *mRecorders.getFirst() };
            jassert(recorder.getNumChannels() == mOutputPortsBuffer.getNumChannels());
            recorder.writeFromFloatArrays(mOutputPortsBuffer.getArrayOfReadPointers(),
                                          mOutputPortsBuffer.getNumChannels(),
                                          numSamples);
        } else {
            jassert(mRecordingConfig == RecordingConfig::mono);
            jassert(mRecorders.size() == mOutputPortsBuffer.getNumChannels());
            for (int channel{}; channel < mOutputPortsBuffer.getNumChannels(); ++channel) {
                auto & recorder{ *mRecorders.getUnchecked(channel) };
                ;
                jassert(recorder.getNumChannels() == 1);
                float const * const channelData{ mOutputPortsBuffer.getArrayOfReadPointers()[channel] };
                recorder.writeFromFloatArrays(&channelData, 1, numSamples);
            }
        }
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
                                      double const sampleRate,
                                      int const bufferSize)
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
    juce::AudioDeviceManager::AudioDeviceSetup const setup{
        outputDevice, inputDevice, sampleRate, bufferSize, neededInputChannels, false, neededOutputChannels, false
    };
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
audio_port_t * AudioManager::registerPort(char const * const newShortName,
                                          char const * const newClientName,
                                          PortType const newType,
                                          std::optional<int> newPhysicalPort)
{
    juce::ScopedLock sl{ mCriticalSection };

    auto newPort{
        std::make_unique<audio_port_t>(++mLastGivePortId, newShortName, newClientName, newType, newPhysicalPort)
    };

    auto const * type{ (newType == PortType::input ? "input" : "output") };
    if (newPhysicalPort.has_value()) {
        std::cout << "Registered physical " << type << "port : id=" << newPort->id
                  << " physicalChannel=" << *newPhysicalPort << '\n';
    } else {
        std::cout << "Registered virtual " << type << " port : id=" << newPort->id << '\n';
    }

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

    std::cout << "Unregistered port #" << port->id << '\n';

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
        jassert(mInputPortsBuffer.getNumSamples() >= nFrames);
        auto const index{ mVirtualInputPorts.indexOf(port) };
        return mInputPortsBuffer.getWritePointer(index);
    }
    case PortType::output: {
        jassert(mOutputPortsBuffer.getNumSamples() >= nFrames);
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
audio_port_t * AudioManager::getPort(uint32_t const id) const
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
        result.push_back(port->fullName);
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
bool AudioManager::prepareToRecord(RecordingOptions const & recordingOptions)
{
    static constexpr auto BITS_PER_SAMPLE = 24;
    static constexpr auto RECORD_QUALITY = 0;

    mRecordingConfig = recordingOptions.config;

    auto * currentAudioDevice{ mAudioDeviceManager.getCurrentAudioDevice() };
    if (!currentAudioDevice) {
        return false;
    }

    std::unique_ptr<juce::AudioFormat> format{};
    switch (recordingOptions.format) {
    case RecordingFormat::aiff:
        format.reset(new juce::AiffAudioFormat{});
        break;
    case RecordingFormat::wav:
        format.reset(new juce::WavAudioFormat{});
        break;
    }
    jassert(format);

    juce::StringPairArray const metaData{}; // lets leave this empty for now

    if (recordingOptions.config == RecordingConfig::interleaved) {
        juce::File const outputFile{ recordingOptions.path };
        auto outputStream{ outputFile.createOutputStream() };
        if (!outputStream) {
            mRecorders.clear(true);
            return false;
        }
        mRecorders.add(format->createWriterFor(outputStream.release(),
                                               recordingOptions.sampleRate,
                                               mOutputPortsBuffer.getNumChannels(),
                                               BITS_PER_SAMPLE,
                                               metaData,
                                               RECORD_QUALITY));
    } else {
        jassert(recordingOptions.config == RecordingConfig::mono);

        auto const baseOutputFile{ juce::File{ recordingOptions.path }.getParentDirectory().getFullPathName() + '/'
                                   + juce::File{ recordingOptions.path }.getFileNameWithoutExtension() };
        auto const extension{ juce::File{ recordingOptions.path }.getFileExtension() };

        for (int output{}; output < mOutputPortsBuffer.getNumChannels(); ++output) {
            juce::String fileIndex{ output };
            while (fileIndex.length() < 3) {
                fileIndex = "0" + fileIndex;
            }
            auto const outputPath{ baseOutputFile + "_" + fileIndex + extension };
            juce::File const outputFile{ outputPath };
            auto outputStream{ outputFile.createOutputStream() };
            if (!outputStream) {
                mRecorders.clear(true);
                return false;
            }
            mRecorders.add(format->createWriterFor(outputStream.release(),
                                                   recordingOptions.sampleRate,
                                                   1,
                                                   BITS_PER_SAMPLE,
                                                   metaData,
                                                   RECORD_QUALITY));
        }
    }

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
    for (auto * recorder : mRecorders) {
        // TODO : is this necessary ?
        recorder->flush();
    }
    mRecorders.clear(true);
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

    // TODO : call prepareToPlay
}

//==============================================================================
void AudioManager::audioDeviceStopped()
{
    juce::ScopedLock sl{ mCriticalSection };

    // TODO : call releaseResources
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
