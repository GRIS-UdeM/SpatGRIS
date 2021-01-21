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

#include "JackClient.h"
#include "constants.hpp"

static_assert(!USE_JACK);

//==============================================================================
std::unique_ptr<AudioManager> AudioManager::mInstance{ nullptr };

//==============================================================================
AudioManager::AudioManager(juce::String const & inputDevice,
                           juce::String const & outputDevice,
                           std::optional<juce::String> deviceType,
                           double const sampleRate,
                           int const bufferSize)
{
    if (deviceType) {
        [[maybe_unused]] auto const & dummy{ mAudioDeviceManager.getAvailableDeviceTypes() };
        mAudioDeviceManager.setCurrentAudioDeviceType(*deviceType, true);
        auto * deviceTypeObject{ mAudioDeviceManager.getCurrentDeviceTypeObject() };
        jassert(deviceTypeObject);
        deviceTypeObject->scanForDevices();
    }

    jassert(RATE_VALUES.contains(juce::String{ sampleRate, 0 }));
    jassert(BUFFER_SIZES.contains(juce::String{ bufferSize }));

    juce::AudioDeviceManager::AudioDeviceSetup const setup{
        outputDevice, inputDevice, sampleRate, bufferSize, juce::BigInteger{}, true, juce::BigInteger{}, true
    };

    auto const error{ mAudioDeviceManager.initialise(128, 128, nullptr, false, setup.outputDeviceName, &setup) };

    if (error.isNotEmpty()) {
        // jassertfalse;
        juce::AlertWindow::showMessageBox(juce::AlertWindow::AlertIconType::WarningIcon, "Error", error, "close");
        std::exit(-1);
    }

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

    if (mJackClient != nullptr) {
        mJackClient->processAudio(numSamples);
    }

    // copy output ports to output channels
    auto const numOutputChannelsToCopy{ std::min(totalNumOutputChannels, mVirtualOutputPorts.size()) };
    for (int i{}; i < numOutputChannelsToCopy; ++i) {
        auto * dest{ outputChannelData[i] };
        if (dest) {
            std::memcpy(dest, mOutputPortsBuffer.getReadPointer(i), sizeof(float) * numSamples);
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
void AudioManager::init(juce::String const & inputDevice,
                        juce::String const & outputDevice,
                        std::optional<juce::String> deviceType,
                        double const sampleRate,
                        int const bufferSize)
{
    mInstance.reset(new AudioManager{ inputDevice, outputDevice, std::move(deviceType), sampleRate, bufferSize });
}

//==============================================================================
jack_port_t * AudioManager::registerPort(char const * const newShortName,
                                         char const * const newClientName,
                                         PortType const newType,
                                         std::optional<int> newPhysicalPort)
{
    juce::ScopedLock sl{ mCriticalSection };

    auto newPort{
        std::make_unique<jack_port_t>(++mLastGivePortId, newShortName, newClientName, newType, newPhysicalPort)
    };

    auto const * type{ (newType == PortType::input ? "input" : "output") };
    if (newPhysicalPort.has_value()) {
        std::cout << "Registered physical " << type << "port : id=" << newPort->id
                  << " physicalChannel=" << *newPhysicalPort << '\n';
    } else {
        std::cout << "Registered virtual " << type << " port : id=" << newPort->id << '\n';
    }

    jack_port_t * result;

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
void AudioManager::registerJackClient(JackClient * jackClient)
{
    juce::ScopedLock sl{ mCriticalSection };
    mJackClient = jackClient;
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
float * AudioManager::getBuffer(jack_port_t * port, [[maybe_unused]] size_t const nFrames)
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
jack_port_t * AudioManager::getPort(char const * name) const
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
jack_port_t * AudioManager::getPort(uint32_t const id) const
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

    auto * sourcePort{ getPort(sourcePortName) };
    auto * destinationPort{ getPort(destinationPortName) };

    jassert(sourcePort->type == PortType::output);
    jassert(destinationPort->type == PortType::input);

    jassert(!mConnections.contains(sourcePort));
    jassert(sourcePort->type == PortType::output);
    jassert(destinationPort->type == PortType::input);

    mConnections.set(sourcePort, destinationPort);
}

//==============================================================================
void AudioManager::disconnect(jack_port_t * source, jack_port_t * destination)
{
    juce::ScopedLock sl{ mCriticalSection };

    jassert(mConnections.contains(source));
    jassert(mConnections[source] == destination);

    mConnections.remove(source);
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
    jassert(mInstance);
    return *mInstance;
}

//==============================================================================
void AudioManager::free()
{
    mInstance.reset();
}
