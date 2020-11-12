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

#include "Application.h"

#include "AudioManager.h"

//==============================================================================
class DeviceTypeChooser final
    : public juce::Component
    , public juce::Button::Listener
{
public:
    using Callback = std::function<void(std::optional<juce::String>)>;

private:
    //==============================================================================
    juce::ComboBox mMenu{};
    juce::TextButton mCloseButton{ "OK" };
    Callback mCallback;

public:
    //==============================================================================
    DeviceTypeChooser(juce::StringArray const & devices, Callback callback) : mCallback(std::move(callback))
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

    void userTriedToCloseWindow() override
    {
        setVisible(false);
        mCallback(mMenu.getText());
        if (juce::DialogWindow * dw = findParentComponentOfClass<juce::DialogWindow>()) {
            dw->exitModalState(1234);
        }
    }

    void buttonClicked(juce::Button *) override { userTriedToCloseWindow(); }
};

//==============================================================================
class DeviceChooser final
    : public juce::Component
    , public juce::Button::Listener
{
public:
    //==============================================================================
    using Callback = std::function<void(juce::String const &, juce::String const &, std::optional<juce::String>)>;

private:
    //==============================================================================
    juce::Label mInputLabel{ "", "Input device" };
    juce::Label mOutputLabel{ "", "Output device" };

    juce::ComboBox mInputMenu{};
    juce::ComboBox mOutputMenu{};

    juce::TextButton mCloseButton{ "OK" };
    std::optional<juce::String> mDeviceType;
    Callback mCallback;

public:
    DeviceChooser(juce::StringArray const & inputDevices,
                  juce::StringArray const & outputDevices,
                  std::optional<juce::String> deviceType,
                  Callback callback)
        : mDeviceType(std::move(deviceType))
        , mCallback(std::move(callback))
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

    void userTriedToCloseWindow() override
    {
        setVisible(false);
        mCallback(mInputMenu.getText(), mOutputMenu.getText(), mDeviceType);
        if (juce::DialogWindow * dw = findParentComponentOfClass<juce::DialogWindow>()) {
            dw->exitModalState(1234);
        }
    }

    void buttonClicked(juce::Button *) override { userTriedToCloseWindow(); }
};

//==============================================================================
void SpatGris2Application::initialise(const juce::String &)
{
    juce::LookAndFeel::setDefaultLookAndFeel(&mGrisFeel);
#if defined(WIN32) || defined(__unix__)
    chooseDeviceType();
#elif defined(__APPLE__)
    chooseDevice(std::nullopt);
#else
    static_assert(false, "What are you building this on?");
#endif
}

//==============================================================================
void SpatGris2Application::start()
{
    mMainWindow.reset(new MainWindow(getApplicationName(), mGrisFeel));
}

//==============================================================================
void SpatGris2Application::chooseDeviceType()
{
    juce::AudioDeviceManager audioDeviceManager{};
    juce::StringArray deviceTypes{};
    for (auto const * deviceType : audioDeviceManager.getAvailableDeviceTypes()) {
        deviceTypes.add(deviceType->getTypeName());
    }

    auto callback = [=](std::optional<juce::String> deviceType) { chooseDevice(std::move(deviceType)); };

    juce::DialogWindow::LaunchOptions options{};
    options.content.set(new DeviceTypeChooser{ deviceTypes, callback }, true);
    options.dialogTitle = "Choose audio Driver";
    options.escapeKeyTriggersCloseButton = true;
    options.useNativeTitleBar = true;
    options.launchAsync();
}

//==============================================================================
void SpatGris2Application::chooseDevice(std::optional<juce::String> deviceType)
{
    juce::AudioDeviceManager audioDeviceManager{};
    if (deviceType) {
        audioDeviceManager.getAvailableDeviceTypes();
        audioDeviceManager.setCurrentAudioDeviceType(*deviceType, true);
    }
    auto * deviceTypeObject{ audioDeviceManager.getCurrentDeviceTypeObject() };
    jassert(deviceTypeObject);
    deviceTypeObject->scanForDevices();
    auto const inputDevices{ deviceTypeObject->getDeviceNames(true) };
    auto const outputDevices{ deviceTypeObject->getDeviceNames(false) };

    auto callback = [=](juce::String const & inputDevice,
                        juce::String const & outputDevice,
                        std::optional<juce::String> deviceType) {
        AudioManager::init(inputDevice, outputDevice, std::move(deviceType));
        start();
    };

    juce::DialogWindow::LaunchOptions options{};
    options.content.set(new DeviceChooser{ inputDevices, outputDevices, std::move(deviceType), callback }, true);
    options.dialogTitle = "Choose audio Driver";
    options.escapeKeyTriggersCloseButton = true;
    options.useNativeTitleBar = true;
    options.launchAsync();
}

//==============================================================================
void SpatGris2Application::shutdown()
{
    mMainWindow.reset();
    AudioManager::free();
}

//==============================================================================
void SpatGris2Application::systemRequestedQuit()
{
    if (mMainWindow->exitWinApp()) {
        quit();
    }
}
