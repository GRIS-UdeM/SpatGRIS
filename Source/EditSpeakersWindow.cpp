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

#include "EditSpeakersWindow.h"

#include "EditableTextCustomComponent.h"
#include "GrisLookAndFeel.h"
#include "MainComponent.h"

//==============================================================================
static double GetFloatPrecision(double value, double precision)
{
    return (floor((value * pow(10, precision) + 0.5)) / pow(10, precision));
}

//==============================================================================
EditSpeakersWindow::EditSpeakersWindow(juce::String const & name,
                                       GrisLookAndFeel & lookAndFeel,
                                       MainContentComponent & mainContentComponent,
                                       juce::String const & configName)
    : juce::DocumentWindow(name, lookAndFeel.getBackgroundColour(), DocumentWindow::allButtons)
    , mLookAndFeel(lookAndFeel)
    , mMainContentComponent(mainContentComponent)
    , mFont(14.0f)
    , mListSpeakerBox(lookAndFeel, "Configuration Speakers")
{
    this->mAddSpeakerButton.setButtonText("Add Speaker");
    this->mAddSpeakerButton.setBounds(5, 404, 100, 22);
    this->mAddSpeakerButton.addListener(this);
    this->mAddSpeakerButton.setColour(juce::ToggleButton::textColourId, this->mLookAndFeel.getFontColour());
    this->mAddSpeakerButton.setLookAndFeel(&this->mLookAndFeel);
    this->mListSpeakerBox.getContent()->addAndMakeVisible(this->mAddSpeakerButton);

    this->mCompSpeakersButton.setButtonText("Compute");
    this->mCompSpeakersButton.setBounds(110, 404, 100, 22);
    this->mCompSpeakersButton.addListener(this);
    this->mCompSpeakersButton.setColour(juce::ToggleButton::textColourId, this->mLookAndFeel.getFontColour());
    this->mCompSpeakersButton.setLookAndFeel(&this->mLookAndFeel);
    this->mListSpeakerBox.getContent()->addAndMakeVisible(this->mCompSpeakersButton);

    // Generate ring of speakers.
    int wlab = 80;

    this->mNumOfSpeakersLabel.setText("# of speakers", juce::NotificationType::dontSendNotification);
    this->mNumOfSpeakersLabel.setJustificationType(juce::Justification::right);
    this->mNumOfSpeakersLabel.setFont(this->mLookAndFeel.getFont());
    this->mNumOfSpeakersLabel.setLookAndFeel(&this->mLookAndFeel);
    this->mNumOfSpeakersLabel.setColour(juce::Label::textColourId, this->mLookAndFeel.getFontColour());
    this->mNumOfSpeakersLabel.setBounds(5, 435, 40, 24);
    this->mListSpeakerBox.getContent()->addAndMakeVisible(this->mNumOfSpeakersLabel);

    this->mNumOfSpeakersTextEditor.setTooltip("Number of speakers in the ring");
    this->mNumOfSpeakersTextEditor.setColour(juce::ToggleButton::textColourId, this->mLookAndFeel.getFontColour());
    this->mNumOfSpeakersTextEditor.setLookAndFeel(&this->mLookAndFeel);
    this->mNumOfSpeakersTextEditor.setBounds(5 + wlab, 435, 40, 24);
    this->mNumOfSpeakersTextEditor.addListener(&this->mMainContentComponent);
    this->mListSpeakerBox.getContent()->addAndMakeVisible(this->mNumOfSpeakersTextEditor);

    this->mNumOfSpeakersTextEditor.setText("8");
    this->mNumOfSpeakersTextEditor.setInputRestrictions(3, "0123456789");
    this->mNumOfSpeakersTextEditor.addListener(this);

    this->mZenithLabel.setText("Elevation", juce::NotificationType::dontSendNotification);
    this->mZenithLabel.setJustificationType(juce::Justification::right);
    this->mZenithLabel.setFont(this->mLookAndFeel.getFont());
    this->mZenithLabel.setLookAndFeel(&this->mLookAndFeel);
    this->mZenithLabel.setColour(juce::Label::textColourId, this->mLookAndFeel.getFontColour());
    this->mZenithLabel.setBounds(105, 435, 80, 24);
    this->mListSpeakerBox.getContent()->addAndMakeVisible(this->mZenithLabel);

    this->mZenithTextEditor.setTooltip("Elevation angle of the ring");
    this->mZenithTextEditor.setColour(juce::ToggleButton::textColourId, this->mLookAndFeel.getFontColour());
    this->mZenithTextEditor.setLookAndFeel(&this->mLookAndFeel);
    this->mZenithTextEditor.setBounds(105 + wlab, 435, 60, 24);
    this->mZenithTextEditor.addListener(&this->mMainContentComponent);
    this->mListSpeakerBox.getContent()->addAndMakeVisible(this->mZenithTextEditor);

    this->mZenithTextEditor.setText("0.0");
    this->mZenithTextEditor.setInputRestrictions(6, "-0123456789.");
    this->mZenithTextEditor.addListener(this);

    this->mRadiusLabel.setText("Distance", juce::NotificationType::dontSendNotification);
    this->mRadiusLabel.setJustificationType(juce::Justification::right);
    this->mRadiusLabel.setFont(this->mLookAndFeel.getFont());
    this->mRadiusLabel.setLookAndFeel(&this->mLookAndFeel);
    this->mRadiusLabel.setColour(juce::Label::textColourId, this->mLookAndFeel.getFontColour());
    this->mRadiusLabel.setBounds(230, 435, 80, 24);
    this->mListSpeakerBox.getContent()->addAndMakeVisible(this->mRadiusLabel);

    this->mRadiusTextEditor.setTooltip("Distance of the speakers from the center.");
    this->mRadiusTextEditor.setColour(juce::ToggleButton::textColourId, this->mLookAndFeel.getFontColour());
    this->mRadiusTextEditor.setLookAndFeel(&this->mLookAndFeel);
    this->mRadiusTextEditor.setBounds(230 + wlab, 435, 60, 24);
    this->mRadiusTextEditor.addListener(&this->mMainContentComponent);
    this->mListSpeakerBox.getContent()->addAndMakeVisible(this->mRadiusTextEditor);

    this->mRadiusTextEditor.setText("1.0");
    this->mRadiusTextEditor.setInputRestrictions(6, "0123456789.");
    this->mRadiusTextEditor.addListener(this);

    this->mOffsetAngleLabel.setText("Offset Angle", juce::NotificationType::dontSendNotification);
    this->mOffsetAngleLabel.setJustificationType(juce::Justification::right);
    this->mOffsetAngleLabel.setFont(this->mLookAndFeel.getFont());
    this->mOffsetAngleLabel.setLookAndFeel(&this->mLookAndFeel);
    this->mOffsetAngleLabel.setColour(juce::Label::textColourId, this->mLookAndFeel.getFontColour());
    this->mOffsetAngleLabel.setBounds(375, 435, 80, 24);
    this->mListSpeakerBox.getContent()->addAndMakeVisible(this->mOffsetAngleLabel);

    this->mOffsetAngleTextEditor.setTooltip("Offset angle of the first speaker.");
    this->mOffsetAngleTextEditor.setColour(juce::ToggleButton::textColourId, this->mLookAndFeel.getFontColour());
    this->mOffsetAngleTextEditor.setLookAndFeel(&this->mLookAndFeel);
    this->mOffsetAngleTextEditor.setBounds(375 + wlab, 435, 60, 24);
    this->mOffsetAngleTextEditor.addListener(&this->mMainContentComponent);
    this->mListSpeakerBox.getContent()->addAndMakeVisible(this->mOffsetAngleTextEditor);

    this->mOffsetAngleTextEditor.setText("0.0");
    this->mOffsetAngleTextEditor.setInputRestrictions(6, "-0123456789.");
    this->mOffsetAngleTextEditor.addListener(this);

    this->mAddRingButton.setButtonText("Add Ring");
    this->mAddRingButton.setBounds(520, 435, 100, 24);
    this->mAddRingButton.addListener(this);
    this->mAddRingButton.setColour(juce::ToggleButton::textColourId, this->mLookAndFeel.getFontColour());
    this->mAddRingButton.setLookAndFeel(&this->mLookAndFeel);
    this->mListSpeakerBox.getContent()->addAndMakeVisible(this->mAddRingButton);

    // Pink noise controls.
    this->mPinkNoiseToggleButton.setButtonText("Reference Pink Noise");
    this->mPinkNoiseToggleButton.setBounds(5, 500, 150, 24);
    this->mPinkNoiseToggleButton.addListener(this);
    this->mPinkNoiseToggleButton.setColour(juce::ToggleButton::textColourId, this->mLookAndFeel.getFontColour());
    this->mPinkNoiseToggleButton.setLookAndFeel(&this->mLookAndFeel);
    this->mListSpeakerBox.getContent()->addAndMakeVisible(this->mPinkNoiseToggleButton);

    this->mPinkNoiseGainSlider.setTextValueSuffix(" dB");
    this->mPinkNoiseGainSlider.setBounds(200, 500, 60, 60);
    this->mPinkNoiseGainSlider.setSliderStyle(juce::Slider::Rotary);
    this->mPinkNoiseGainSlider.setRotaryParameters(M_PI * 1.3f, M_PI * 2.7f, true);
    this->mPinkNoiseGainSlider.setRange(-60, -3, 1);
    this->mPinkNoiseGainSlider.setValue(-20);
    this->mPinkNoiseGainSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 20);
    this->mPinkNoiseGainSlider.setColour(juce::ToggleButton::textColourId, this->mLookAndFeel.getFontColour());
    this->mPinkNoiseGainSlider.setLookAndFeel(&this->mLookAndFeel);
    this->mPinkNoiseGainSlider.addListener(this);
    this->mListSpeakerBox.getContent()->addAndMakeVisible(this->mPinkNoiseGainSlider);

    this->mListSpeakerBox.getContent()->addAndMakeVisible(this->mSpeakersTableListBox);

    this->mListSpeakerBox.repaint();
    this->mListSpeakerBox.resized();

    this->setContentNonOwned(&this->mListSpeakerBox, false);
}

//==============================================================================
void EditSpeakersWindow::initComp()
{
    mSpeakersTableListBox.setModel(this);

    mSpeakersTableListBox.setColour(juce::ListBox::outlineColourId, this->mLookAndFeel.getWinBackgroundColour());
    mSpeakersTableListBox.setColour(juce::ListBox::backgroundColourId, this->mLookAndFeel.getWinBackgroundColour());
    mSpeakersTableListBox.setOutlineThickness(1);

    mSpeakersTableListBox.getHeader().addColumn("ID", 1, 40, 40, 60, juce::TableHeaderComponent::defaultFlags);
    mSpeakersTableListBox.getHeader().addColumn("X", 2, 70, 50, 120, juce::TableHeaderComponent::defaultFlags);
    mSpeakersTableListBox.getHeader().addColumn("Y", 3, 70, 50, 120, juce::TableHeaderComponent::defaultFlags);
    mSpeakersTableListBox.getHeader().addColumn("Z", 4, 70, 50, 120, juce::TableHeaderComponent::defaultFlags);
    mSpeakersTableListBox.getHeader().addColumn("Azimuth", 5, 70, 50, 120, juce::TableHeaderComponent::defaultFlags);
    mSpeakersTableListBox.getHeader().addColumn("Elevation", 6, 70, 50, 120, juce::TableHeaderComponent::defaultFlags);
    mSpeakersTableListBox.getHeader().addColumn("Distance", 7, 70, 50, 120, juce::TableHeaderComponent::defaultFlags);
    mSpeakersTableListBox.getHeader().addColumn("Output", 8, 70, 50, 120, juce::TableHeaderComponent::defaultFlags);
    mSpeakersTableListBox.getHeader().addColumn("Gain (dB)", 9, 70, 50, 120, juce::TableHeaderComponent::notSortable);
    mSpeakersTableListBox.getHeader().addColumn("Highpass", 10, 70, 50, 120, juce::TableHeaderComponent::notSortable);
    mSpeakersTableListBox.getHeader().addColumn("Direct", 11, 70, 50, 120, juce::TableHeaderComponent::notSortable);
    mSpeakersTableListBox.getHeader().addColumn("delete", 12, 70, 50, 120, juce::TableHeaderComponent::notSortable);

    mSpeakersTableListBox.getHeader().setSortColumnId(1, true); // Sort forwards by the ID column.

    mSpeakersTableListBox.setMultipleSelectionEnabled(true);

    mNumRows = (unsigned int)this->mMainContentComponent.getListSpeaker().size();

    this->mListSpeakerBox.setBounds(0, 0, getWidth(), getHeight());
    this->mListSpeakerBox.correctSize(getWidth() - 8, getHeight());
    mSpeakersTableListBox.setSize(getWidth(), 400);

    mSpeakersTableListBox.updateContent();

    this->mListSpeakerBox.repaint();
    this->mListSpeakerBox.resized();
    this->resized();
}

//==============================================================================
struct Sorter {
    int id;
    float value;
    bool directout;
};

//==============================================================================
bool compareLessThan(Sorter const & a, Sorter const & b)
{
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
bool compareGreaterThan(Sorter const & a, Sorter const & b)
{
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
void EditSpeakersWindow::sortOrderChanged(int const newSortColumnId, bool const isForwards)
{
    int size = (int)this->mMainContentComponent.getListSpeaker().size();
    struct Sorter tosort[MaxOutputs];

    for (int i = 0; i < size; i++) {
        tosort[i].id = this->mMainContentComponent.getListSpeaker()[i]->getIdSpeaker();
        tosort[i].directout = this->mMainContentComponent.getListSpeaker()[i]->isDirectOut();
        switch (newSortColumnId) {
        case 1:
            tosort[i].value = (float)this->mMainContentComponent.getListSpeaker()[i]->getIdSpeaker();
            break;
        case 2:
            tosort[i].value = (float)this->mMainContentComponent.getListSpeaker()[i]->getCoordinate().x;
            break;
        case 3:
            tosort[i].value = (float)this->mMainContentComponent.getListSpeaker()[i]->getCoordinate().z;
            break;
        case 4:
            tosort[i].value = (float)this->mMainContentComponent.getListSpeaker()[i]->getCoordinate().y;
            break;
        case 5:
            tosort[i].value = (float)this->mMainContentComponent.getListSpeaker()[i]->getAziZenRad().x;
            break;
        case 6:
            tosort[i].value = (float)this->mMainContentComponent.getListSpeaker()[i]->getAziZenRad().y;
            break;
        case 7:
            tosort[i].value = (float)this->mMainContentComponent.getListSpeaker()[i]->getAziZenRad().z;
            break;
        case 8:
            tosort[i].value = (float)this->mMainContentComponent.getListSpeaker()[i]->getOutputPatch();
            break;
        }
    }
    if (isForwards) {
        std::sort(tosort, tosort + size, compareLessThan);
    } else {
        std::sort(tosort, tosort + size, compareGreaterThan);
    }

    std::vector<int> newOrder(size);
    for (int i = 0; i < size; i++) {
        newOrder[i] = tosort[i].id;
    }
    this->mMainContentComponent.reorderSpeakers(newOrder);
    updateWinContent();
}

//==============================================================================
void EditSpeakersWindow::sliderValueChanged(juce::Slider * slider)
{
    float gain;
    if (slider == &this->mPinkNoiseGainSlider) {
        gain = powf(10.0f, this->mPinkNoiseGainSlider.getValue() / 20.0f);
        this->mMainContentComponent.getJackClient()->pinkNoiseGain = gain;
    }
}

//==============================================================================
void EditSpeakersWindow::buttonClicked(juce::Button * button)
{
    bool tripletState = this->mMainContentComponent.isTripletsShown;
    int selectedRow = this->mSpeakersTableListBox.getSelectedRow();
    int sortColumnId = this->mSpeakersTableListBox.getHeader().getSortColumnId();
    bool sortedForwards = this->mSpeakersTableListBox.getHeader().isSortedForwards();

    this->mMainContentComponent.setShowTriplets(false);

    if (button == &this->mAddSpeakerButton) {
        if (selectedRow == -1 || selectedRow == (this->mNumRows - 1)) {
            this->mMainContentComponent.addSpeaker(sortColumnId, sortedForwards);
            this->updateWinContent();
            this->mSpeakersTableListBox.selectRow(this->getNumRows() - 1);
        } else {
            this->mMainContentComponent.insertSpeaker(selectedRow, sortColumnId, sortedForwards);
            this->updateWinContent();
            this->mSpeakersTableListBox.selectRow(selectedRow + 1);
        }
        this->mSpeakersTableListBox.getHeader().setSortColumnId(sortColumnId, sortedForwards);
        this->mMainContentComponent.needToComputeVbap = true;
    } else if (button == &this->mCompSpeakersButton) {
        if (this->mMainContentComponent.updateLevelComp()) {
            this->mMainContentComponent.setShowTriplets(tripletState);
        }
    } else if (button == &this->mAddRingButton) {
        for (int i = 0; i < this->mNumOfSpeakersTextEditor.getText().getIntValue(); i++) {
            if (selectedRow == -1 || selectedRow == (this->mNumRows - 1)) {
                this->mMainContentComponent.addSpeaker(sortColumnId, sortedForwards);
                this->mNumRows = (int)this->mMainContentComponent.getListSpeaker().size();
                selectedRow = this->mNumRows - 1;
            } else {
                this->mMainContentComponent.insertSpeaker(selectedRow, sortColumnId, sortedForwards);
                selectedRow += 1;
                this->mNumRows = (int)this->mMainContentComponent.getListSpeaker().size();
            }

            float azimuth = 360.0f / this->mNumOfSpeakersTextEditor.getText().getIntValue() * i
                            + this->mOffsetAngleTextEditor.getText().getFloatValue();
            if (azimuth > 360.0f) {
                azimuth -= 360.0f;
            } else if (azimuth < 0.0f) {
                azimuth += 360.0f;
            }
            float zenith = this->mZenithTextEditor.getText().getFloatValue();
            float radius = this->mRadiusTextEditor.getText().getFloatValue();
            this->mMainContentComponent.getListSpeaker()[selectedRow]->setAziZenRad(glm::vec3(azimuth, zenith, radius));
        }
        this->updateWinContent();
        this->mSpeakersTableListBox.selectRow(selectedRow);
        // TableList needs different sorting parameters to trigger the sorting function.
        this->mSpeakersTableListBox.getHeader().setSortColumnId(sortColumnId, !sortedForwards);
        // This is the real sorting!
        this->mSpeakersTableListBox.getHeader().setSortColumnId(sortColumnId, sortedForwards);
        this->mMainContentComponent.needToComputeVbap = true;
    } else if (button == &this->mPinkNoiseToggleButton) {
        this->mMainContentComponent.getJackClient()->pinkNoiseSound = this->mPinkNoiseToggleButton.getToggleState();
    } else if (button->getName() != ""
               && (button->getName().getIntValue() >= 0
                   && (unsigned int)button->getName().getIntValue()
                          <= this->mMainContentComponent.getListSpeaker().size())) {
        if (this->mSpeakersTableListBox.getNumSelectedRows() > 1
            && this->mSpeakersTableListBox.getSelectedRows().contains(button->getName().getIntValue())) {
            for (int i = this->mSpeakersTableListBox.getSelectedRows().size() - 1; i >= 0; i--) {
                int rownum = this->mSpeakersTableListBox.getSelectedRows()[i];
                this->mMainContentComponent.removeSpeaker(rownum);
            }
        } else {
            this->mMainContentComponent.removeSpeaker(button->getName().getIntValue());
        }
        this->mMainContentComponent.resetSpeakerIds();
        updateWinContent();
        this->mSpeakersTableListBox.deselectAllRows();
        this->mMainContentComponent.needToComputeVbap = true;
    } else {
        int row = button->getName().getIntValue() - 1000;
        this->mMainContentComponent.getListSpeaker()[row]->setDirectOut(button->getToggleState());
        if (this->mSpeakersTableListBox.getNumSelectedRows() > 1) {
            for (int i = 0; i < this->mSpeakersTableListBox.getSelectedRows().size(); i++) {
                int rownum = this->mSpeakersTableListBox.getSelectedRows()[i];
                this->mMainContentComponent.getListSpeaker()[rownum]->setDirectOut(button->getToggleState());
                juce::ToggleButton * tog
                    = dynamic_cast<juce::ToggleButton *>(this->mSpeakersTableListBox.getCellComponent(11, rownum));
                tog->setToggleState(button->getToggleState(), juce::NotificationType::dontSendNotification);
            }
        }
        updateWinContent();
        this->mMainContentComponent.needToComputeVbap = true;
    }
}

//==============================================================================
void EditSpeakersWindow::textEditorTextChanged(juce::TextEditor & editor)
{
    float value;
    juce::String test;
    if (&editor == &this->mNumOfSpeakersTextEditor) {
    } else if (&editor == &this->mZenithTextEditor) {
        test = this->mZenithTextEditor.getText().retainCharacters(".");
        if (test.length() > 1) {
            this->mZenithTextEditor.setText(this->mZenithTextEditor.getText().dropLastCharacters(1), false);
        }
        value = this->mZenithTextEditor.getText().getFloatValue();
        if (value > 90.0f) {
            this->mZenithTextEditor.setText(juce::String(90.0f), false);
        } else if (value < -90.0f) {
            this->mZenithTextEditor.setText(juce::String(-90.0f), false);
        }
    } else if (&editor == &this->mRadiusTextEditor) {
        test = this->mRadiusTextEditor.getText().retainCharacters(".");
        if (test.length() > 1) {
            this->mRadiusTextEditor.setText(this->mRadiusTextEditor.getText().dropLastCharacters(1), false);
        }
        value = this->mRadiusTextEditor.getText().getFloatValue();
        if (value > 1.0f) {
            this->mRadiusTextEditor.setText(juce::String(1.0f), false);
        }
    } else if (&editor == &this->mOffsetAngleTextEditor) {
        test = this->mOffsetAngleTextEditor.getText().retainCharacters(".");
        if (test.length() > 1) {
            this->mOffsetAngleTextEditor.setText(this->mOffsetAngleTextEditor.getText().dropLastCharacters(1), false);
        }
        value = this->mOffsetAngleTextEditor.getText().getFloatValue();
        if (value < -180.0f) {
            this->mOffsetAngleTextEditor.setText(juce::String(-180.0f), false);
        } else if (value > 180.0f) {
            this->mOffsetAngleTextEditor.setText(juce::String(180.0f), false);
        }
    }
}

//==============================================================================
void EditSpeakersWindow::textEditorReturnKeyPressed(juce::TextEditor & editor)
{
    juce::ignoreUnused(editor);
    this->unfocusAllComponents();
}

//==============================================================================
void EditSpeakersWindow::updateWinContent()
{
    this->mNumRows = (unsigned int)this->mMainContentComponent.getListSpeaker().size();
    this->mSpeakersTableListBox.updateContent();
    if (this->mInitialized) {
        this->mMainContentComponent.needToSaveSpeakerSetup = true;
    }
    this->mInitialized = true;
}

//==============================================================================
void EditSpeakersWindow::selectedRow(int const value)
{
    juce::MessageManagerLock mmlock;
    this->mSpeakersTableListBox.selectRow(value);
    this->repaint();
}

//==============================================================================
void EditSpeakersWindow::closeButtonPressed()
{
    int exitV = 1;
    if (this->mMainContentComponent.needToSaveSpeakerSetup) {
        juce::AlertWindow alert("Closing Speaker Setup Window !",
                                "Do you want to compute and save the current setup ?",
                                juce::AlertWindow::WarningIcon);
        alert.setLookAndFeel(&mLookAndFeel);
        alert.addButton("Save", 1, juce::KeyPress(juce::KeyPress::returnKey));
        alert.addButton("No", 2);
        alert.addButton("Cancel", 0, juce::KeyPress(juce::KeyPress::escapeKey));
        exitV = alert.runModalLoop();

        if (exitV == 1) {
            alert.setVisible(false);
            this->mMainContentComponent.updateLevelComp();
            this->mMainContentComponent.handleTimer(false);
            this->setAlwaysOnTop(false);
            this->mMainContentComponent.handleSaveAsSpeakerSetup();
            this->mMainContentComponent.handleTimer(true);
        } else if (exitV == 2) {
            alert.setVisible(false);
            this->mMainContentComponent.reloadXmlFileSpeaker();
            this->mMainContentComponent.updateLevelComp();
        }
    }
    if (exitV) {
        this->mMainContentComponent.getJackClient()->pinkNoiseSound = false;
        this->mMainContentComponent.closeSpeakersConfigurationWindow();
    }
}

//==============================================================================
void EditSpeakersWindow::resized()
{
    this->juce::DocumentWindow::resized();

    mSpeakersTableListBox.setSize(getWidth(), getHeight() - 195);

    this->mListSpeakerBox.setSize(getWidth(), getHeight());
    this->mListSpeakerBox.correctSize(getWidth() - 10, getHeight() - 30);

    this->mAddSpeakerButton.setBounds(5, getHeight() - 180, 100, 22);
    this->mCompSpeakersButton.setBounds(getWidth() - 105, getHeight() - 180, 100, 22);

    this->mNumOfSpeakersLabel.setBounds(5, getHeight() - 140, 80, 24);
    this->mNumOfSpeakersTextEditor.setBounds(5 + 80, getHeight() - 140, 40, 24);
    this->mZenithLabel.setBounds(120, getHeight() - 140, 80, 24);
    this->mZenithTextEditor.setBounds(120 + 80, getHeight() - 140, 60, 24);
    this->mRadiusLabel.setBounds(255, getHeight() - 140, 80, 24);
    this->mRadiusTextEditor.setBounds(255 + 80, getHeight() - 140, 60, 24);
    this->mOffsetAngleLabel.setBounds(400, getHeight() - 140, 80, 24);
    this->mOffsetAngleTextEditor.setBounds(400 + 80, getHeight() - 140, 60, 24);
    this->mAddRingButton.setBounds(getWidth() - 105, getHeight() - 140, 100, 24);

    this->mPinkNoiseToggleButton.setBounds(5, getHeight() - 75, 150, 24);
    this->mPinkNoiseGainSlider.setBounds(180, getHeight() - 100, 60, 60);
}

//==============================================================================
juce::String EditSpeakersWindow::getText(int const columnNumber, int const rowNumber) const
{
    juce::String text = "";
    if (this->mMainContentComponent.getListSpeaker().size() > (unsigned int)rowNumber) {
        switch (columnNumber) {
        case 1:
            text = juce::String(this->mMainContentComponent.getListSpeaker()[rowNumber]->getIdSpeaker());
            break;
        case 2:
            text = juce::String(this->mMainContentComponent.getListSpeaker()[rowNumber]->getCoordinate().x);
            break;
        case 3:
            text = juce::String(this->mMainContentComponent.getListSpeaker()[rowNumber]->getCoordinate().z);
            break;
        case 4:
            text = juce::String(this->mMainContentComponent.getListSpeaker()[rowNumber]->getCoordinate().y);
            break;
        case 5:
            text = juce::String(this->mMainContentComponent.getListSpeaker()[rowNumber]->getAziZenRad().x);
            break;
        case 6:
            text = juce::String(this->mMainContentComponent.getListSpeaker()[rowNumber]->getAziZenRad().y);
            break;
        case 7:
            text = juce::String(this->mMainContentComponent.getListSpeaker()[rowNumber]->getAziZenRad().z);
            break;
        case 8:
            text = juce::String(this->mMainContentComponent.getListSpeaker()[rowNumber]->getOutputPatch());
            break;
        case 9:
            text = juce::String(this->mMainContentComponent.getListSpeaker()[rowNumber]->getGain());
            break;
        case 10:
            text = juce::String(this->mMainContentComponent.getListSpeaker()[rowNumber]->getHighPassCutoff());
            break;
        case 11:
            text = juce::String(
                static_cast<int>(this->mMainContentComponent.getListSpeaker()[rowNumber]->isDirectOut()));
            break;
        default:
            text = "?";
        }
    }

    return text;
}

//==============================================================================
void EditSpeakersWindow::setText(int const columnNumber,
                                 int const rowNumber,
                                 juce::String const & newText,
                                 bool const altDown)
{
    int ival;
    int oldval;
    float val;
    float diff;
    if (this->mMainContentComponent.getLockSpeakers().try_lock()) {
        if (this->mMainContentComponent.getListSpeaker().size() > (unsigned int)rowNumber) {
            glm::vec3 newP;
            switch (columnNumber) {
            case 2: // X
                newP = this->mMainContentComponent.getListSpeaker()[rowNumber]->getCoordinate();
                val = GetFloatPrecision(newText.getFloatValue(), 3);
                diff = val - newP.x;
                newP.x = val;
                this->mMainContentComponent.getListSpeaker()[rowNumber]->setCoordinate(newP);
                if (this->mSpeakersTableListBox.getNumSelectedRows() > 1) {
                    for (int i = 0; i < this->mSpeakersTableListBox.getSelectedRows().size(); i++) {
                        int rownum = this->mSpeakersTableListBox.getSelectedRows()[i];
                        if (rownum == rowNumber) {
                            continue;
                        }
                        newP = this->mMainContentComponent.getListSpeaker()[rownum]->getCoordinate();
                        if (altDown) {
                            newP.x += diff;
                        } else {
                            newP.x = val;
                        }
                        this->mMainContentComponent.getListSpeaker()[rownum]->setCoordinate(newP);
                    }
                }
                break;
            case 3: // Y
                newP = this->mMainContentComponent.getListSpeaker()[rowNumber]->getCoordinate();
                val = GetFloatPrecision(newText.getFloatValue(), 3);
                diff = val - newP.z;
                newP.z = val;
                this->mMainContentComponent.getListSpeaker()[rowNumber]->setCoordinate(newP);
                if (this->mSpeakersTableListBox.getNumSelectedRows() > 1) {
                    for (int i = 0; i < this->mSpeakersTableListBox.getSelectedRows().size(); i++) {
                        int rownum = this->mSpeakersTableListBox.getSelectedRows()[i];
                        if (rownum == rowNumber) {
                            continue;
                        }
                        newP = this->mMainContentComponent.getListSpeaker()[rownum]->getCoordinate();
                        if (altDown) {
                            newP.z += diff;
                        } else {
                            newP.z = val;
                        }
                        this->mMainContentComponent.getListSpeaker()[rownum]->setCoordinate(newP);
                    }
                }
                break;
            case 4: // Z
                newP = this->mMainContentComponent.getListSpeaker()[rowNumber]->getCoordinate();
                val = GetFloatPrecision(newText.getFloatValue(), 3);
                diff = val - newP.y;
                val = val < -1.0f ? -1.0f : val > 1.0f ? 1.0f : val;
                newP.y = val;
                this->mMainContentComponent.getListSpeaker()[rowNumber]->setCoordinate(newP);
                if (this->mSpeakersTableListBox.getNumSelectedRows() > 1) {
                    for (int i = 0; i < this->mSpeakersTableListBox.getSelectedRows().size(); i++) {
                        int rownum = this->mSpeakersTableListBox.getSelectedRows()[i];
                        if (rownum == rowNumber) {
                            continue;
                        }
                        newP = this->mMainContentComponent.getListSpeaker()[rownum]->getCoordinate();
                        if (altDown) {
                            newP.y += diff;
                            newP.y = newP.y < -1.0f ? -1.0f : newP.y > 1.0f ? 1.0f : newP.y;
                        } else {
                            newP.y = val;
                        }
                        this->mMainContentComponent.getListSpeaker()[rownum]->setCoordinate(newP);
                    }
                }
                break;
            case 5: // Azimuth
                newP = this->mMainContentComponent.getListSpeaker()[rowNumber]->getAziZenRad();
                val = GetFloatPrecision(newText.getFloatValue(), 3);
                diff = val - newP.x;
                while (val > 360.0f) {
                    val -= 360.0f;
                }
                while (val < 0.0f) {
                    val += 360.0f;
                }
                newP.x = val;
                this->mMainContentComponent.getListSpeaker()[rowNumber]->setAziZenRad(newP);
                if (this->mSpeakersTableListBox.getNumSelectedRows() > 1) {
                    for (int i = 0; i < this->mSpeakersTableListBox.getSelectedRows().size(); i++) {
                        int rownum = this->mSpeakersTableListBox.getSelectedRows()[i];
                        if (rownum == rowNumber) {
                            continue;
                        }
                        newP = this->mMainContentComponent.getListSpeaker()[rownum]->getAziZenRad();
                        if (altDown) {
                            newP.x += diff;
                            while (newP.x > 360.0f) {
                                newP.x -= 360.0f;
                            }
                            while (newP.x < 0.0f) {
                                newP.x += 360.0f;
                            }
                        } else {
                            newP.x = val;
                        }
                        this->mMainContentComponent.getListSpeaker()[rownum]->setAziZenRad(newP);
                    }
                }
                break;
            case 6: // Elevation
                newP = this->mMainContentComponent.getListSpeaker()[rowNumber]->getAziZenRad();
                val = GetFloatPrecision(newText.getFloatValue(), 3);
                diff = val - newP.y;
                if (val < -90.0f) {
                    val = -90.0f;
                } else if (val > 90.0f) {
                    val = 90.0f;
                }
                newP.y = val;
                this->mMainContentComponent.getListSpeaker()[rowNumber]->setAziZenRad(newP);
                if (this->mSpeakersTableListBox.getNumSelectedRows() > 1) {
                    for (int i = 0; i < this->mSpeakersTableListBox.getSelectedRows().size(); i++) {
                        int rownum = this->mSpeakersTableListBox.getSelectedRows()[i];
                        if (rownum == rowNumber) {
                            continue;
                        }
                        newP = this->mMainContentComponent.getListSpeaker()[rownum]->getAziZenRad();
                        if (altDown) {
                            newP.y += diff;
                            if (newP.y < -90.0f) {
                                newP.y = -90.0f;
                            } else if (newP.y > 90.0f) {
                                newP.y = 90.0f;
                            }
                        } else {
                            newP.y = val;
                        }
                        this->mMainContentComponent.getListSpeaker()[rownum]->setAziZenRad(newP);
                    }
                }
                break;
            case 7: // Distance
                newP = this->mMainContentComponent.getListSpeaker()[rowNumber]->getAziZenRad();
                if (this->mMainContentComponent.isRadiusNormalized()
                    && !this->mMainContentComponent.getListSpeaker()[rowNumber]->isDirectOut()) {
                    val = 1.0;
                } else {
                    val = GetFloatPrecision(newText.getFloatValue(), 3);
                }
                diff = val - newP.z;
                if (val < 0.0f) {
                    val = 0.0f;
                } else if (val > 2.5f) {
                    val = 2.5f;
                }
                newP.z = val;
                this->mMainContentComponent.getListSpeaker()[rowNumber]->setAziZenRad(newP);
                if (this->mSpeakersTableListBox.getNumSelectedRows() > 1) {
                    for (int i = 0; i < this->mSpeakersTableListBox.getSelectedRows().size(); i++) {
                        int rownum = this->mSpeakersTableListBox.getSelectedRows()[i];
                        if (rownum == rowNumber) {
                            continue;
                        }
                        newP = this->mMainContentComponent.getListSpeaker()[rownum]->getAziZenRad();
                        if (altDown) {
                            newP.z += diff;
                            if (newP.z < 0.0f) {
                                newP.z = 0.0f;
                            } else if (newP.z > 2.5f) {
                                newP.z = 2.5f;
                            }
                        } else {
                            newP.z = val;
                        }
                        this->mMainContentComponent.getListSpeaker()[rownum]->setAziZenRad(newP);
                    }
                }
                break;
            case 8: // Output patch
                this->mMainContentComponent.setShowTriplets(false);
                oldval = this->mMainContentComponent.getListSpeaker()[rowNumber]->getOutputPatch();
                ival = newText.getIntValue();
                if (ival < 0) {
                    ival = 0;
                } else if (ival > 256) {
                    ival = 256;
                }
                if (!this->mMainContentComponent.getListSpeaker()[rowNumber]->isDirectOut()) {
                    for (auto && it : this->mMainContentComponent.getListSpeaker()) {
                        if (it == this->mMainContentComponent.getListSpeaker()[rowNumber] || it->isDirectOut()) {
                            continue;
                        }
                        if (it->getOutputPatch() == ival) {
                            juce::AlertWindow alert("Wrong output patch!    ",
                                                    "Sorry! Output patch number " + juce::String(ival)
                                                        + " is already used.",
                                                    juce::AlertWindow::WarningIcon);
                            alert.setLookAndFeel(&this->mLookAndFeel);
                            alert.addButton("OK", 0, juce::KeyPress(juce::KeyPress::returnKey));
                            alert.runModalLoop();
                            ival = oldval;
                        }
                    }
                }
                this->mMainContentComponent.getListSpeaker()[rowNumber]->setOutputPatch(ival);
                break;
            case 9: // Gain
                val = newText.getFloatValue();
                diff = val - this->mMainContentComponent.getListSpeaker()[rowNumber]->getGain();
                if (val < -18.0f) {
                    val = -18.0f;
                } else if (val > 6.0f) {
                    val = 6.0f;
                }
                this->mMainContentComponent.getListSpeaker()[rowNumber]->setGain(val);
                if (this->mSpeakersTableListBox.getNumSelectedRows() > 1) {
                    for (int i = 0; i < this->mSpeakersTableListBox.getSelectedRows().size(); i++) {
                        int rownum = this->mSpeakersTableListBox.getSelectedRows()[i];
                        if (rownum == rowNumber) {
                            continue;
                        }
                        if (altDown) {
                            float g = this->mMainContentComponent.getListSpeaker()[rownum]->getGain() + diff;
                            if (g < -18.0f) {
                                g = -18.0f;
                            } else if (g > 6.0f) {
                                g = 6.0f;
                            }
                            this->mMainContentComponent.getListSpeaker()[rownum]->setGain(g);
                        } else {
                            this->mMainContentComponent.getListSpeaker()[rownum]->setGain(val);
                        }
                    }
                }
                break;
            case 10: // Filter Cutoff
                val = newText.getFloatValue();
                diff = val - this->mMainContentComponent.getListSpeaker()[rowNumber]->getHighPassCutoff();
                if (val < 0.0f) {
                    val = 0.0f;
                } else if (val > 150.0f) {
                    val = 150.0f;
                }
                this->mMainContentComponent.getListSpeaker()[rowNumber]->setHighPassCutoff(val);
                if (this->mSpeakersTableListBox.getNumSelectedRows() > 1) {
                    for (int i = 0; i < this->mSpeakersTableListBox.getSelectedRows().size(); i++) {
                        int rownum = this->mSpeakersTableListBox.getSelectedRows()[i];
                        if (rownum == rowNumber) {
                            continue;
                        }
                        if (altDown) {
                            float g = this->mMainContentComponent.getListSpeaker()[rownum]->getHighPassCutoff() + diff;
                            if (g < 0.0f) {
                                g = 0.0f;
                            } else if (g > 150.0f) {
                                g = 150.0f;
                            }
                            this->mMainContentComponent.getListSpeaker()[rownum]->setHighPassCutoff(g);
                        } else {
                            this->mMainContentComponent.getListSpeaker()[rownum]->setHighPassCutoff(val);
                        }
                    }
                }
                break;
            case 11: // Direct Out
                this->mMainContentComponent.setShowTriplets(false);
                this->mMainContentComponent.getListSpeaker()[rowNumber]->setDirectOut(newText.getIntValue());
                break;
            }
        }
        this->updateWinContent();
        this->mMainContentComponent.needToComputeVbap = true;
        this->mMainContentComponent.getLockSpeakers().unlock();
    }
}

//==============================================================================
// This is overloaded from TableListBoxModel, and should fill in the background of the whole row.
void EditSpeakersWindow::paintRowBackground(juce::Graphics & g,
                                            int const rowNumber,
                                            int const width,
                                            int const height,
                                            bool const rowIsSelected)
{
    juce::ignoreUnused(width);
    juce::ignoreUnused(height);

    if (rowIsSelected) {
        if (this->mMainContentComponent.getLockSpeakers().try_lock()) {
            this->mMainContentComponent.getListSpeaker()[rowNumber]->selectSpeaker();
            this->mMainContentComponent.getLockSpeakers().unlock();
        }
        g.fillAll(this->mLookAndFeel.getHighlightColour());
    } else {
        if (this->mMainContentComponent.getLockSpeakers().try_lock()) {
            this->mMainContentComponent.getListSpeaker()[rowNumber]->unSelectSpeaker();
            this->mMainContentComponent.getLockSpeakers().unlock();
        }
        if (rowNumber % 2) {
            g.fillAll(this->mLookAndFeel.getBackgroundColour().withBrightness(0.6));
        } else {
            g.fillAll(this->mLookAndFeel.getBackgroundColour().withBrightness(0.7));
        }
    }
}

//==============================================================================
// This is overloaded from TableListBoxModel, and must paint any cells that aren't using custom components.
void EditSpeakersWindow::paintCell(juce::Graphics & g,
                                   int rowNumber,
                                   int columnId,
                                   int width,
                                   int height,
                                   bool /*rowIsSelected*/)
{
    g.setColour(juce::Colours::black);
    g.setFont(mFont);

    if (this->mMainContentComponent.getLockSpeakers().try_lock()) {
        if (this->mMainContentComponent.getListSpeaker().size() > static_cast<unsigned int>(rowNumber)) {
            juce::String text = getText(columnId, rowNumber);
            g.drawText(text, 2, 0, width - 4, height, juce::Justification::centredLeft, true);
        }
        this->mMainContentComponent.getLockSpeakers().unlock();
    }
    g.setColour(juce::Colours::black.withAlpha(0.2f));
    g.fillRect(width - 1, 0, 1, height);
}

//==============================================================================
juce::Component * EditSpeakersWindow::refreshComponentForCell(int const rowNumber,
                                                              int const columnId,
                                                              bool const isRowSelected,
                                                              Component * existingComponentToUpdate)
{
    juce::ignoreUnused(isRowSelected);

    if (columnId == 11) {
        juce::ToggleButton * tbDirect = static_cast<juce::ToggleButton *>(existingComponentToUpdate);
        if (tbDirect == nullptr)
            tbDirect = new juce::ToggleButton();
        tbDirect->setName(juce::String(rowNumber + 1000));
        tbDirect->setClickingTogglesState(true);
        tbDirect->setBounds(4, 404, 88, 22);
        tbDirect->addListener(this);
        tbDirect->setToggleState(this->mMainContentComponent.getListSpeaker()[rowNumber]->isDirectOut(),
                                 juce::dontSendNotification);
        tbDirect->setLookAndFeel(&this->mLookAndFeel);
        return tbDirect;
    }
    if (columnId == 12) {
        juce::TextButton * tbRemove = static_cast<juce::TextButton *>(existingComponentToUpdate);
        if (tbRemove == nullptr)
            tbRemove = new juce::TextButton();
        tbRemove->setButtonText("X");
        tbRemove->setName(juce::String(rowNumber));
        tbRemove->setBounds(4, 404, 88, 22);
        tbRemove->addListener(this);
        tbRemove->setColour(juce::ToggleButton::textColourId, this->mLookAndFeel.getFontColour());
        tbRemove->setLookAndFeel(&this->mLookAndFeel);
        return tbRemove;
    }

    // The other columns are editable text columns, for which we use the custom juce::Label component
    EditableTextCustomComponent * textLabel = static_cast<EditableTextCustomComponent *>(existingComponentToUpdate);
    if (textLabel == nullptr)
        textLabel = new EditableTextCustomComponent(*this);

    textLabel->setRowAndColumn(rowNumber, columnId);

    if (this->mMainContentComponent.getModeSelected() == LBAP
        || this->mMainContentComponent.getListSpeaker()[rowNumber]->isDirectOut()) {
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
int EditSpeakersWindow::getModeSelected() const
{
    return this->mMainContentComponent.getModeSelected();
}

//==============================================================================
bool EditSpeakersWindow::getDirectOutForSpeakerRow(int const row) const
{
    return this->mMainContentComponent.getListSpeaker()[row]->isDirectOut();
}