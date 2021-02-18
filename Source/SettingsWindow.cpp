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

#include "SettingsWindow.h"

#include "AudioManager.h"
#include "GrisLookAndFeel.h"
#include "MainComponent.h"
#include "constants.hpp"

//==============================================================================
SettingsComponent::SettingsComponent(MainContentComponent & parent,
                                     GrisLookAndFeel & lookAndFeel,
                                     RecordingFormat const recordingFormat,
                                     RecordingConfig const recordingConfig,
                                     int const attenuationDbIndex,
                                     int const attenuationCutoffIndex,
                                     int const oscPort)
    : mMainContentComponent(parent)
    , mLookAndFeel(lookAndFeel)
{
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

    auto initComboBox = [this](juce::ComboBox & combo, juce::StringArray choices = {}, int selectedIndex = 0) {
        combo.addItemList(std::move(choices), 1);
        combo.setSelectedItemIndex(selectedIndex, juce::dontSendNotification);
        combo.setBounds(0, 0, RIGHT_COL_WIDTH, COMPONENT_HEIGHT);
        combo.setLookAndFeel(&mLookAndFeel);
        combo.addListener(this);

        addAndMakeVisible(combo);
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
    initSectionLabel(mGeneralSectionLabel);

    initLabel(mOscInputPortLabel);
    mOscInputPortTextEditor.setTooltip("Port Socket OSC Input");
    mOscInputPortTextEditor.setTextToShowWhenEmpty("", mLookAndFeel.getOffColour());
    mOscInputPortTextEditor.setColour(juce::ToggleButton::textColourId, mLookAndFeel.getFontColour());
    mOscInputPortTextEditor.setLookAndFeel(&mLookAndFeel);
    mOscInputPortTextEditor.setBounds(0, 0, RIGHT_COL_WIDTH, COMPONENT_HEIGHT);
    mOscInputPortTextEditor.setInputRestrictions(5, "0123456789");
    mOscInputPortTextEditor.setText(juce::String{ oscPort });
    addAndMakeVisible(mOscInputPortTextEditor);

    initLabel(mRecFormatLabel);
    initComboBox(mRecFormatCombo, RECORDING_FORMAT_STRINGS, static_cast<int>(recordingFormat));

    initLabel(mRecFileConfigLabel);
    initComboBox(mRecFileConfigCombo, RECORDING_CONFIG_STRINGS, static_cast<int>(recordingConfig));

    //==============================================================================
    initSectionLabel(mCubeSectionLabel);

    initLabel(mDistanceDbLabel);
    initComboBox(mDistanceDbCombo, ATTENUATION_DB_STRINGS, attenuationDbIndex);

    initLabel(mDistanceCutoffLabel);
    initComboBox(mDistanceCutoffCombo, ATTENUATION_FREQUENCY_STRINGS, attenuationCutoffIndex);

    //==============================================================================
    mSaveSettingsButton.setButtonText("Save");
    mSaveSettingsButton.setBounds(0, 0, RIGHT_COL_WIDTH / 2, COMPONENT_HEIGHT);
    mSaveSettingsButton.addListener(this);
    mSaveSettingsButton.setColour(juce::ToggleButton::textColourId, mLookAndFeel.getFontColour());
    mSaveSettingsButton.setLookAndFeel(&mLookAndFeel);
    addAndMakeVisible(mSaveSettingsButton);

    fillComboBoxes();
    placeComponents();
}

//==============================================================================
void SettingsWindow::closeButtonPressed()
{
    mMainContentComponent.closePropertiesWindow();
}

//==============================================================================
void SettingsComponent::buttonClicked(juce::Button * button)
{
    if (button == &mSaveSettingsButton) {
        mMainContentComponent.saveProperties(mDeviceTypeCombo.getText(),
                                             mInputDeviceCombo.getText(),
                                             mOutputDeviceCombo.getText(),
                                             mSampleRateCombo.getText().getIntValue(),
                                             mBufferSizeCombo.getText().getIntValue(),
                                             static_cast<RecordingFormat>(mRecFormatCombo.getSelectedItemIndex()),
                                             static_cast<RecordingConfig>(mRecFileConfigCombo.getSelectedItemIndex()),
                                             mDistanceDbCombo.getSelectedItemIndex(),
                                             mDistanceCutoffCombo.getSelectedItemIndex(),
                                             mOscInputPortTextEditor.getTextValue().toString().getIntValue());
        mMainContentComponent.closePropertiesWindow();
    }
}

//==============================================================================
void SettingsComponent::placeComponents()
{
    auto yPosition = PADDING;

    auto const skip = [&yPosition]() { yPosition += LINE_SKIP; };
    auto const skipSection = [&yPosition]() { yPosition += SECTION_SKIP; };

    //==============================================================================
    mAudioSectionLabel.setTopLeftPosition(LEFT_COL_START, yPosition);
    skip();

    mDeviceTypeLabel.setTopLeftPosition(LEFT_COL_START, yPosition);
    mDeviceTypeCombo.setTopLeftPosition(RIGHT_COL_START, yPosition);
    skip();

    mInputDeviceLabel.setTopLeftPosition(LEFT_COL_START, yPosition);
    mInputDeviceCombo.setTopLeftPosition(RIGHT_COL_START, yPosition);
    skip();

    mOutputDeviceLabel.setTopLeftPosition(LEFT_COL_START, yPosition);
    mOutputDeviceCombo.setTopLeftPosition(RIGHT_COL_START, yPosition);
    skip();

    mSampleRateLabel.setTopLeftPosition(LEFT_COL_START, yPosition);
    mSampleRateCombo.setTopLeftPosition(RIGHT_COL_START, yPosition);
    skip();

    mBufferSize.setTopLeftPosition(LEFT_COL_START, yPosition);
    mBufferSizeCombo.setTopLeftPosition(RIGHT_COL_START, yPosition);
    skipSection();

    //==============================================================================
    mGeneralSectionLabel.setTopLeftPosition(LEFT_COL_START, yPosition);
    skip();

    mOscInputPortLabel.setTopLeftPosition(LEFT_COL_START, yPosition);
    mOscInputPortTextEditor.setTopLeftPosition(RIGHT_COL_START, yPosition);
    skip();

    mRecFormatLabel.setTopLeftPosition(LEFT_COL_START, yPosition);
    mRecFormatCombo.setTopLeftPosition(RIGHT_COL_START, yPosition);
    skip();

    mRecFileConfigLabel.setTopLeftPosition(LEFT_COL_START, yPosition);
    mRecFileConfigCombo.setTopLeftPosition(RIGHT_COL_START, yPosition);
    skipSection();

    //==============================================================================
    mCubeSectionLabel.setTopLeftPosition(LEFT_COL_START, yPosition);
    skip();

    mDistanceDbLabel.setTopLeftPosition(LEFT_COL_START, yPosition);
    mDistanceDbCombo.setTopLeftPosition(RIGHT_COL_START, yPosition);
    skip();

    mDistanceCutoffLabel.setTopLeftPosition(LEFT_COL_START, yPosition);
    mDistanceCutoffCombo.setTopLeftPosition(RIGHT_COL_START, yPosition);
    skipSection();

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
    if (audioDevice) {
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

        auto const sampleRates{ audioDevice->getAvailableSampleRates() };
        auto const currentSampleRate{ audioDevice->getCurrentSampleRate() };
        auto const sampleRateIndex{ sampleRates.indexOf(currentSampleRate) };

        mSampleRateCombo.addItemList(TO_STRING_ARRAY(sampleRates), 1);
        mSampleRateCombo.setSelectedItemIndex(sampleRateIndex, juce::dontSendNotification);

        auto const bufferSizes{ audioDevice->getAvailableBufferSizes() };
        auto const currentBufferSize{ audioDevice->getCurrentBufferSizeSamples() };
        auto const bufferSizeIndex{ bufferSizes.indexOf(currentBufferSize) };

        mBufferSizeCombo.addItemList(TO_STRING_ARRAY(bufferSizes), 1);
        mBufferSizeCombo.setSelectedItemIndex(bufferSizeIndex, juce::dontSendNotification);
    }
}

//==============================================================================
void SettingsComponent::comboBoxChanged(juce::ComboBox * comboBoxThatHasChanged)
{
    auto & audioDeviceManager{ AudioManager::getInstance().getAudioDeviceManager() };
    auto setup{ audioDeviceManager.getAudioDeviceSetup() };
    auto const hasSeparateInputsAndOutputs{
        audioDeviceManager.getCurrentDeviceTypeObject()->hasSeparateInputsAndOutputs()
    };

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
    } else if (comboBoxThatHasChanged == &mRecFormatCombo) {
    } else if (comboBoxThatHasChanged == &mRecFileConfigCombo) {
    } else if (comboBoxThatHasChanged == &mDistanceDbCombo) {
    } else if (comboBoxThatHasChanged == &mDistanceCutoffCombo) {
    } else {
        jassertfalse;
    }
    fillComboBoxes();
}

//==============================================================================
SettingsWindow::SettingsWindow(MainContentComponent & parent,
                               GrisLookAndFeel & grisLookAndFeel,
                               RecordingFormat const recordingFormat,
                               RecordingConfig const recordingConfig,
                               int const attenuationDbIndex,
                               int const attenuationFrequencyIndex,
                               int const oscPort)
    : juce::DocumentWindow("Settings", grisLookAndFeel.getBackgroundColour(), DocumentWindow::allButtons)
    , mMainContentComponent(parent)
    , mPropertiesComponent(parent,
                           grisLookAndFeel,
                           recordingFormat,
                           recordingConfig,
                           attenuationDbIndex,
                           attenuationFrequencyIndex,
                           oscPort)
{
    setContentNonOwned(&mPropertiesComponent, true);
    setResizable(false, false);
    setUsingNativeTitleBar(true);
    centreWithSize(getWidth(), getHeight());
    SettingsWindow::setVisible(true);
}