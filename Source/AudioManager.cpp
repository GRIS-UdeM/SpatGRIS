#include "AudioManager.h"

#if !USE_JACK

//==============================================================================
jack_port_t * AudioManager::registerPort(char const * const newShortName,
                                         char const * const newClientName,
                                         PortType const newType,
                                         std::optional<int> newPhysicalPort)
{
    auto newPort{
        std::make_unique<_jack_port>(++mLastGivePortId, newShortName, newClientName, newType, newPhysicalPort)
    };

    switch (newType) {
    case PortType::input:
        jassert(!mInputPorts.contains(newPort.get()));
        return mInputPorts.add(newPort.release());
    case PortType::output:
        jassert(!mOutputPorts.contains(newPort.get()));
        return mOutputPorts.add(newPort.release());
    }
    jassertfalse;
}

//==============================================================================
void AudioManager::unregisterPort(jack_port_t * port)
{
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
    if (!mConnections.contains(port)) {
        return false;
    }

    return strcmp(port->fullName, mConnections[port]->fullName) == 0;
}

//==============================================================================
jack_port_t * AudioManager::findPortByName(char const * name) const
{
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
std::optional<jack_port_t *> AudioManager::getPort(char const * name) const
{
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
    auto const sourcePort{ getPort(sourcePortName) };
    auto const destinationPort{ getPort(destinationPortName) };

    jassert(sourcePort);
    jassert(destinationPort);

    jassert(!mConnections.contains(*sourcePort));

    mConnections.set(*sourcePort, *destinationPort);

    mPortConnectCallback((*sourcePort)->id, (*destinationPort)->id, 0, nullptr);
}

//==============================================================================
void AudioManager::disconnect(jack_port_t * source, jack_port_t * destination)
{
    jassert(mConnections.contains(source));
    jassert(mConnections[source] == destination);

    mConnections.remove(source);

    mPortConnectCallback(source->id, destination->id, 0, nullptr);
}

//==============================================================================
AudioManager & AudioManager::getInstance()
{
    static AudioManager instance{};
    return instance;
}

//==============================================================================
AudioManager::AudioManager()
{
    // TODO: magic numbers
    auto error{ mAudioDeviceManager.initialiseWithDefaultDevices(256, 256) };
    jassert(error.isEmpty());

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
}

#endif
