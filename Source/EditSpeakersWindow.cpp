/*
 This file is part of SpatGRIS2.
 
 Developers: Samuel Béland, Nicolas Masson
 
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

#include "EditSpeakersWindow.h"

#include "EditableTextCustomComponent.h"
#include "GrisLookAndFeel.h"
#include "MainComponent.h"
#include "UiComponent.h"

//==============================================================================
static double GetFloatPrecision(double value, double precision) {
    return (floor((value * pow(10, precision) + 0.5)) / pow(10, precision));
}

//==============================================================================
EditSpeakersWindow::EditSpeakersWindow( juce::String const& name,
                                        GrisLookAndFeel& lookAndFeel,
                                        MainContentComponent& mainContentComponent,
                                        juce::String const& configName )
    : juce::DocumentWindow(name, lookAndFeel.getBackgroundColour(), DocumentWindow::allButtons)
    , grisFeel(lookAndFeel)
    , mainContentComponent(mainContentComponent)
    , font(14.0f)
{
    this->boxListSpeaker.reset(new Box(&this->grisFeel, "Configuration Speakers"));

    this->butAddSpeaker.setButtonText("Add Speaker");
    this->butAddSpeaker.setBounds(5, 404, 100, 22);
    this->butAddSpeaker.addListener(this);
    this->butAddSpeaker.setColour(ToggleButton::textColourId, this->grisFeel.getFontColour());
    this->butAddSpeaker.setLookAndFeel(&this->grisFeel);
    this->boxListSpeaker->getContent()->addAndMakeVisible(this->butAddSpeaker);

    this->butcompSpeakers.setButtonText("Compute");
    this->butcompSpeakers.setBounds(110, 404, 100, 22);
    this->butcompSpeakers.addListener(this);
    this->butcompSpeakers.setColour(ToggleButton::textColourId, this->grisFeel.getFontColour());
    this->butcompSpeakers.setLookAndFeel(&this->grisFeel);
    this->boxListSpeaker->getContent()->addAndMakeVisible(this->butcompSpeakers);

    // Generate ring of speakers.
    int wlab = 80;

    this->rNumOfSpeakersLabel.setText("# of speakers", NotificationType::dontSendNotification);
    this->rNumOfSpeakersLabel.setJustificationType(Justification::right);
    this->rNumOfSpeakersLabel.setFont(this->grisFeel.getFont());
    this->rNumOfSpeakersLabel.setLookAndFeel(&this->grisFeel);
    this->rNumOfSpeakersLabel.setColour(Label::textColourId, this->grisFeel.getFontColour());
    this->rNumOfSpeakersLabel.setBounds(5, 435, 40, 24);
    this->boxListSpeaker->getContent()->addAndMakeVisible(this->rNumOfSpeakersLabel);

    this->rNumOfSpeakers.setTooltip("Number of speakers in the ring");
    this->rNumOfSpeakers.setColour(ToggleButton::textColourId, this->grisFeel.getFontColour());
    this->rNumOfSpeakers.setLookAndFeel(&this->grisFeel);
    this->rNumOfSpeakers.setBounds(5+wlab, 435, 40, 24);
    this->rNumOfSpeakers.addListener(&this->mainContentComponent);
    this->boxListSpeaker->getContent()->addAndMakeVisible(this->rNumOfSpeakers);

    this->rNumOfSpeakers.setText("8");
    this->rNumOfSpeakers.setInputRestrictions(3, "0123456789");
    this->rNumOfSpeakers.addListener(this);

    this->rZenithLabel.setText("Elevation", NotificationType::dontSendNotification);
    this->rZenithLabel.setJustificationType(Justification::right);
    this->rZenithLabel.setFont(this->grisFeel.getFont());
    this->rZenithLabel.setLookAndFeel(&this->grisFeel);
    this->rZenithLabel.setColour(Label::textColourId, this->grisFeel.getFontColour());
    this->rZenithLabel.setBounds(105, 435, 80, 24);
    this->boxListSpeaker->getContent()->addAndMakeVisible(this->rZenithLabel);

    this->rZenith.setTooltip("Elevation angle of the ring");
    this->rZenith.setColour(ToggleButton::textColourId, this->grisFeel.getFontColour());
    this->rZenith.setLookAndFeel(&this->grisFeel);
    this->rZenith.setBounds(105+wlab, 435, 60, 24);
    this->rZenith.addListener(&this->mainContentComponent);
    this->boxListSpeaker->getContent()->addAndMakeVisible(this->rZenith);

    this->rZenith.setText("0.0");
    this->rZenith.setInputRestrictions(6, "-0123456789.");
    this->rZenith.addListener(this);

    this->rRadiusLabel.setText("Distance", NotificationType::dontSendNotification);
    this->rRadiusLabel.setJustificationType(Justification::right);
    this->rRadiusLabel.setFont(this->grisFeel.getFont());
    this->rRadiusLabel.setLookAndFeel(&this->grisFeel);
    this->rRadiusLabel.setColour(Label::textColourId, this->grisFeel.getFontColour());
    this->rRadiusLabel.setBounds(230, 435, 80, 24);
    this->boxListSpeaker->getContent()->addAndMakeVisible(this->rRadiusLabel);

    this->rRadius.setTooltip("Distance of the speakers from the center.");
    this->rRadius.setColour(ToggleButton::textColourId, this->grisFeel.getFontColour());
    this->rRadius.setLookAndFeel(&this->grisFeel);
    this->rRadius.setBounds(230+wlab, 435, 60, 24);
    this->rRadius.addListener(&this->mainContentComponent);
    this->boxListSpeaker->getContent()->addAndMakeVisible(this->rRadius);

    this->rRadius.setText("1.0");
    this->rRadius.setInputRestrictions(6, "0123456789.");
    this->rRadius.addListener(this);

    this->rOffsetAngleLabel.setText("Offset Angle", NotificationType::dontSendNotification);
    this->rOffsetAngleLabel.setJustificationType(Justification::right);
    this->rOffsetAngleLabel.setFont(this->grisFeel.getFont());
    this->rOffsetAngleLabel.setLookAndFeel(&this->grisFeel);
    this->rOffsetAngleLabel.setColour(Label::textColourId, this->grisFeel.getFontColour());
    this->rOffsetAngleLabel.setBounds(375, 435, 80, 24);
    this->boxListSpeaker->getContent()->addAndMakeVisible(this->rOffsetAngleLabel);

    this->rOffsetAngle.setTooltip("Offset angle of the first speaker.");
    this->rOffsetAngle.setColour(ToggleButton::textColourId, this->grisFeel.getFontColour());
    this->rOffsetAngle.setLookAndFeel(&this->grisFeel);
    this->rOffsetAngle.setBounds(375+wlab, 435, 60, 24);
    this->rOffsetAngle.addListener(&this->mainContentComponent);
    this->boxListSpeaker->getContent()->addAndMakeVisible(this->rOffsetAngle);

    this->rOffsetAngle.setText("0.0");
    this->rOffsetAngle.setInputRestrictions(6, "-0123456789.");
    this->rOffsetAngle.addListener(this);

    this->butAddRing.setButtonText("Add Ring");
    this->butAddRing.setBounds(520, 435, 100, 24);
    this->butAddRing.addListener(this);
    this->butAddRing.setColour(ToggleButton::textColourId, this->grisFeel.getFontColour());
    this->butAddRing.setLookAndFeel(&this->grisFeel);
    this->boxListSpeaker->getContent()->addAndMakeVisible(this->butAddRing);

    // Pink noise controls.
    this->pinkNoise.setButtonText("Reference Pink Noise");
    this->pinkNoise.setBounds(5, 500, 150, 24);
    this->pinkNoise.addListener(this);
    this->pinkNoise.setColour(ToggleButton::textColourId, this->grisFeel.getFontColour());
    this->pinkNoise.setLookAndFeel(&this->grisFeel);
    this->boxListSpeaker->getContent()->addAndMakeVisible(this->pinkNoise);

    this->pinkNoiseGain.setTextValueSuffix(" dB");
    this->pinkNoiseGain.setBounds(200, 500, 60, 60);
    this->pinkNoiseGain.setSliderStyle(Slider::Rotary);
    this->pinkNoiseGain.setRotaryParameters(M_PI * 1.3f, M_PI * 2.7f, true);
    this->pinkNoiseGain.setRange(-60, -3, 1);
    this->pinkNoiseGain.setValue(-20);
    this->pinkNoiseGain.setTextBoxStyle (Slider::TextBoxBelow, false, 60, 20);
    this->pinkNoiseGain.setColour(ToggleButton::textColourId, this->grisFeel.getFontColour());
    this->pinkNoiseGain.setLookAndFeel(&this->grisFeel);
    this->pinkNoiseGain.addListener(this);
    this->boxListSpeaker->getContent()->addAndMakeVisible(this->pinkNoiseGain);

    this->boxListSpeaker->getContent()->addAndMakeVisible(this->tableListSpeakers);

    this->boxListSpeaker->repaint();
    this->boxListSpeaker->resized();

    this->setContentNonOwned(this->boxListSpeaker.get(), false);
}

//==============================================================================
void EditSpeakersWindow::initComp() {
    tableListSpeakers.setModel(this);

    tableListSpeakers.setColour(ListBox::outlineColourId, this->grisFeel.getWinBackgroundColour());
    tableListSpeakers.setColour(ListBox::backgroundColourId, this->grisFeel.getWinBackgroundColour());
    tableListSpeakers.setOutlineThickness(1);

    tableListSpeakers.getHeader().addColumn("ID", 1, 40, 40, 60, TableHeaderComponent::defaultFlags);
    tableListSpeakers.getHeader().addColumn("X", 2, 70, 50, 120, TableHeaderComponent::defaultFlags);
    tableListSpeakers.getHeader().addColumn("Y", 3, 70, 50, 120, TableHeaderComponent::defaultFlags);
    tableListSpeakers.getHeader().addColumn("Z", 4, 70, 50, 120, TableHeaderComponent::defaultFlags);
    tableListSpeakers.getHeader().addColumn("Azimuth", 5, 70, 50, 120, TableHeaderComponent::defaultFlags);
    tableListSpeakers.getHeader().addColumn("Elevation", 6, 70, 50, 120, TableHeaderComponent::defaultFlags);
    tableListSpeakers.getHeader().addColumn("Distance", 7, 70, 50, 120, TableHeaderComponent::defaultFlags);
    tableListSpeakers.getHeader().addColumn("Output", 8, 70, 50, 120, TableHeaderComponent::defaultFlags);
    tableListSpeakers.getHeader().addColumn("Gain (dB)", 9, 70, 50, 120, TableHeaderComponent::notSortable);
    tableListSpeakers.getHeader().addColumn("Highpass", 10, 70, 50, 120, TableHeaderComponent::notSortable);
    tableListSpeakers.getHeader().addColumn("Direct", 11, 70, 50, 120, TableHeaderComponent::notSortable);
    tableListSpeakers.getHeader().addColumn("delete", 12, 70, 50, 120, TableHeaderComponent::notSortable);

    tableListSpeakers.getHeader().setSortColumnId(1, true); // Sort forwards by the ID column.

    tableListSpeakers.setMultipleSelectionEnabled(true);

    numRows = (unsigned int)this->mainContentComponent.getListSpeaker().size();

    this->boxListSpeaker->setBounds(0, 0, getWidth(), getHeight());
    this->boxListSpeaker->correctSize(getWidth() - 8, getHeight());
    tableListSpeakers.setSize(getWidth(), 400);

    tableListSpeakers.updateContent();

    this->boxListSpeaker->repaint();
    this->boxListSpeaker->resized();
    this->resized();
}

//==============================================================================
struct Sorter {
    int id;
    float value;
    bool directout;
};

//==============================================================================
bool compareLessThan(Sorter const& a, Sorter const& b) {
    if (a.directout && b.directout)
        return a.id < b.id;
    else if (a.directout)
        return false;
    else if (b.directout)
        return true;
    else if (a.value == b.value)
        return a.id < b.id;
    else
        return a.value < b.value;
}

//==============================================================================
bool compareGreaterThan(Sorter const& a, Sorter const& b) {
    if (a.directout && b.directout)
        return a.id > b.id;
    else if (a.directout)
        return false;
    else if (b.directout)
        return true;
    else if (a.value == b.value)
        return a.id > b.id;
    else
        return a.value > b.value;
}

//==============================================================================
void EditSpeakersWindow::sortOrderChanged(int const newSortColumnId, bool const isForwards) {
    int size = (int)this->mainContentComponent.getListSpeaker().size();
    struct Sorter tosort[MaxOutputs];

    for (int i = 0; i < size; i++) {
        tosort[i].id = this->mainContentComponent.getListSpeaker()[i]->getIdSpeaker();
        tosort[i].directout = this->mainContentComponent.getListSpeaker()[i]->getDirectOut();
        switch (newSortColumnId) {
            case 1:
                tosort[i].value = (float)this->mainContentComponent.getListSpeaker()[i]->getIdSpeaker();
                break;
            case 2:
                tosort[i].value = (float)this->mainContentComponent.getListSpeaker()[i]->getCoordinate().x;
                break;
            case 3:
                tosort[i].value = (float)this->mainContentComponent.getListSpeaker()[i]->getCoordinate().z;
                break;
            case 4:
                tosort[i].value = (float)this->mainContentComponent.getListSpeaker()[i]->getCoordinate().y;
                break;
            case 5:
                tosort[i].value = (float)this->mainContentComponent.getListSpeaker()[i]->getAziZenRad().x;
                break;
            case 6:
                tosort[i].value = (float)this->mainContentComponent.getListSpeaker()[i]->getAziZenRad().y;
                break;
            case 7:
                tosort[i].value = (float)this->mainContentComponent.getListSpeaker()[i]->getAziZenRad().z;
                break;
            case 8:
                tosort[i].value = (float)this->mainContentComponent.getListSpeaker()[i]->getOutputPatch();
                break;
        }
    }
    if (isForwards) {
        std::sort(tosort, tosort + size, compareLessThan);
    } else {
        std::sort(tosort, tosort + size, compareGreaterThan);
    }

    vector<int> newOrder(size);
    for (int i = 0; i < size; i++) {
        newOrder[i] = tosort[i].id;
    }
    this->mainContentComponent.reorderSpeakers(newOrder);
    updateWinContent();
}

//==============================================================================
void EditSpeakersWindow::sliderValueChanged(juce::Slider *slider) {
    float gain;
    if (slider == &this->pinkNoiseGain) {
        gain = powf(10.0f, this->pinkNoiseGain.getValue() / 20.0f);
        this->mainContentComponent.getJackClient()->pinkNoiseGain = gain;
    }
}

//==============================================================================
void EditSpeakersWindow::buttonClicked(juce::Button *button) {
    bool tripletState = this->mainContentComponent.isTripletsShown;
    int selectedRow = this->tableListSpeakers.getSelectedRow();
    int sortColumnId = this->tableListSpeakers.getHeader().getSortColumnId();
    bool sortedForwards = this->tableListSpeakers.getHeader().isSortedForwards();

    this->mainContentComponent.setShowTriplets(false);

    if (button == &this->butAddSpeaker) {
        if (selectedRow == -1 || selectedRow == (this->numRows - 1)) {
            this->mainContentComponent.addSpeaker(sortColumnId, sortedForwards);
            this->updateWinContent();
            this->tableListSpeakers.selectRow(this->getNumRows() - 1);
        } else {
            this->mainContentComponent.insertSpeaker(selectedRow, sortColumnId, sortedForwards);
            this->updateWinContent();
            this->tableListSpeakers.selectRow(selectedRow + 1);
        }
        this->tableListSpeakers.getHeader().setSortColumnId(sortColumnId, sortedForwards);
        this->mainContentComponent.needToComputeVbap = true;
    } else if (button == &this->butcompSpeakers) {
        if (this->mainContentComponent.updateLevelComp()) {
            this->mainContentComponent.setShowTriplets(tripletState);
        }
    } else if (button == &this->butAddRing) {
        for (int i = 0; i < this->rNumOfSpeakers.getText().getIntValue(); i++) {
            if (selectedRow == -1 || selectedRow == (this->numRows-1)) {
                this->mainContentComponent.addSpeaker(sortColumnId, sortedForwards);
                this->numRows = (int)this->mainContentComponent.getListSpeaker().size();
                selectedRow = this->numRows - 1;
            } else {
                this->mainContentComponent.insertSpeaker(selectedRow, sortColumnId, sortedForwards);
                selectedRow += 1;
                this->numRows = (int)this->mainContentComponent.getListSpeaker().size();

            }

            float azimuth = 360.0f / this->rNumOfSpeakers.getText().getIntValue() * i + this->rOffsetAngle.getText().getFloatValue();
            if (azimuth > 360.0f) {
                azimuth -= 360.0f;
            } else if (azimuth < 0.0f) {
                azimuth += 360.0f;
            }
            float zenith = this->rZenith.getText().getFloatValue();
            float radius = this->rRadius.getText().getFloatValue();
            this->mainContentComponent.getListSpeaker()[selectedRow]->setAziZenRad(glm::vec3(azimuth, zenith, radius));
        }
        this->updateWinContent();
        this->tableListSpeakers.selectRow(selectedRow);
        // TableList needs different sorting parameters to trigger the sorting function.
        this->tableListSpeakers.getHeader().setSortColumnId(sortColumnId, ! sortedForwards);
        // This is the real sorting!
        this->tableListSpeakers.getHeader().setSortColumnId(sortColumnId, sortedForwards);
        this->mainContentComponent.needToComputeVbap = true;
    } else if (button == &this->pinkNoise) {
        this->mainContentComponent.getJackClient()->pinkNoiseSound = this->pinkNoise.getToggleState();
    } else if (button->getName() != "" && (button->getName().getIntValue() >= 0 &&
              (unsigned int)button->getName().getIntValue() <= this->mainContentComponent.getListSpeaker().size())) {
        if (this->tableListSpeakers.getNumSelectedRows() > 1 &&
                this->tableListSpeakers.getSelectedRows().contains(button->getName().getIntValue())) {
            for (int i = this->tableListSpeakers.getSelectedRows().size() - 1; i >= 0 ; i--) {
                int rownum = this->tableListSpeakers.getSelectedRows()[i];
                this->mainContentComponent.removeSpeaker(rownum);
            }
        } else {
            this->mainContentComponent.removeSpeaker(button->getName().getIntValue());
        }
        this->mainContentComponent.resetSpeakerIds();
        updateWinContent();
        this->tableListSpeakers.deselectAllRows();
        this->mainContentComponent.needToComputeVbap = true;
    } else {
        int row = button->getName().getIntValue() - 1000;
        this->mainContentComponent.getListSpeaker()[row]->setDirectOut(button->getToggleState());
        if (this->tableListSpeakers.getNumSelectedRows() > 1) {
            for (int i = 0; i < this->tableListSpeakers.getSelectedRows().size(); i++) {
                int rownum = this->tableListSpeakers.getSelectedRows()[i];
                this->mainContentComponent.getListSpeaker()[rownum]->setDirectOut(button->getToggleState());
                ToggleButton *tog = dynamic_cast<ToggleButton *>(this->tableListSpeakers.getCellComponent(11, rownum));
                tog->setToggleState(button->getToggleState(), NotificationType::dontSendNotification);
            }
        }
        updateWinContent();
        this->mainContentComponent.needToComputeVbap = true;
    }
}

//==============================================================================
void EditSpeakersWindow::textEditorTextChanged(juce::TextEditor& editor) {
    float value;
    String test;
    if (&editor == &this->rNumOfSpeakers) {
    } else if (&editor == &this->rZenith) {
        test = this->rZenith.getText().retainCharacters(".");
        if (test.length() > 1) {
            this->rZenith.setText(this->rZenith.getText().dropLastCharacters(1), false);
        }
        value = this->rZenith.getText().getFloatValue();
        if (value > 90.0f) {
            this->rZenith.setText(String(90.0f), false);
        } else if (value < -90.0f) {
            this->rZenith.setText(String(-90.0f), false);
        }
    } else if (&editor == &this->rRadius) {
        test = this->rRadius.getText().retainCharacters(".");
        if (test.length() > 1) {
            this->rRadius.setText(this->rRadius.getText().dropLastCharacters(1), false);
        }
        value = this->rRadius.getText().getFloatValue();
        if (value > 1.0f) {
            this->rRadius.setText(String(1.0f), false);
        }
    } else if (&editor == &this->rOffsetAngle) {
        test = this->rOffsetAngle.getText().retainCharacters(".");
        if (test.length() > 1) {
            this->rOffsetAngle.setText(this->rOffsetAngle.getText().dropLastCharacters(1), false);
        }
        value = this->rOffsetAngle.getText().getFloatValue();
        if (value < -180.0f) {
            this->rOffsetAngle.setText(String(-180.0f), false);
        } else if (value > 180.0f) {
            this->rOffsetAngle.setText(String(180.0f), false);
        }
    }
}

//==============================================================================
void EditSpeakersWindow::textEditorReturnKeyPressed(juce::TextEditor& editor) {
    juce::ignoreUnused(editor);
    this->unfocusAllComponents();
}

//==============================================================================
void EditSpeakersWindow::updateWinContent() {
    this->numRows = (unsigned int)this->mainContentComponent.getListSpeaker().size();
    this->tableListSpeakers.updateContent();
    if (this->initialized) {
        this->mainContentComponent.needToSaveSpeakerSetup = true;
    }
    this->initialized = true;
}

//==============================================================================
void EditSpeakersWindow::selectedRow(int const value) {
    MessageManagerLock mmlock;
    this->tableListSpeakers.selectRow(value);
    this->repaint();
}

//==============================================================================
void EditSpeakersWindow::closeButtonPressed() {
    int exitV = 1;
    if (this->mainContentComponent.needToSaveSpeakerSetup) {
        AlertWindow alert ("Closing Speaker Setup Window !",
                           "Do you want to compute and save the current setup ?",
                           AlertWindow::WarningIcon);
        alert.setLookAndFeel(&grisFeel);
        alert.addButton ("Save", 1, KeyPress(KeyPress::returnKey));
        alert.addButton ("No", 2);
        alert.addButton ("Cancel", 0, KeyPress(KeyPress::escapeKey));
        exitV = alert.runModalLoop();

        if (exitV == 1) {
            alert.setVisible(false);
            this->mainContentComponent.updateLevelComp();
            this->mainContentComponent.handleTimer(false);
            this->setAlwaysOnTop(false);
            this->mainContentComponent.handleSaveAsSpeakerSetup();
            this->mainContentComponent.handleTimer(true);
        } else if (exitV == 2) {
            alert.setVisible(false);
            this->mainContentComponent.reloadXmlFileSpeaker();
            this->mainContentComponent.updateLevelComp();
        }
    }
    if (exitV) {
        this->mainContentComponent.getJackClient()->pinkNoiseSound = false;
        this->mainContentComponent.destroyWinSpeakConf();
    }
}

//==============================================================================
void EditSpeakersWindow::resized() {
    this->juce::DocumentWindow::resized();

    tableListSpeakers.setSize(getWidth(), getHeight() - 195);

    this->boxListSpeaker->setSize(getWidth(), getHeight());
    this->boxListSpeaker->correctSize(getWidth() - 10, getHeight() - 30);

    this->butAddSpeaker.setBounds(5, getHeight() - 180, 100, 22);
    this->butcompSpeakers.setBounds(getWidth() - 105, getHeight() - 180, 100, 22);

    this->rNumOfSpeakersLabel.setBounds(5, getHeight() - 140, 80, 24);
    this->rNumOfSpeakers.setBounds(5 + 80, getHeight() - 140, 40, 24);
    this->rZenithLabel.setBounds(120, getHeight() - 140, 80, 24);
    this->rZenith.setBounds(120 + 80, getHeight() - 140, 60, 24);
    this->rRadiusLabel.setBounds(255, getHeight() - 140, 80, 24);
    this->rRadius.setBounds(255 + 80, getHeight() - 140, 60, 24);
    this->rOffsetAngleLabel.setBounds(400, getHeight() - 140, 80, 24);
    this->rOffsetAngle.setBounds(400 + 80, getHeight() - 140, 60, 24);
    this->butAddRing.setBounds(getWidth() - 105, getHeight() - 140, 100, 24);

    this->pinkNoise.setBounds(5, getHeight() - 75, 150, 24);
    this->pinkNoiseGain.setBounds(180, getHeight() - 100, 60, 60);
}

//==============================================================================
juce::String EditSpeakersWindow::getText(int const columnNumber, int const rowNumber) const {
    juce::String text = "";
    if (this->mainContentComponent.getListSpeaker().size() > (unsigned int)rowNumber) {
        switch (columnNumber) {
            case 1:
                text = String(this->mainContentComponent.getListSpeaker()[rowNumber]->getIdSpeaker());
                break;
            case 2:
                text = String(this->mainContentComponent.getListSpeaker()[rowNumber]->getCoordinate().x);
                break;
            case 3:
                text = String(this->mainContentComponent.getListSpeaker()[rowNumber]->getCoordinate().z);
                break;
            case 4:
                text = String(this->mainContentComponent.getListSpeaker()[rowNumber]->getCoordinate().y);
                break;
            case 5:
                text = String(this->mainContentComponent.getListSpeaker()[rowNumber]->getAziZenRad().x);
                break;
            case 6:
                text = String(this->mainContentComponent.getListSpeaker()[rowNumber]->getAziZenRad().y);
                break;
            case 7:
                text = String(this->mainContentComponent.getListSpeaker()[rowNumber]->getAziZenRad().z);
                break;
            case 8:
                text = String(this->mainContentComponent.getListSpeaker()[rowNumber]->getOutputPatch());
                break;
            case 9:
                text = String(this->mainContentComponent.getListSpeaker()[rowNumber]->getGain());
                break;
            case 10:
                text = String(this->mainContentComponent.getListSpeaker()[rowNumber]->getHighPassCutoff());
                break;
            case 11:
                text = String((int)this->mainContentComponent.getListSpeaker()[rowNumber]->getDirectOut());
                break;
            default:
                text = "?";
        }
    }

    return text;
}

//==============================================================================
void EditSpeakersWindow::setText(int const columnNumber, int const rowNumber, String const& newText, bool const altDown) {
    int ival;
    int oldval;
    float val;
    float diff;
    if (this->mainContentComponent.getLockSpeakers().try_lock()) {
        if (this->mainContentComponent.getListSpeaker().size() > (unsigned int)rowNumber) {
            glm::vec3 newP;
            switch (columnNumber) {
                case 2: // X
                    newP = this->mainContentComponent.getListSpeaker()[rowNumber]->getCoordinate();
                    val = GetFloatPrecision(newText.getFloatValue(), 3);
                    diff = val - newP.x;
                    newP.x = val;
                    this->mainContentComponent.getListSpeaker()[rowNumber]->setCoordinate(newP);
                    if (this->tableListSpeakers.getNumSelectedRows() > 1) {
                        for (int i = 0; i < this->tableListSpeakers.getSelectedRows().size(); i++) {
                            int rownum = this->tableListSpeakers.getSelectedRows()[i];
                            if (rownum == rowNumber) {
                                continue;
                            }
                            newP = this->mainContentComponent.getListSpeaker()[rownum]->getCoordinate();
                            if (altDown) {
                                newP.x += diff;
                            } else {
                                newP.x = val;
                            }
                            this->mainContentComponent.getListSpeaker()[rownum]->setCoordinate(newP);
                        }
                    }
                    break;
                case 3: // Y
                    newP = this->mainContentComponent.getListSpeaker()[rowNumber]->getCoordinate();
                    val = GetFloatPrecision(newText.getFloatValue(), 3);
                    diff = val - newP.z;
                    newP.z = val;
                    this->mainContentComponent.getListSpeaker()[rowNumber]->setCoordinate(newP);
                    if (this->tableListSpeakers.getNumSelectedRows() > 1) {
                        for (int i = 0; i < this->tableListSpeakers.getSelectedRows().size(); i++) {
                            int rownum = this->tableListSpeakers.getSelectedRows()[i];
                            if (rownum == rowNumber) {
                                continue;
                            }
                            newP = this->mainContentComponent.getListSpeaker()[rownum]->getCoordinate();
                            if (altDown) {
                                newP.z += diff;
                            } else {
                                newP.z = val;
                            }
                            this->mainContentComponent.getListSpeaker()[rownum]->setCoordinate(newP);
                        }
                    }
                    break;
                case 4: // Z
                    newP = this->mainContentComponent.getListSpeaker()[rowNumber]->getCoordinate();
                    val = GetFloatPrecision(newText.getFloatValue(), 3);
                    diff = val - newP.y;
                    val = val < -1.0f ? -1.0f : val > 1.0f ? 1.0f : val;
                    newP.y = val;
                    this->mainContentComponent.getListSpeaker()[rowNumber]->setCoordinate(newP);
                    if (this->tableListSpeakers.getNumSelectedRows() > 1) {
                        for (int i = 0; i < this->tableListSpeakers.getSelectedRows().size(); i++) {
                            int rownum = this->tableListSpeakers.getSelectedRows()[i];
                            if (rownum == rowNumber) {
                                continue;
                            }
                            newP = this->mainContentComponent.getListSpeaker()[rownum]->getCoordinate();
                            if (altDown) {
                                newP.y += diff;
                                newP.y = newP.y < -1.0f ? -1.0f : newP.y > 1.0f ? 1.0f : newP.y;
                            } else {
                                newP.y = val;
                            }
                            this->mainContentComponent.getListSpeaker()[rownum]->setCoordinate(newP);
                        }
                    }
                    break;
                case 5: // Azimuth
                    newP = this->mainContentComponent.getListSpeaker()[rowNumber]->getAziZenRad();
                    val = GetFloatPrecision(newText.getFloatValue(), 3);
                    diff = val - newP.x;
                    while (val > 360.0f) { val -= 360.0f; }
                    while (val < 0.0f) { val += 360.0f; }
                    newP.x = val;
                    this->mainContentComponent.getListSpeaker()[rowNumber]->setAziZenRad(newP);
                    if (this->tableListSpeakers.getNumSelectedRows() > 1) {
                        for (int i = 0; i < this->tableListSpeakers.getSelectedRows().size(); i++) {
                            int rownum = this->tableListSpeakers.getSelectedRows()[i];
                            if (rownum == rowNumber) {
                                continue;
                            }
                            newP = this->mainContentComponent.getListSpeaker()[rownum]->getAziZenRad();
                            if (altDown) {
                                newP.x += diff;
                                while (newP.x > 360.0f) { newP.x -= 360.0f; }
                                while (newP.x < 0.0f) { newP.x += 360.0f; }
                            } else {
                                newP.x = val;
                            }
                            this->mainContentComponent.getListSpeaker()[rownum]->setAziZenRad(newP);
                        }
                    }
                    break;
                case 6: // Elevation
                    newP = this->mainContentComponent.getListSpeaker()[rowNumber]->getAziZenRad();
                    val = GetFloatPrecision(newText.getFloatValue(), 3);
                    diff = val - newP.y;
                    if (val < -90.0f) { val = -90.0f; }
                    else if (val > 90.0f) { val = 90.0f; }
                    newP.y = val;
                    this->mainContentComponent.getListSpeaker()[rowNumber]->setAziZenRad(newP);
                    if (this->tableListSpeakers.getNumSelectedRows() > 1) {
                        for (int i = 0; i < this->tableListSpeakers.getSelectedRows().size(); i++) {
                            int rownum = this->tableListSpeakers.getSelectedRows()[i];
                            if (rownum == rowNumber) {
                                continue;
                            }
                            newP = this->mainContentComponent.getListSpeaker()[rownum]->getAziZenRad();
                            if (altDown) {
                                newP.y += diff;
                                if (newP.y < -90.0f) { newP.y = -90.0f; }
                                else if (newP.y > 90.0f) { newP.y = 90.0f; }
                            } else {
                                newP.y = val;
                            }
                            this->mainContentComponent.getListSpeaker()[rownum]->setAziZenRad(newP);
                        }
                    }
                    break;
                case 7: // Distance
                    newP = this->mainContentComponent.getListSpeaker()[rowNumber]->getAziZenRad();
                    if (this->mainContentComponent.isRadiusNormalized() && !this->mainContentComponent.getListSpeaker()[rowNumber]->getDirectOut()) {
                        val = 1.0;
                    } else {
                        val = GetFloatPrecision(newText.getFloatValue(), 3);
                    }
                    diff = val - newP.z;
                    if (val < 0.0f) { val = 0.0f; }
                    else if (val > 2.5f) { val = 2.5f; }
                    newP.z = val;
                    this->mainContentComponent.getListSpeaker()[rowNumber]->setAziZenRad(newP);
                    if (this->tableListSpeakers.getNumSelectedRows() > 1) {
                        for (int i = 0; i < this->tableListSpeakers.getSelectedRows().size(); i++) {
                            int rownum = this->tableListSpeakers.getSelectedRows()[i];
                            if (rownum == rowNumber) {
                                continue;
                            }
                            newP = this->mainContentComponent.getListSpeaker()[rownum]->getAziZenRad();
                            if (altDown) {
                                newP.z += diff;
                                if (newP.z < 0.0f) { newP.z = 0.0f; }
                                else if (newP.z > 2.5f) { newP.z = 2.5f; }
                            } else {
                                newP.z = val;
                            }
                            this->mainContentComponent.getListSpeaker()[rownum]->setAziZenRad(newP);
                        }
                    }
                    break;
                case 8: // Output patch
                    this->mainContentComponent.setShowTriplets(false);
                    oldval = this->mainContentComponent.getListSpeaker()[rowNumber]->getOutputPatch();
                    ival = newText.getIntValue();
                    if (ival < 0) { ival = 0; }
                    else if (ival > 256) { ival = 256; }
                    if (! this->mainContentComponent.getListSpeaker()[rowNumber]->getDirectOut()) {
                        for (auto&& it : this->mainContentComponent.getListSpeaker()) {
                            if (it == this->mainContentComponent.getListSpeaker()[rowNumber] || it->getDirectOut()) {
                                continue;
                            }
                            if (it->getOutputPatch() == ival) {
                                AlertWindow alert ("Wrong output patch!    ",
                                                   "Sorry! Output patch number " + String(ival) + " is already used.",
                                                   AlertWindow::WarningIcon);
                                alert.setLookAndFeel(&this->grisFeel);
                                alert.addButton("OK", 0, KeyPress(KeyPress::returnKey));
                                alert.runModalLoop();
                                ival = oldval;
                            }
                        }
                    }
                    this->mainContentComponent.getListSpeaker()[rowNumber]->setOutputPatch(ival);
                    break;
                case 9: // Gain
                    val = newText.getFloatValue();
                    diff = val - this->mainContentComponent.getListSpeaker()[rowNumber]->getGain();
                    if (val < -18.0f) { val = -18.0f; }
                    else if (val > 6.0f) { val = 6.0f; }
                    this->mainContentComponent.getListSpeaker()[rowNumber]->setGain(val);
                    if (this->tableListSpeakers.getNumSelectedRows() > 1) {
                        for (int i = 0; i < this->tableListSpeakers.getSelectedRows().size(); i++) {
                            int rownum = this->tableListSpeakers.getSelectedRows()[i];
                            if (rownum == rowNumber) {
                                continue;
                            }
                            if (altDown) {
                                float g = this->mainContentComponent.getListSpeaker()[rownum]->getGain() + diff;
                                if (g < -18.0f) { g = -18.0f; }
                                else if (g > 6.0f) { g = 6.0f; }
                                this->mainContentComponent.getListSpeaker()[rownum]->setGain(g);
                            } else {
                                this->mainContentComponent.getListSpeaker()[rownum]->setGain(val);
                            }
                        }
                    }
                    break;
                case 10: // Filter Cutoff
                    val = newText.getFloatValue();
                    diff = val - this->mainContentComponent.getListSpeaker()[rowNumber]->getHighPassCutoff();
                    if (val < 0.0f) { val = 0.0f; }
                    else if (val > 150.0f) { val = 150.0f; }
                    this->mainContentComponent.getListSpeaker()[rowNumber]->setHighPassCutoff(val);
                    if (this->tableListSpeakers.getNumSelectedRows() > 1) {
                        for (int i = 0; i < this->tableListSpeakers.getSelectedRows().size(); i++) {
                            int rownum = this->tableListSpeakers.getSelectedRows()[i];
                            if (rownum == rowNumber) {
                                continue;
                            }
                            if (altDown) {
                                float g = this->mainContentComponent.getListSpeaker()[rownum]->getHighPassCutoff() + diff;
                                if (g < 0.0f) { g = 0.0f; }
                                else if (g > 150.0f) { g = 150.0f; }
                                this->mainContentComponent.getListSpeaker()[rownum]->setHighPassCutoff(g);
                            } else {
                                this->mainContentComponent.getListSpeaker()[rownum]->setHighPassCutoff(val);
                            }
                        }
                    }
                    break;
                case 11: // Direct Out
                    this->mainContentComponent.setShowTriplets(false);
                    this->mainContentComponent.getListSpeaker()[rowNumber]->setDirectOut(newText.getIntValue());
                    break;
            }
        }
        this->updateWinContent();
        this->mainContentComponent.needToComputeVbap = true;
        this->mainContentComponent.getLockSpeakers().unlock();
    }
}

//==============================================================================
// This is overloaded from TableListBoxModel, and should fill in the background of the whole row.
void EditSpeakersWindow::paintRowBackground(Graphics& g, int const rowNumber, int const width, int const height, bool const rowIsSelected) {
    juce::ignoreUnused(width);
    juce::ignoreUnused(height);

    if (rowIsSelected) {
        if (this->mainContentComponent.getLockSpeakers().try_lock()) {
            this->mainContentComponent.getListSpeaker()[rowNumber]->selectSpeaker();
            this->mainContentComponent.getLockSpeakers().unlock();
        }
        g.fillAll(this->grisFeel.getHighlightColour());
    } else {
        if (this->mainContentComponent.getLockSpeakers().try_lock()) {
            this->mainContentComponent.getListSpeaker()[rowNumber]->unSelectSpeaker();
            this->mainContentComponent.getLockSpeakers().unlock();
        }
        if (rowNumber % 2) {
            g.fillAll(this->grisFeel.getBackgroundColour().withBrightness(0.6));
        } else {
            g.fillAll(this->grisFeel.getBackgroundColour().withBrightness(0.7));
        }
    }
}

//==============================================================================
// This is overloaded from TableListBoxModel, and must paint any cells that aren't using custom components.
void EditSpeakersWindow::paintCell(Graphics& g, int rowNumber, int columnId, int width, int height, bool /*rowIsSelected*/) {
    g.setColour(Colours::black);
    g.setFont(font);

    if (this->mainContentComponent.getLockSpeakers().try_lock()) {
        if (this->mainContentComponent.getListSpeaker().size() > static_cast<unsigned int>(rowNumber)) {
            String text = getText(columnId, rowNumber);
            g.drawText(text, 2, 0, width - 4, height, Justification::centredLeft, true);
        }
        this->mainContentComponent.getLockSpeakers().unlock();
    }
    g.setColour(Colours::black.withAlpha (0.2f));
    g.fillRect(width - 1, 0, 1, height);
}

//==============================================================================
Component * EditSpeakersWindow::refreshComponentForCell(int const rowNumber, int const columnId, bool const isRowSelected, Component *existingComponentToUpdate) {
    juce::ignoreUnused(isRowSelected);

    if (columnId == 11) {
        ToggleButton *tbDirect = static_cast<ToggleButton *> (existingComponentToUpdate);
        if (tbDirect == nullptr)
            tbDirect = new ToggleButton();
        tbDirect->setName(String(rowNumber + 1000));
        tbDirect->setClickingTogglesState(true);
        tbDirect->setBounds(4, 404, 88, 22);
        tbDirect->addListener(this);
        tbDirect->setToggleState(this->mainContentComponent.getListSpeaker()[rowNumber]->getDirectOut(), dontSendNotification);
        tbDirect->setLookAndFeel(&this->grisFeel);
        return tbDirect;
    }
    if (columnId == 12) {
        TextButton *tbRemove = static_cast<TextButton *> (existingComponentToUpdate);
        if (tbRemove == nullptr)
            tbRemove = new TextButton();
        tbRemove->setButtonText("X");
        tbRemove->setName(String(rowNumber));
        tbRemove->setBounds(4, 404, 88, 22);
        tbRemove->addListener(this);
        tbRemove->setColour(ToggleButton::textColourId, this->grisFeel.getFontColour());
        tbRemove->setLookAndFeel(&this->grisFeel);
        return tbRemove;
    }

    // The other columns are editable text columns, for which we use the custom Label component
    EditableTextCustomComponent *textLabel = static_cast<EditableTextCustomComponent *> (existingComponentToUpdate);
    if (textLabel == nullptr)
        textLabel = new EditableTextCustomComponent(*this);

    textLabel->setRowAndColumn(rowNumber, columnId);

    if (this->mainContentComponent.getModeSelected() == LBAP || this->mainContentComponent.getListSpeaker()[rowNumber]->getDirectOut()) {
        if (columnId < 2) {
            textLabel->setEditable(false);
        }
    } else {
        if (columnId < 5) {
            textLabel->setEditable(false);
        }
    }

    return textLabel;
}

//==============================================================================
int EditSpeakersWindow::getModeSelected() const {
    return this->mainContentComponent.getModeSelected();
}

//==============================================================================
bool EditSpeakersWindow::getDirectOutForSpeakerRow(int const row) const {
    return this->mainContentComponent.getListSpeaker()[row]->getDirectOut();
}