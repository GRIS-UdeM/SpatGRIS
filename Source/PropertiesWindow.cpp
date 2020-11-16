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

#include "PropertiesWindow.h"

#include "GrisLookAndFeel.h"
#include "MainComponent.h"
#include "ServerGrisConstants.h"

//==============================================================================
juce::Label * PropertiesComponent::createPropLabel(juce::String const & lab,
                                                   juce::Justification::Flags const just,
                                                   int const ypos,
                                                   int const width)
{
    juce::Label * label = new juce::Label();
    label->setText(lab, juce::NotificationType::dontSendNotification);
    label->setJustificationType(just);
    label->setBounds(10, ypos, width, 22);
    label->setFont(this->mLookAndFeel.getFont());
    label->setLookAndFeel(&this->mLookAndFeel);
    label->setColour(juce::Label::textColourId, this->mLookAndFeel.getFontColour());
    this->juce::Component::addAndMakeVisible(label);
    return label;
}

//==============================================================================
juce::TextEditor * PropertiesComponent::createPropIntTextEditor(juce::String const & tooltip, int ypos, int init)
{
    juce::TextEditor * editor = new juce::TextEditor();
    editor->setTooltip(tooltip);
    editor->setTextToShowWhenEmpty("", this->mLookAndFeel.getOffColour());
    editor->setColour(juce::ToggleButton::textColourId, this->mLookAndFeel.getFontColour());
    editor->setLookAndFeel(&this->mLookAndFeel);
    editor->setBounds(130, ypos, 120, 22);
    editor->setInputRestrictions(5, "0123456789");
    editor->setText(juce::String(init));
    /* Implemented but not yet in current Juce release. */
    // this->mOscInputPortTextEditor->setJustification(Justification::right);
    this->juce::Component::addAndMakeVisible(editor);
    return editor;
}

//==============================================================================
juce::ComboBox *
    PropertiesComponent::createPropComboBox(juce::StringArray const & choices, int const selected, int const ypos)
{
    juce::ComboBox * combo = new juce::ComboBox();
    combo->addItemList(choices, 1);
    combo->setSelectedItemIndex(selected);
    combo->setBounds(130, ypos, 120, 22);
    combo->setLookAndFeel(&this->mLookAndFeel);
    this->juce::Component::addAndMakeVisible(combo);
    return combo;
}

//==============================================================================
PropertiesComponent::PropertiesComponent(MainContentComponent & parent,
                                         GrisLookAndFeel & lookAndFeel,
                                         juce::Array<juce::String> const & devices,
                                         juce::String const & currentDevice,
                                         int const indR,
                                         int const indB,
                                         int const indFF,
                                         int const indFC,
                                         int const indAttDB,
                                         int const indAttHz,
                                         int const oscPort)
    : mMainContentComponent(parent)
    , mLookAndFeel(lookAndFeel)
{
    int ypos = 20;

    this->mGeneralLabel.reset(this->createPropLabel("General Settings", juce::Justification::left, ypos));
    ypos += 30;

    this->mOscInputPortLabel.reset(this->createPropLabel("OSC Input Port :", juce::Justification::left, ypos));
    this->mOscInputPortTextEditor.reset(this->createPropIntTextEditor("Port Socket OSC Input", ypos, oscPort));
    ypos += 40;

    this->mJackSettingsLabel.reset(this->createPropLabel("Jack Settings", juce::Justification::left, ypos));
    ypos += 30;

    if (!devices.isEmpty()) {
        int deviceIndex = 0;
        if (devices.contains(currentDevice)) {
            deviceIndex = devices.indexOf(currentDevice);
        }
        this->mDeviceLabel.reset(this->createPropLabel("Output Device :", juce::Justification::left, ypos));
        this->mDeviceCombo.reset(this->createPropComboBox(devices, deviceIndex, ypos));
        ypos += 30;
    }

    this->mRateLabel.reset(this->createPropLabel("Sampling Rate (hz) :", juce::Justification::left, ypos));
    this->mRateCombo.reset(this->createPropComboBox(RATE_VALUES, indR, ypos));
    ypos += 30;

    this->mBufferLabel.reset(this->createPropLabel("Buffer Size (spls) :", juce::Justification::left, ypos));
    this->mBufferCombo.reset(this->createPropComboBox(BUFFER_SIZES, indB, ypos));
    ypos += 40;

    this->mRecordingLabel.reset(this->createPropLabel("Recording Settings", juce::Justification::left, ypos));
    ypos += 30;

    this->mRecFormatLabel.reset(this->createPropLabel("File Format :", juce::Justification::left, ypos));
    this->mRecFormatCombo.reset(this->createPropComboBox(FILE_FORMATS, indFF, ypos));
    ypos += 30;

    this->mRecFileConfigLabel.reset(this->createPropLabel("Output Format :", juce::Justification::left, ypos));
    this->mRecFileConfigCombo.reset(this->createPropComboBox(FILE_CONFIGS, indFC, ypos));
    ypos += 40;

    this->mCubeDistanceLabel.reset(
        this->createPropLabel("CUBE Distance Settings", juce::Justification::left, ypos, 250));
    ypos += 30;

    this->mDistanceDbLabel.reset(this->createPropLabel("Attenuation (dB) :", juce::Justification::left, ypos));
    this->mDistanceDbCombo.reset(this->createPropComboBox(ATTENUATION_DB, indAttDB, ypos));
    ypos += 30;

    this->mDistanceCutoffLabel.reset(this->createPropLabel("Attenuation (Hz) :", juce::Justification::left, ypos));
    this->mDistanceCutoffCombo.reset(this->createPropComboBox(ATTENUATION_CUTOFFS, indAttHz, ypos));
    ypos += 40;

    this->mValidSettingsButton.reset(new juce::TextButton());
    this->mValidSettingsButton->setButtonText("Save");
    this->mValidSettingsButton->setBounds(163, ypos, 88, 22);
    this->mValidSettingsButton->addListener(this);
    this->mValidSettingsButton->setColour(juce::ToggleButton::textColourId, this->mLookAndFeel.getFontColour());
    this->mValidSettingsButton->setLookAndFeel(&this->mLookAndFeel);
    this->addAndMakeVisible(this->mValidSettingsButton.get());
}

//==============================================================================
void PropertiesWindow::closeButtonPressed()
{
    this->mainContentComponent.closePropertiesWindow();
}

//==============================================================================
void PropertiesComponent::buttonClicked(juce::Button * button)
{
    if (button == this->mValidSettingsButton.get()) {
        this->mMainContentComponent.saveProperties(
            this->mDeviceCombo.get() != nullptr ? this->mDeviceCombo->getText() : juce::String{},
            this->mRateCombo->getText().getIntValue(),
            this->mBufferCombo->getText().getIntValue(),
            this->mRecFormatCombo->getSelectedItemIndex(),
            this->mRecFileConfigCombo->getSelectedItemIndex(),
            this->mDistanceDbCombo->getSelectedItemIndex(),
            this->mDistanceCutoffCombo->getSelectedItemIndex(),
            this->mOscInputPortTextEditor->getTextValue().toString().getIntValue());
        this->mMainContentComponent.closePropertiesWindow();
    }
}

//==============================================================================
PropertiesWindow::PropertiesWindow(MainContentComponent & parent,
                                   GrisLookAndFeel & lookAndFeel,
                                   juce::Array<juce::String> const & devices,
                                   juce::String const & currentDevice,
                                   int indR,
                                   int indB,
                                   int indFF,
                                   int indFC,
                                   int indAttDB,
                                   int indAttHz,
                                   int oscPort)
    : juce::DocumentWindow("Properties", lookAndFeel.getBackgroundColour(), DocumentWindow::allButtons)
    , mainContentComponent(parent)
    , propertiesComponent(parent,
                          lookAndFeel,
                          devices,
                          currentDevice,
                          indR,
                          indB,
                          indFF,
                          indFC,
                          indAttDB,
                          indAttHz,
                          oscPort)
{
    this->setContentNonOwned(&this->propertiesComponent, false);
}