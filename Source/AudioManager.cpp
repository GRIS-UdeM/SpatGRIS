#include "AudioManager.h"

#if !USE_JACK

    #include <iostream>

//==============================================================================
AudioManager::AudioManager()
{
    // display devices info
    for (auto const deviceType : mAudioDeviceManager.getAvailableDeviceTypes()) {
        deviceType->scanForDevices();
        auto const hasSeparateInputsAndOutputs{ deviceType->hasSeparateInputsAndOutputs() };
        std::cout << "=======================================\n";
        std::cout << deviceType->getTypeName() << '\n';
        std::cout << "Has separate inputs and outputs : " << (hasSeparateInputsAndOutputs ? "YES" : "NO") << "\n\n";
        std::cout << "Inputs :\n";
        for (auto const & deviceName : deviceType->getDeviceNames(true)) {
            std::cout << "\t" << deviceName << '\n';
        }
        std::cout << "Outputs :\n";
        for (auto const & deviceName : deviceType->getDeviceNames(false)) {
            std::cout << "\t" << deviceName << '\n';
        }
    }

    // TODO: magic numbers
    #ifdef WIN32
    juce::String const outputDevice{ "Realtek Digital Output (Realtek High Definition Audio)" };
    juce::String const inputDevice{ "Analogue 1 + 2 (Focusrite Usb Audio)" };
    #elif defined __APPLE__
    juce::String const outputDevice{ "MacBook Pro Speakers" };
    juce::String const inputDevice{ "BlackHole 128ch" };
    #else
    static_assert(false, "yo");
    #endif
    juce::AudioDeviceManager::AudioDeviceSetup const setup{
        outputDevice, inputDevice, 48000.0, 512, juce::BigInteger{}, true, juce::BigInteger{}, true
    };

    auto error{ mAudioDeviceManager.initialise(128, 128, nullptr, false, setup.outputDeviceName, &setup) };
    jassert(error.isEmpty());

    auto * audioDevice{ mAudioDeviceManager.getCurrentAudioDevice() };
    jassert(audioDevice != nullptr);
    int channelIndex{};
    for (auto const & inputChannelName : audioDevice->getInputChannelNames()) {
        registerPort(inputChannelName.toStdString().c_str(), "system", PortType::output, channelIndex++);
    }
    channelIndex = 0;
    for (auto const & outputChannelName : audioDevice->getOutputChannelNames()) {
        registerPort(outputChannelName.toStdString().c_str(), "system", PortType::input, channelIndex++);
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
    mOutputPortsBuffer.clear();
    mInputPortsBuffer.clear();

    // copy input channels to input ports
    auto const numInputChannelsToCopy{ std::min(totalNumInputChannels, mVirtualInputPorts.size()) };
    for (int i{}; i < numInputChannelsToCopy; ++i) {
        auto * dest{ inputChannelData[i] };
        if (dest) {
            mInputPortsBuffer.copyFrom(i, 0, dest, numSamples);
        }
    }
    auto const inputPeak{ mOutputPortsBuffer.getMagnitude(0, numSamples) };

    if (mProcessCallback != nullptr) {
        mProcessCallback(numSamples, mProcessCallbackArg);
    }
    auto const outputPeak{ mInputPortsBuffer.getMagnitude(0, numSamples) };

    // copy output ports to output channels
    auto const numOutputChannelsToCopy{ std::min(totalNumOutputChannels, mVirtualOutputPorts.size()) };
    for (int i{}; i < numOutputChannelsToCopy; ++i) {
        auto * dest{ outputChannelData[i] };
        if (dest) {
            std::memcpy(dest, mOutputPortsBuffer.getReadPointer(i), sizeof(float) * numSamples);
        }
    }

    /*jassert(totalNumOutputChannels <= mInputPortsBuffer.getNumChannels());
    decltype(mConnections)::Iterator connection{ mConnections };
    while (connection.next()) {
        auto * source{ connection.getKey() };
        auto const * destination{ connection.getValue() };
        if (destination->physicalPort.has_value()) {
            auto const sourceIndex{ mOutputPorts.indexOf(source) };
            auto const destinationIndex{ *destination->physicalPort };
            std::memcpy(outputChannelData[destinationIndex],
                        mInputPortsBuffer.getReadPointer(sourceIndex),
                        sizeof(float) * numSamples);
        }
    }*/
}

//==============================================================================
void AudioManager::setBufferSizes(int const numSamples)
{
    juce::ScopedLock sl{ mCriticalSection };

    mInputPortsBuffer.setSize(mVirtualInputPorts.size(), numSamples, false, false, false);
    mOutputPortsBuffer.setSize(mVirtualOutputPorts.size(), numSamples, false, false, false);
}

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

    auto const * type{ (newType == PortType::input ? "input" : "output") };
    if (newPhysicalPort.has_value()) {
        std::cout << "Registered physical " << type << "port : id=" << newPort->id
                  << " physicalChannel=" << *newPhysicalPort << '\n';
    } else {
        std::cout << "Registered virtual " << type << " port : id=" << newPort->id << '\n';
    }

    jack_port_t * result{};

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
void AudioManager::unregisterPort(jack_port_t * port)
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
bool AudioManager::isConnectedTo(jack_port_t const * port, char const * port_name) const
{
    juce::ScopedLock sl{ mCriticalSection };

    if (!mConnections.contains(const_cast<jack_port_t *>(port))) {
        return false;
    }

    return strcmp(port_name, mConnections[const_cast<jack_port_t *>(port)]->fullName) == 0;
}

//==============================================================================
jack_port_t * AudioManager::findPortByName(char const * name) const
{
    juce::ScopedLock sl{ mCriticalSection };

    auto const find = [=](juce::OwnedArray<jack_port_t> const & portArray) -> jack_port_t * {
        for (auto * port : portArray) {
            if (strcmp(port->fullName, name) == 0) {
                return port;
            }
        }
        return nullptr;
    };

    jack_port_t * found;
    found = find(mVirtualInputPorts);
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
    return found;
}

//==============================================================================
void AudioManager::registerProcessCallback(JackProcessCallback const callback, void * arg)
{
    juce::ScopedLock sl{ mCriticalSection };

    mProcessCallback = callback;
    mProcessCallbackArg = arg;
}

//==============================================================================
juce::Array<jack_port_t *> AudioManager::getInputPorts() const
{
    juce::Array<jack_port_t *> result{};
    result.addArray(mPhysicalInputPorts);
    result.addArray(mVirtualInputPorts);
    return result;
}

//==============================================================================
juce::Array<jack_port_t *> AudioManager::getOutputPorts() const
{
    juce::Array<jack_port_t *> result{};
    result.addArray(mPhysicalOutputPorts);
    result.addArray(mVirtualOutputPorts);
    return result;
}

//==============================================================================
void * AudioManager::getBuffer(jack_port_t * port, [[maybe_unused]] jack_nframes_t const nFrames)
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
std::optional<jack_port_t *> AudioManager::getPort(char const * name) const
{
    juce::ScopedLock sl{ mCriticalSection };

    auto * found{ findPortByName(name) };
    if (found) {
        return found;
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

    jassert(sourcePort->type == PortType::output);
    jassert(destinationPort->type == PortType::input);

    jassert(!mConnections.contains(sourcePort));
    jassert(sourcePort->type == PortType::output);
    jassert(destinationPort->type == PortType::input);

    mConnections.set(sourcePort, destinationPort);

    mPortConnectCallback(sourcePort->id, destinationPort->id, 1, &mDummyJackClient);
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
void AudioManager::audioDeviceError(const juce::String & errorMessage)
{
    jassertfalse;
}

//==============================================================================
void AudioManager::audioDeviceAboutToStart(juce::AudioIODevice * device)
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

#endif
