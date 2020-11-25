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
    addAndMakeVisible(combo);
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

    auto initComponent = [this](juce::Component & component, int const yPosition, int const width = 100) {
        component.setBounds(10, yPosition, width, 22);
        component.setLookAndFeel(&mLookAndFeel);
        addAndMakeVisible(component);
    };
    auto initLabel = [this, &initComponent](juce::Label & label, int const yPosition, int const width = 100) {
        label.setJustificationType(juce::Justification::Flags::left);
        label.setFont(mLookAndFeel.getFont());
        label.setColour(juce::Label::textColourId, mLookAndFeel.getFontColour());
        initComponent(label, yPosition, width);
    };

    initLabel(mGeneralLabel, ypos);
    ypos += 30;

    initLabel(mOscInputPortLabel, ypos);
    mOscInputPortTextEditor.reset(createPropIntTextEditor("Port Socket OSC Input", ypos, oscPort));
    ypos += 40;

    initLabel(mJackSettingsLabel, ypos);
    ypos += 30;

    if (!devices.isEmpty()) {
        auto deviceIndex = 0;
        if (devices.contains(currentDevice)) {
            deviceIndex = devices.indexOf(currentDevice);
        }
        initLabel(mDeviceLabel, ypos);
        this->mDeviceCombo.reset(this->createPropComboBox(devices, deviceIndex, ypos));
        ypos += 30;
    }

    initLabel(mRateLabel, ypos);
    this->mRateCombo.reset(this->createPropComboBox(RATE_VALUES, indR, ypos));
    ypos += 30;

    initLabel(mBufferLabel, ypos);
    this->mBufferCombo.reset(this->createPropComboBox(BUFFER_SIZES, indB, ypos));
    ypos += 40;

    initLabel(mRecordingLabel, ypos);
    ypos += 30;

    initLabel(mRecFormatLabel, ypos);
    this->mRecFormatCombo.reset(this->createPropComboBox(FILE_FORMATS, indFF, ypos));
    ypos += 30;

    initLabel(mRecFileConfigLabel, ypos);
    this->mRecFileConfigCombo.reset(this->createPropComboBox(FILE_CONFIGS, indFC, ypos));
    ypos += 40;

    initLabel(mCubeDistanceLabel, ypos, 250);
    ypos += 30;

    initLabel(mDistanceDbLabel, ypos);
    this->mDistanceDbCombo.reset(this->createPropComboBox(ATTENUATION_DB, indAttDB, ypos));
    ypos += 30;

    initLabel(mDistanceCutoffLabel, ypos);
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