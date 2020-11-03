/*
 This file is part of SpatGRIS2.

 Developers: Samuel Béland, Olivier Bélanger, Nicolas Masson

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

#if !USE_JACK

    #include <iostream>

//==============================================================================
class DeviceTypeChooser final
    : public juce::Component
    , public juce::Button::Listener
{
    juce::String & mChosenDeviceType;
    juce::ComboBox mMenu{};
    juce::TextButton mCloseButton{ "OK" };

public:
    DeviceTypeChooser(juce::StringArray const & devices, juce::String & chosenDeviceType)
        : mChosenDeviceType(chosenDeviceType)
    {
        mMenu.addItemList(devices, 1);
        mMenu.setSelectedItemIndex(0);

        mCloseButton.addListener(this);

        mMenu.setBounds(0, 0, 300, 40);
        mCloseButton.setBounds(0, 40, 300, 40);

        addAndMakeVisible(mMenu);
        addAndMakeVisible(mCloseButton);

        setSize(300, 80);
    }
    ~DeviceTypeChooser() override = default;

    void buttonClicked(juce::Button *) override
    {
        mChosenDeviceType = mMenu.getText();
        if (auto * dialogWindow = findParentComponentOfClass<juce::DialogWindow>()) {
            dialogWindow->exitModalState(1234);
        }
    }
};

//==============================================================================
class DeviceChooser final
    : public juce::Component
    , public juce::Button::Listener
{
    juce::String & mChosenInputDevice;
    juce::String & mChosenOutputDevice;

    juce::Label mInputLabel{ "", "Input device" };
    juce::Label mOutputLabel{ "", "Output device" };

    juce::ComboBox mInputMenu{};
    juce::ComboBox mOutputMenu{};

    juce::TextButton mCloseButton{ "OK" };

public:
    DeviceChooser(juce::StringArray const & inputDevices,
                  juce::StringArray const & outputDevices,
                  juce::String & chosenInputDevice,
                  juce::String & chosenOutputDevice)
        : mChosenInputDevice(chosenInputDevice)
        , mChosenOutputDevice(chosenOutputDevice)
    {
        mInputMenu.addItemList(inputDevices, 1);
        mInputMenu.setSelectedItemIndex(0);

        mOutputMenu.addItemList(outputDevices, 1);
        mOutputMenu.setSelectedItemIndex(0);

        mCloseButton.addListener(this);

        mInputLabel.setBounds(0, 0, 150, 40);
        mInputMenu.setBounds(150, 0, 150, 40);

        mOutputLabel.setBounds(0, 40, 150, 40);
        mOutputMenu.setBounds(150, 40, 150, 40);

        mCloseButton.setBounds(0, 80, 300, 40);

        addAndMakeVisible(mInputLabel);
        addAndMakeVisible(mInputMenu);
        addAndMakeVisible(mOutputLabel);
        addAndMakeVisible(mOutputMenu);
        addAndMakeVisible(mCloseButton);

        setSize(300, 120);
    }
    ~DeviceChooser() override = default;

    void buttonClicked(juce::Button *) override
    {
        mChosenInputDevice = mInputMenu.getText();
        mChosenOutputDevice = mOutputMenu.getText();
        if (auto * dialogWindow = findParentComponentOfClass<juce::DialogWindow>()) {
            dialogWindow->exitModalState(1234);
        }
    }
};

//==============================================================================
AudioManager::AudioManager()
{
    // choose device type
    #if defined(WIN32) || defined(__linux__)

    juce::StringArray availableDeviceTypes{};
    for (auto const * deviceType : mAudioDeviceManager.getAvailableDeviceTypes()) {
        availableDeviceTypes.add(deviceType->getTypeName());
    }
    juce::String chosenDeviceType{};

    {
        juce::DialogWindow::LaunchOptions launchOptions{};
        launchOptions.content.set(new DeviceTypeChooser{ availableDeviceTypes, chosenDeviceType }, true);
        launchOptions.dialogTitle = "Choose audio driver";
        launchOptions.runModal();
    }

    mAudioDeviceManager.setCurrentAudioDeviceType(chosenDeviceType, true);
    #endif
    auto & deviceType{ *mAudioDeviceManager.getCurrentDeviceTypeObject() };
    deviceType.scanForDevices();
    juce::StringArray const inputDevices{ deviceType.getDeviceNames(true) };
    juce::StringArray const outputDevices{ deviceType.getDeviceNames(false) };

    juce::String chosenInputDevice{};
    juce::String chosenOutputDevice{};
    juce::DialogWindow::LaunchOptions launchOptions{};
    launchOptions.content.set(new DeviceChooser{ inputDevices, outputDevices, chosenInputDevice, chosenOutputDevice },
                              true);
    launchOptions.dialogTitle = "Choose audio devices";
    launchOptions.runModal();

    juce::AudioDeviceManager::AudioDeviceSetup const setup{
        chosenOutputDevice, chosenInputDevice, 48000.0, 512, juce::BigInteger{}, true, juce::BigInteger{}, true
    };

    auto error{ mAudioDeviceManager.initialise(128, 128, nullptr, false, setup.outputDeviceName, &setup) };

    if (error.isNotEmpty()) {
        // jassertfalse;
        juce::AlertWindow::showMessageBox(juce::AlertWindow::AlertIconType::WarningIcon, "Error", error, "close");
        std::exit(-1);
    }

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

    if (mProcessCallback != nullptr) {
        mProcessCallback(numSamples, mProcessCallbackArg);
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
