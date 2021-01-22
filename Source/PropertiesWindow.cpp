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
#include "constants.hpp"

//==============================================================================
juce::TextEditor *
    PropertiesComponent::createPropIntTextEditor(juce::String const & tooltip, int const yPosition, int const init)
{
    auto * editor{ new juce::TextEditor{} };

    editor->setTooltip(tooltip);
    editor->setTextToShowWhenEmpty("", mLookAndFeel.getOffColour());
    editor->setColour(juce::ToggleButton::textColourId, mLookAndFeel.getFontColour());
    editor->setLookAndFeel(&mLookAndFeel);
    editor->setBounds(130, yPosition, 120, 22);
    editor->setInputRestrictions(5, "0123456789");
    editor->setText(juce::String{ init });

    addAndMakeVisible(editor);
    return editor;
}

//==============================================================================
juce::ComboBox *
    PropertiesComponent::createPropComboBox(juce::StringArray const & choices, int const selected, int const yPosition)
{
    auto * combo{ new juce::ComboBox{} };

    combo->addItemList(choices, 1);
    combo->setSelectedItemIndex(selected);
    combo->setBounds(130, yPosition, 120, 22);
    combo->setLookAndFeel(&mLookAndFeel);

    addAndMakeVisible(combo);
    return combo;
}

//==============================================================================
PropertiesComponent::PropertiesComponent(MainContentComponent & parent,
                                         GrisLookAndFeel & lookAndFeel,
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
    auto yPosition = 20;

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

    initLabel(mGeneralLabel, yPosition);
    yPosition += 30;

    initLabel(mOscInputPortLabel, yPosition);
    mOscInputPortTextEditor.reset(createPropIntTextEditor("Port Socket OSC Input", yPosition, oscPort));
    yPosition += 40;

    initLabel(mJackSettingsLabel, yPosition);
    yPosition += 30;

    initLabel(mRateLabel, yPosition);
    mRateCombo.reset(createPropComboBox(RATE_VALUES, indR, yPosition));
    yPosition += 30;

    initLabel(mBufferLabel, yPosition);
    mBufferCombo.reset(createPropComboBox(BUFFER_SIZES, indB, yPosition));
    yPosition += 40;

    initLabel(mRecordingLabel, yPosition);
    yPosition += 30;

    initLabel(mRecFormatLabel, yPosition);
    mRecFormatCombo.reset(createPropComboBox(FILE_FORMATS, indFF, yPosition));
    yPosition += 30;

    initLabel(mRecFileConfigLabel, yPosition);
    mRecFileConfigCombo.reset(createPropComboBox(FILE_CONFIGS, indFC, yPosition));
    yPosition += 40;

    initLabel(mCubeDistanceLabel, yPosition, 250);
    yPosition += 30;

    initLabel(mDistanceDbLabel, yPosition);
    mDistanceDbCombo.reset(createPropComboBox(ATTENUATION_DB, indAttDB, yPosition));
    yPosition += 30;

    initLabel(mDistanceCutoffLabel, yPosition);
    mDistanceCutoffCombo.reset(createPropComboBox(ATTENUATION_CUTOFFS, indAttHz, yPosition));
    yPosition += 40;

    mValidSettingsButton.reset(new juce::TextButton());
    mValidSettingsButton->setButtonText("Save");
    mValidSettingsButton->setBounds(163, yPosition, 88, 22);
    mValidSettingsButton->addListener(this);
    mValidSettingsButton->setColour(juce::ToggleButton::textColourId, mLookAndFeel.getFontColour());
    mValidSettingsButton->setLookAndFeel(&mLookAndFeel);
    addAndMakeVisible(mValidSettingsButton.get());
}

//==============================================================================
void PropertiesWindow::closeButtonPressed()
{
    mMainContentComponent.closePropertiesWindow();
}

//==============================================================================
void PropertiesComponent::buttonClicked(juce::Button * button)
{
    if (button == mValidSettingsButton.get()) {
        mMainContentComponent.saveProperties(mDeviceCombo != nullptr ? mDeviceCombo->getText() : juce::String{},
                                             mRateCombo->getText().getIntValue(),
                                             mBufferCombo->getText().getIntValue(),
                                             mRecFormatCombo->getSelectedItemIndex(),
                                             mRecFileConfigCombo->getSelectedItemIndex(),
                                             mDistanceDbCombo->getSelectedItemIndex(),
                                             mDistanceCutoffCombo->getSelectedItemIndex(),
                                             mOscInputPortTextEditor->getTextValue().toString().getIntValue());
        mMainContentComponent.closePropertiesWindow();
    }
}

//==============================================================================
PropertiesWindow::PropertiesWindow(MainContentComponent & parent,
                                   GrisLookAndFeel & grisLookAndFeel,
                                   juce::String const & currentDevice,
                                   int const indR,
                                   int const indB,
                                   int const indFf,
                                   int const indFc,
                                   int const indAttDb,
                                   int const indAttHz,
                                   int const oscPort)
    : juce::DocumentWindow("Properties", grisLookAndFeel.getBackgroundColour(), DocumentWindow::allButtons)
    , mMainContentComponent(parent)
    , mPropertiesComponent(parent,
                           grisLookAndFeel,
                           currentDevice,
                           indR,
                           indB,
                           indFf,
                           indFc,
                           indAttDb,
                           indAttHz,
                           oscPort)
{
    setContentNonOwned(&mPropertiesComponent, false);
}