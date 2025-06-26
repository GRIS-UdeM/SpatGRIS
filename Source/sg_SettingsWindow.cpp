/*
 This file is part of SpatGRIS.

 Developers: Gaël Lane Lépine, Samuel Béland, Olivier Bélanger, Nicolas Masson

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

#include "sg_SettingsWindow.hpp"

#include "sg_AudioManager.hpp"
#include "sg_GrisLookAndFeel.hpp"
#include "sg_MainComponent.hpp"
#include "sg_SpeakerViewComponent.hpp"


#include <bitset>

namespace gris
{
namespace
{
constexpr auto PADDING = 20;
constexpr auto LEFT_COL_WIDTH = 150;
constexpr auto RIGHT_COL_WIDTH = 150;

constexpr auto LEFT_COL_START = PADDING;
constexpr auto RIGHT_COL_START = LEFT_COL_START + LEFT_COL_WIDTH + PADDING;

constexpr auto COMPONENT_HEIGHT = 22;

constexpr auto LINE_SKIP = 30;
constexpr auto SECTION_SKIP = 50;

bool isNotPowerOfTwo(int const value)
{
    jassert(value >= 0);
    std::bitset<sizeof(value) * 8> const bitSet{ static_cast<unsigned>(value) };
    return bitSet.count() != 1;
}

} // namespace

//==============================================================================
SettingsComponent::SettingsComponent(MainContentComponent & parent, SpeakerViewComponent & sVComponent, GrisLookAndFeel & lookAndFeel)
    : mMainContentComponent(parent)
    , mSVComponent(sVComponent)
    , mLookAndFeel(lookAndFeel)
{

    mInitialOSCPort = parent.getOscPort();
    mInitialUDPInputPort = mSVComponent.getUDPInputPort();
    mInitialUDPOutputPort = mSVComponent.mUDPOutputPort;
    mInitialUDPOutputAddress = mSVComponent.mUDPOutputAddress;

    auto initLabel = [this](juce::Label & label) {
        label.setJustificationType(juce::Justification::Flags::centredRight);
        label.setFont(mLookAndFeel.getFont());
        label.setColour(juce::Label::textColourId, mLookAndFeel.getFontColour());
        label.setBounds(0, 0, LEFT_COL_WIDTH, COMPONENT_HEIGHT);
        label.setLookAndFeel(&mLookAndFeel);
        addAndMakeVisible(label);
    };

    auto initSectionLabel = [this, &initLabel](juce::Label & label) {
        initLabel(label);
        label.setJustificationType(juce::Justification::Flags::centredLeft);
        label.setFont(mLookAndFeel.getFont().withHorizontalScale(1.5f));
    };

    auto initComboBox
        = [this](juce::ComboBox & combo, juce::StringArray const & choices = {}, int const selectedIndex = 0) {
              combo.addItemList(choices, 1);
              combo.setSelectedItemIndex(selectedIndex, juce::dontSendNotification);
              combo.setBounds(0, 0, RIGHT_COL_WIDTH, COMPONENT_HEIGHT);
              combo.setLookAndFeel(&mLookAndFeel);
              combo.addListener(this);

              addAndMakeVisible(combo);
          };

    auto initTextEditor
        = [this](juce::TextEditor & edit, juce::StringRef tooltip, juce::StringRef default_val) {
          edit.setTooltip(tooltip);
          edit.setTextToShowWhenEmpty("", mLookAndFeel.getOffColour());
          edit.setColour(juce::ToggleButton::textColourId, mLookAndFeel.getFontColour());
          edit.setLookAndFeel(&mLookAndFeel);
          edit.setBounds(0, 0, RIGHT_COL_WIDTH, COMPONENT_HEIGHT);
          edit.setText(juce::String{ default_val });
          edit.addListener(this);
          addAndMakeVisible(edit);
        };

    auto & audioManager{ AudioManager::getInstance() };

    //==============================================================================
    initSectionLabel(mAudioSectionLabel);

    initLabel(mDeviceTypeLabel);
    initComboBox(mDeviceTypeCombo, audioManager.getAvailableDeviceTypeNames());

    initLabel(mInputDeviceLabel);
    initComboBox(mInputDeviceCombo);

    initLabel(mOutputDeviceLabel);
    initComboBox(mOutputDeviceCombo);

    initLabel(mSampleRateLabel);
    initComboBox(mSampleRateCombo);

    initLabel(mBufferSize);
    initComboBox(mBufferSizeCombo);

    //==============================================================================
    initSectionLabel(mSpatNetworkSettings);

    initLabel(mOscInputPortLabel);
    initTextEditor(mOscInputPortTextEditor, "Port Socket OSC Input", juce::String{mInitialOSCPort});
    mOscInputPortTextEditor.setInputRestrictions(5, "0123456789");

    initSectionLabel(mSpeakerViewNetworkSettings);

    initLabel(mSpeakerViewInputPortLabel);
    initTextEditor(mSpeakerViewInputPortTextEditor, "SpeakerView Data Input port", juce::String{mInitialUDPInputPort});
    mSpeakerViewInputPortTextEditor.setInputRestrictions(5, "0123456789");

    initLabel(mSpeakerViewOutputAddressLabel);
    initTextEditor(mSpeakerViewOutputAddressTextEditor, "SpeakerViewData Output Address", mInitialUDPOutputAddress);
    // we want an ip address here
    mSpeakerViewOutputAddressTextEditor.setInputRestrictions(15, "0123456789.");

    initLabel(mSpeakerViewOutputPortLabel);
    initTextEditor(mSpeakerViewOutputPortTextEditor, "SpeakerViewData Output Port", juce::String{mInitialUDPOutputPort});
    mSpeakerViewOutputPortTextEditor.setInputRestrictions(5, "0123456789");

    //==============================================================================
    mSaveSettingsButton.setButtonText("Apply");
    mSaveSettingsButton.setBounds(0, 0, RIGHT_COL_WIDTH / 2, COMPONENT_HEIGHT);
    mSaveSettingsButton.addListener(this);
    mSaveSettingsButton.setColour(juce::ToggleButton::textColourId, mLookAndFeel.getFontColour());
    mSaveSettingsButton.setLookAndFeel(&mLookAndFeel);
    addAndMakeVisible(mSaveSettingsButton);

    fillComboBoxes();
    placeComponents();
}

//==============================================================================
SettingsComponent::~SettingsComponent()
{
    auto const newOscPort{ mOscInputPortTextEditor.getText().getIntValue() };
    if (newOscPort != mInitialOSCPort) {
        mMainContentComponent.setOscPort(newOscPort);
    }
    auto const newUDPInputPort{ mSpeakerViewInputPortTextEditor.getText().getIntValue() };
    if (newUDPInputPort != mInitialUDPInputPort) {
        if (mSVComponent.setUDPInputPort(newUDPInputPort)) {
            juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::AlertIconType::InfoIcon,
                                                   "Could not change SpeakerView input port",
                                                   "Could not change the SpeakerView input port to "+ juce::String(newUDPInputPort) + " . Some other application may have the same port open ?\n",
                                                   "Ok",
                                                   &mMainContentComponent);
        }
    }
    auto const newUDPOutputPort{ mSpeakerViewOutputPortTextEditor.getText().getIntValue() };
    if (newUDPOutputPort != mInitialUDPOutputPort) {
        mSVComponent.mUDPOutputPort = newUDPOutputPort;
    }
    auto const newUDPOutputAddress{ mSpeakerViewOutputAddressTextEditor.getText()};
    if (newUDPOutputAddress != mInitialUDPOutputAddress) {
        mSVComponent.mUDPOutputAddress = newUDPOutputAddress;
    }
}

//==============================================================================
void SettingsWindow::closeButtonPressed()
{
    mMainContentComponent.closePropertiesWindow();
}

//==============================================================================
bool SettingsWindow::keyPressed(const juce::KeyPress & key)
{
    auto const key_w{ juce::KeyPress(87) };
    if (key.getModifiers().isCommandDown() && key.isKeyCurrentlyDown(key_w.getKeyCode())) {
        mMainContentComponent.closePropertiesWindow();
        return true;
    }
    return false;
}

//==============================================================================
void SettingsComponent::buttonClicked(juce::Button * button)
{
    if (button == &mSaveSettingsButton) {
        if (isSelectedAudioDeviceActive()) {
            mMainContentComponent.closePropertiesWindow();
        }
    }
}

//==============================================================================
void SettingsComponent::placeComponents()
{
    auto yPosition = PADDING;

    auto const addLineGap = [&yPosition]() { yPosition += LINE_SKIP; };
    auto const addSectionGap = [&yPosition]() { yPosition += SECTION_SKIP; };

    //==============================================================================
    mAudioSectionLabel.setTopLeftPosition(LEFT_COL_START, yPosition);
    addLineGap();

    mDeviceTypeLabel.setTopLeftPosition(LEFT_COL_START, yPosition);
    mDeviceTypeCombo.setTopLeftPosition(RIGHT_COL_START, yPosition);
    addLineGap();

    mInputDeviceLabel.setTopLeftPosition(LEFT_COL_START, yPosition);
    mInputDeviceCombo.setTopLeftPosition(RIGHT_COL_START, yPosition);
    addLineGap();

    mOutputDeviceLabel.setTopLeftPosition(LEFT_COL_START, yPosition);
    mOutputDeviceCombo.setTopLeftPosition(RIGHT_COL_START, yPosition);
    addLineGap();

    mSampleRateLabel.setTopLeftPosition(LEFT_COL_START, yPosition);
    mSampleRateCombo.setTopLeftPosition(RIGHT_COL_START, yPosition);
    addLineGap();

    mBufferSize.setTopLeftPosition(LEFT_COL_START, yPosition);
    mBufferSizeCombo.setTopLeftPosition(RIGHT_COL_START, yPosition);
    addSectionGap();

    //==============================================================================
    mSpatNetworkSettings.setTopLeftPosition(LEFT_COL_START, yPosition);
    addLineGap();

    mOscInputPortLabel.setTopLeftPosition(LEFT_COL_START, yPosition);
    mOscInputPortTextEditor.setTopLeftPosition(RIGHT_COL_START, yPosition);
    addSectionGap();

    mSpeakerViewNetworkSettings.setTopLeftPosition(LEFT_COL_START, yPosition);
    addLineGap();

    mSpeakerViewInputPortLabel.setTopLeftPosition(LEFT_COL_START, yPosition);
    mSpeakerViewInputPortTextEditor.setTopLeftPosition(RIGHT_COL_START, yPosition);
    addLineGap();

    mSpeakerViewOutputAddressLabel.setTopLeftPosition(LEFT_COL_START, yPosition);
    mSpeakerViewOutputAddressTextEditor.setTopLeftPosition(RIGHT_COL_START, yPosition);
    addLineGap();

    mSpeakerViewOutputPortLabel.setTopLeftPosition(LEFT_COL_START, yPosition);
    mSpeakerViewOutputPortTextEditor.setTopLeftPosition(RIGHT_COL_START, yPosition);
    addSectionGap();

    mSaveSettingsButton.setTopRightPosition(RIGHT_COL_START + RIGHT_COL_WIDTH, yPosition);

    //==============================================================================

    setBounds(0, 0, PADDING + LEFT_COL_WIDTH + PADDING + RIGHT_COL_WIDTH + PADDING, yPosition + PADDING);
    getTopLevelComponent()->setSize(getWidth(), getHeight());
}

//==============================================================================
void SettingsComponent::fillComboBoxes()
{
    static auto constexpr TO_STRING_ARRAY = [](auto const & coll) {
        juce::StringArray result{};
        for (auto const & it : coll) {
            result.add(juce::String{ it });
        }
        return result;
    };

    auto & audioManager{ AudioManager::getInstance() };
    auto & audioDeviceManager{ audioManager.getAudioDeviceManager() };
    auto const currentAudioDeviceSetup{ audioDeviceManager.getAudioDeviceSetup() };
    auto * currentAudioDeviceType{ audioDeviceManager.getCurrentDeviceTypeObject() };
    auto * audioDevice{ audioDeviceManager.getCurrentAudioDevice() };

    auto const audioDeviceTypes{ audioManager.getAvailableDeviceTypeNames() };
    auto const audioDeviceTypeIndex{ audioDeviceTypes.indexOf(currentAudioDeviceType->getTypeName()) };

    mDeviceTypeCombo.clear(juce::dontSendNotification);
    mDeviceTypeCombo.addItemList(audioManager.getAvailableDeviceTypeNames(), 1);
    mDeviceTypeCombo.setSelectedItemIndex(audioDeviceTypeIndex, juce::dontSendNotification);

    mInputDeviceCombo.clear(juce::dontSendNotification);
    mOutputDeviceCombo.clear(juce::dontSendNotification);
    mSampleRateCombo.clear(juce::dontSendNotification);
    mBufferSizeCombo.clear(juce::dontSendNotification);
    mInputDevices = currentAudioDeviceType->getDeviceNames(true);
    auto const & currentInputDevice{ currentAudioDeviceSetup.inputDeviceName };
    auto const inputDeviceIndex{ mInputDevices.indexOf(currentInputDevice) };

    mInputDeviceCombo.addItemList(mInputDevices, 1);
    mInputDeviceCombo.setSelectedItemIndex(inputDeviceIndex, juce::dontSendNotification);

    mOutputDevices = currentAudioDeviceType->getDeviceNames(false);
    auto const & currentOutputDevice{ currentAudioDeviceSetup.outputDeviceName };
    auto const outputDeviceIndex{ mOutputDevices.indexOf(currentOutputDevice) };

    mOutputDeviceCombo.addItemList(mOutputDevices, 1);
    mOutputDeviceCombo.setSelectedItemIndex(outputDeviceIndex, juce::dontSendNotification);

    if (audioDevice) {
        auto const sampleRates = audioDevice->getAvailableSampleRates();
        auto const currentSampleRate{ audioDevice->getCurrentSampleRate() };
        auto const sampleRateIndex{ sampleRates.indexOf(currentSampleRate) };

        mSampleRateCombo.addItemList(TO_STRING_ARRAY(sampleRates), 1);
        mSampleRateCombo.setSelectedItemIndex(sampleRateIndex, juce::dontSendNotification);

        auto bufferSizes = audioDevice->getAvailableBufferSizes();
        bufferSizes.removeIf(isNotPowerOfTwo);
        auto const currentBufferSize{ audioDevice->getCurrentBufferSizeSamples() };
        auto const bufferSizeIndex{ bufferSizes.indexOf(currentBufferSize) };

        mBufferSizeCombo.addItemList(TO_STRING_ARRAY(bufferSizes), 1);
        mBufferSizeCombo.setSelectedItemIndex(bufferSizeIndex, juce::dontSendNotification);
    }
}

//==============================================================================
bool SettingsComponent::isSelectedAudioDeviceActive()
{
    auto * currentAudioDevice{ AudioManager::getInstance().getAudioDeviceManager().getCurrentAudioDevice() };

    if (currentAudioDevice == nullptr) {
        juce::AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::WarningIcon,
                                               "Error opening audio device.",
                                               "Please select an active audio device.",
                                               "OK",
                                               this,
                                               nullptr);
        return false;
    }
    return true;
}

//==============================================================================
void SettingsComponent::comboBoxChanged(juce::ComboBox * comboBoxThatHasChanged)
{
    auto & audioDeviceManager{ AudioManager::getInstance().getAudioDeviceManager() };
    auto setup{ audioDeviceManager.getAudioDeviceSetup() };
    setup.inputChannels = NEEDED_INPUT_CHANNELS;
    setup.outputChannels = NEEDED_OUTPUT_CHANNELS;
    auto const hasSeparateInputsAndOutputs{
        audioDeviceManager.getCurrentDeviceTypeObject()->hasSeparateInputsAndOutputs()
    };

    juce::ScopedLock const lock{ mMainContentComponent.getAudioProcessor().getLock() };

    if (comboBoxThatHasChanged == &mDeviceTypeCombo) {
        audioDeviceManager.setCurrentAudioDeviceType(comboBoxThatHasChanged->getText(), true);
    } else if (comboBoxThatHasChanged == &mInputDeviceCombo) {
        auto const deviceName{ mInputDeviceCombo.getText() };
        setup.inputDeviceName = deviceName;
        if (!hasSeparateInputsAndOutputs) {
            setup.outputDeviceName = deviceName;
        }
        audioDeviceManager.setAudioDeviceSetup(setup, true);
    } else if (comboBoxThatHasChanged == &mOutputDeviceCombo) {
        auto const deviceName{ mOutputDeviceCombo.getText() };
        setup.outputDeviceName = deviceName;
        if (!hasSeparateInputsAndOutputs) {
            setup.inputDeviceName = deviceName;
        }
        audioDeviceManager.setAudioDeviceSetup(setup, true);
    } else if (comboBoxThatHasChanged == &mSampleRateCombo) {
        setup.sampleRate = mSampleRateCombo.getText().getDoubleValue();
        audioDeviceManager.setAudioDeviceSetup(setup, true);
    } else if (comboBoxThatHasChanged == &mBufferSizeCombo) {
        setup.bufferSize = mBufferSizeCombo.getText().getIntValue();
        audioDeviceManager.setAudioDeviceSetup(setup, true);
    } else {
        jassertfalse;
    }
    fillComboBoxes();

    if (isSelectedAudioDeviceActive()) {
        auto * currentAudioDevice{ AudioManager::getInstance().getAudioDeviceManager().getCurrentAudioDevice() };
        AudioManager::getInstance().reloadPlayerAudioFiles(currentAudioDevice->getCurrentBufferSizeSamples(),
                                                           currentAudioDevice->getCurrentSampleRate());
    }
}

void SettingsComponent::textEditorFocusLost(juce::TextEditor& textEditor)
{

  if (&textEditor == &mSpeakerViewOutputAddressTextEditor) {
    // Validate IP address (thanks https://forum.juce.com/t/how-to-achive-ip-address-validation-for-taxteditor/12036/6 )
    juce::IPAddress ip = juce::IPAddress(textEditor.getText());
    if (ip.toString() != textEditor.getText())
    {
      textEditor.setText(localhost);
    }
  }
  // Validate UDP ports (can't have UDP port < 1024 or > 65535)
  else if (
      &textEditor == &mSpeakerViewInputPortTextEditor ||
      &textEditor == &mSpeakerViewOutputPortTextEditor ||
      &textEditor == &mOscInputPortTextEditor) {
    if (textEditor.getText().getIntValue() < minUDPPort) {
        textEditor.setText(minUDPPortString);
    } else if (textEditor.getText().getIntValue() > maxUDPPort) {
      textEditor.setText(maxUDPPortString);
    }
  }
}

//==============================================================================
SettingsWindow::SettingsWindow(MainContentComponent & parent, SpeakerViewComponent & sVComponent, GrisLookAndFeel & grisLookAndFeel)
    : DocumentWindow("Settings", grisLookAndFeel.getBackgroundColour(), allButtons)
    , mMainContentComponent(parent)
    , mPropertiesComponent(parent, sVComponent, grisLookAndFeel)
{
    setAlwaysOnTop(true);
    setContentNonOwned(&mPropertiesComponent, true);
    setResizable(false, false);
    setUsingNativeTitleBar(true);
    centreWithSize(getWidth(), getHeight());
    SettingsWindow::setVisible(true);
}

} // namespace gris
