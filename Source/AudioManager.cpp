#include "AudioManager.h"

#if !USE_JACK

    #include <iostream>

//==============================================================================
jack_port_t * AudioManager::registerPort(char const * const newShortName,
                                         char const * const newClientName,
                                         PortType const newType,
                                         std::optional<int> newPhysicalPort)
{
    juce::ScopedLock sl{ mCriticalSection };

    auto newPort{
        std::make_unique<_jack_port>(++mLastGivePortId, newShortName, newClientName, newType, newPhysicalPort)
    };

    jack_port_t * result{};

    switch (newType) {
    case PortType::input:
        jassert(!mInputPorts.contains(newPort.get()));
        result = mInputPorts.add(newPort.release());
        break;
    case PortType::output:
        jassert(!mOutputPorts.contains(newPort.get()));
        result = mOutputPorts.add(newPort.release());
        break;
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
void AudioManager::unregisterPort(jack_port_t * port)
{
    juce::ScopedLock sl{ mCriticalSection };

    if (mConnections.contains(port)) {
        mConnections.remove(port);
    }
    if (mConnections.containsValue(port)) {
        mConnections.removeValue(port);
    }

    auto const inputIndex{ mInputPorts.indexOf(port) };
    if (inputIndex > 0) {
        mInputPorts.remove(inputIndex);
    } else {
        jassert(mOutputPorts.contains(port));
        mOutputPorts.removeObject(port);
    }
}

//==============================================================================
bool AudioManager::isConnectedTo(jack_port_t const * port, char const * port_name) const
{
    juce::ScopedLock sl{ mCriticalSection };

    if (!mConnections.contains(port)) {
        return false;
    }

    return strcmp(port_name, mConnections[port]->fullName) == 0;
}

//==============================================================================
jack_port_t * AudioManager::findPortByName(char const * name) const
{
    juce::ScopedLock sl{ mCriticalSection };

    for (auto const port : mInputPorts) {
        if (strcmp(port->fullName, name) == 0) {
            return port;
        }
    }
    for (auto const port : mOutputPorts) {
        if (strcmp(port->fullName, name) == 0) {
            return port;
        }
    }
    return nullptr;
}

//==============================================================================
void AudioManager::registerProcessCallback(JackProcessCallback const callback, void * arg)
{
    juce::ScopedLock sl{ mCriticalSection };

    mProcessCallback = callback;
    mProcessCallbackArg = arg;
}

//==============================================================================
void * AudioManager::getBuffer(jack_port_t * port, [[maybe_unused]] jack_nframes_t const nFrames)
{
    juce::ScopedLock sl{ mCriticalSection };

    switch (port->type) {
    case PortType::input:
        jassert(mInputBuffer.getNumSamples() >= nFrames);
        return mInputBuffer.getWritePointer(mInputPorts.indexOf(port));
    case PortType::output:
        jassert(mOutputBuffer.getNumSamples() >= nFrames);
        return mOutputBuffer.getWritePointer(mOutputPorts.indexOf(port));
    }
    jassertfalse;
    return nullptr;
}

//==============================================================================
std::optional<jack_port_t *> AudioManager::getPort(char const * name) const
{
    juce::ScopedLock sl{ mCriticalSection };

    for (auto * port : mInputPorts) {
        if (strcmp(name, port->fullName) == 0) {
            return port;
        }
    }
    for (auto * port : mOutputPorts) {
        if (strcmp(name, port->fullName) == 0) {
            return port;
        }
    }
    return std::nullopt;
}

//==============================================================================
void AudioManager::connect(char const * sourcePortName, char const * destinationPortName)
{
    juce::ScopedLock sl{ mCriticalSection };

    auto const maybe_sourcePort{ getPort(sourcePortName) };
    auto const maybe_destinationPort{ getPort(destinationPortName) };

    jassert(maybe_sourcePort);
    jassert(maybe_destinationPort);

    auto * sourcePort{ *maybe_sourcePort };
    auto * destinationPort{ *maybe_destinationPort };

    jassert(!mConnections.contains(sourcePort));
    jassert(sourcePort->type == PortType::output);
    jassert(destinationPort->type == PortType::input);

    mConnections.set(sourcePort, destinationPort);

    mPortConnectCallback(sourcePort->id, destinationPort->id, 0, nullptr);
}

//==============================================================================
void AudioManager::disconnect(jack_port_t * source, jack_port_t * destination)
{
    juce::ScopedLock sl{ mCriticalSection };

    jassert(mConnections.contains(source));
    jassert(mConnections[source] == destination);

    mConnections.remove(source);

    mPortConnectCallback(source->id, destination->id, 0, nullptr);
}

//==============================================================================
void AudioManager::audioDeviceError(const String & errorMessage)
{
    jassertfalse;
}

//==============================================================================
void AudioManager::audioDeviceIOCallback(const float ** inputChannelData,
                                         int const totalNumInputChannels,
                                         float ** const outputChannelData,
                                         int const totalNumOutputChannels,
                                         int const numSamples)
{
    juce::ScopedLock sl{ mCriticalSection };

    jassert(totalNumInputChannels <= mInputBuffer.getNumChannels());
    mInputBuffer.clear();
    for (int i{}; i < totalNumInputChannels; ++i) {
        mInputBuffer.copyFrom(i, 0, inputChannelData[i], numSamples);
    }

    if (mProcessCallback != nullptr) {
        mProcessCallback(numSamples, mProcessCallbackArg);
    }

    jassert(totalNumOutputChannels <= mOutputBuffer.getNumChannels());

    decltype(mConnections)::Iterator connection{ mConnections };
    while (connection.next()) {
        auto * source{ connection.getKey() };
        auto const * destination{ connection.getValue() };
        if (destination->physicalPort.has_value()) {
            auto const sourceIndex{ mOutputPorts.indexOf(source) };
            auto const destinationIndex{ *destination->physicalPort };
            std::memcpy(outputChannelData[destinationIndex],
                        mOutputBuffer.getReadPointer(sourceIndex),
                        sizeof(float) * numSamples);
        }
    }
}

//==============================================================================
void AudioManager::audioDeviceAboutToStart(AudioIODevice * device)
{
    juce::ScopedLock sl{ mCriticalSection };

    auto const bufferSize{ device->getCurrentBufferSizeSamples() };
    setBufferSizes(bufferSize);

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
    static AudioManager instance{};
    return instance;
}

//==============================================================================
void AudioManager::setBufferSizes(int const numSamples)
{
    juce::ScopedLock sl{ mCriticalSection };

    mInputBuffer.setSize(mInputPorts.size(), numSamples, false, false, false);
    mOutputBuffer.setSize(mOutputPorts.size(), numSamples, false, false, false);
}

//==============================================================================
AudioManager::AudioManager()
{
    // TODO: magic numbers
    auto error{ mAudioDeviceManager.initialiseWithDefaultDevices(256, 256) };
    jassert(error.isEmpty());

    AudioDeviceManager::AudioDeviceSetup setup{};

    for (auto const deviceType : mAudioDeviceManager.getAvailableDeviceTypes()) {
        deviceType->scanForDevices();
        auto const hasSeparateInputsAndOutputs{ deviceType->hasSeparateInputsAndOutputs() };
        std::cout << "=======================================\n";
        std::cout << deviceType->getTypeName() << '\n';
        std::cout << "Has seperate inputs and outputs : " << (hasSeparateInputsAndOutputs ? "YES" : "NO") << "\n\n";
        for (auto const & deviceName : deviceType->getDeviceNames()) {
            std::cout << "\t" << deviceName << '\n';
        }
    }

    auto * audioDevice{ mAudioDeviceManager.getCurrentAudioDevice() };
    jassert(audioDevice != nullptr);
    int channelIndex{};
    for (auto const & inputChannelName : audioDevice->getInputChannelNames()) {
        registerPort(inputChannelName.toStdString().c_str(), "system", PortType::input, channelIndex++);
    }
    channelIndex = 0;
    for (auto const & outputChannelName : audioDevice->getOutputChannelNames()) {
        registerPort(outputChannelName.toStdString().c_str(), "system", PortType::output, channelIndex++);
    }

    mAudioDeviceManager.addAudioCallback(this);
}

#endif
