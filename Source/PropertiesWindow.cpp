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
    label->setFont(this->grisFeel.getFont());
    label->setLookAndFeel(&this->grisFeel);
    label->setColour(juce::Label::textColourId, this->grisFeel.getFontColour());
    this->juce::Component::addAndMakeVisible(label);
    return label;
}

//==============================================================================
juce::TextEditor * PropertiesComponent::createPropIntTextEditor(juce::String const & tooltip, int ypos, int init)
{
    juce::TextEditor * editor = new juce::TextEditor();
    editor->setTooltip(tooltip);
    editor->setTextToShowWhenEmpty("", this->grisFeel.getOffColour());
    editor->setColour(juce::ToggleButton::textColourId, this->grisFeel.getFontColour());
    editor->setLookAndFeel(&this->grisFeel);
    editor->setBounds(130, ypos, 120, 22);
    editor->setInputRestrictions(5, "0123456789");
    editor->setText(juce::String(init));
    /* Implemented but not yet in current Juce release. */
    // this->tedOSCInPort->setJustification(Justification::right);
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
    combo->setLookAndFeel(&this->grisFeel);
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
    : mainContentComponent(parent)
    , grisFeel(lookAndFeel)
{
    int ypos = 20;

    this->generalLabel.reset(this->createPropLabel("General Settings", juce::Justification::left, ypos));
    ypos += 30;

    this->labOSCInPort.reset(this->createPropLabel("OSC Input Port :", juce::Justification::left, ypos));
    this->tedOSCInPort.reset(this->createPropIntTextEditor("Port Socket OSC Input", ypos, oscPort));
    ypos += 40;

    this->jackSettingsLabel.reset(this->createPropLabel("Jack Settings", juce::Justification::left, ypos));
    ypos += 30;

    if (!devices.isEmpty()) {
        int deviceIndex = 0;
        if (devices.contains(currentDevice)) {
            deviceIndex = devices.indexOf(currentDevice);
        }
        this->labDevice.reset(this->createPropLabel("Output Device :", juce::Justification::left, ypos));
        this->cobDevice.reset(this->createPropComboBox(devices, deviceIndex, ypos));
        ypos += 30;
    }

    this->labRate.reset(this->createPropLabel("Sampling Rate (hz) :", juce::Justification::left, ypos));
    this->cobRate.reset(this->createPropComboBox(RateValues, indR, ypos));
    ypos += 30;

    this->labBuff.reset(this->createPropLabel("Buffer Size (spls) :", juce::Justification::left, ypos));
    this->cobBuffer.reset(this->createPropComboBox(BufferSizes, indB, ypos));
    ypos += 40;

    this->recordingLabel.reset(this->createPropLabel("Recording Settings", juce::Justification::left, ypos));
    ypos += 30;

    this->labRecFormat.reset(this->createPropLabel("File Format :", juce::Justification::left, ypos));
    this->recordFormat.reset(this->createPropComboBox(FileFormats, indFF, ypos));
    ypos += 30;

    this->labRecFileConfig.reset(this->createPropLabel("Output Format :", juce::Justification::left, ypos));
    this->recordFileConfig.reset(this->createPropComboBox(FileConfigs, indFC, ypos));
    ypos += 40;

    this->cubeDistanceLabel.reset(
        this->createPropLabel("CUBE Distance Settings", juce::Justification::left, ypos, 250));
    ypos += 30;

    this->labDistanceDB.reset(this->createPropLabel("Attenuation (dB) :", juce::Justification::left, ypos));
    this->cobDistanceDB.reset(this->createPropComboBox(AttenuationDBs, indAttDB, ypos));
    ypos += 30;

    this->labDistanceCutoff.reset(this->createPropLabel("Attenuation (Hz) :", juce::Justification::left, ypos));
    this->cobDistanceCutoff.reset(this->createPropComboBox(AttenuationCutoffs, indAttHz, ypos));
    ypos += 40;

    this->butValidSettings.reset(new juce::TextButton());
    this->butValidSettings->setButtonText("Save");
    this->butValidSettings->setBounds(163, ypos, 88, 22);
    this->butValidSettings->addListener(this);
    this->butValidSettings->setColour(juce::ToggleButton::textColourId, this->grisFeel.getFontColour());
    this->butValidSettings->setLookAndFeel(&this->grisFeel);
    this->addAndMakeVisible(this->butValidSettings.get());
}

//==============================================================================
void PropertiesWindow::closeButtonPressed()
{
    this->mainContentComponent.closePropertiesWindow();
}

//==============================================================================
void PropertiesComponent::buttonClicked(juce::Button * button)
{
    if (button == this->butValidSettings.get()) {
        this->mainContentComponent.saveProperties(this->cobDevice.get() != nullptr ? this->cobDevice->getText()
                                                                                   : juce::String{},
                                                  this->cobRate->getText().getIntValue(),
                                                  this->cobBuffer->getText().getIntValue(),
                                                  this->recordFormat->getSelectedItemIndex(),
                                                  this->recordFileConfig->getSelectedItemIndex(),
                                                  this->cobDistanceDB->getSelectedItemIndex(),
                                                  this->cobDistanceCutoff->getSelectedItemIndex(),
                                                  this->tedOSCInPort->getTextValue().toString().getIntValue());
        this->mainContentComponent.closePropertiesWindow();
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