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
#include "Speaker.h"
#include "narrow.hpp"

namespace cols
{
    static constexpr auto ID = 1;
    static constexpr auto X = 2;
    static constexpr auto Y = 3;
    static constexpr auto Z = 4;
    static constexpr auto AZIMUTH = 5;
    static constexpr auto ELEVATION = 6;
    static constexpr auto DISTANCE = 7;
    static constexpr auto OUTPUT_PATCH = 8;
    static constexpr auto GAIN = 9;
    static constexpr auto HIGHPASS = 10;
    static constexpr auto DIRECT = 11;
    static constexpr auto DELETE = 12;
}


//==============================================================================
template<typename T>
static T getFloatPrecision(T const value, T const precision)
{
    static_assert(std::is_floating_point_v<T>);
    return std::floor((value * std::pow(narrow<T>(10), precision) + narrow<T>(0.5)))
           / std::pow(narrow<T>(10), precision);
}

//==============================================================================
EditSpeakersWindow::EditSpeakersWindow(juce::String const & name,
                                       GrisLookAndFeel & lookAndFeel,
                                       MainContentComponent & mainContentComponent,
                                       juce::String const & /*configName*/)
    : juce::DocumentWindow(name, lookAndFeel.getBackgroundColour(), DocumentWindow::allButtons)
    , mMainContentComponent(mainContentComponent)
    , mLookAndFeel(lookAndFeel)
    , mListSpeakerBox(lookAndFeel, "Configuration Speakers")
    , mFont(14.0f)
{
    mAddSpeakerButton.setButtonText("Add Speaker");
    mAddSpeakerButton.setBounds(5, 404, 100, 22);
    mAddSpeakerButton.addListener(this);
    mAddSpeakerButton.setColour(juce::ToggleButton::textColourId, mLookAndFeel.getFontColour());
    mAddSpeakerButton.setLookAndFeel(&mLookAndFeel);
    mListSpeakerBox.getContent()->addAndMakeVisible(mAddSpeakerButton);

    mCompSpeakersButton.setButtonText("Compute");
    mCompSpeakersButton.setBounds(110, 404, 100, 22);
    mCompSpeakersButton.addListener(this);
    mCompSpeakersButton.setColour(juce::ToggleButton::textColourId, mLookAndFeel.getFontColour());
    mCompSpeakersButton.setLookAndFeel(&mLookAndFeel);
    mListSpeakerBox.getContent()->addAndMakeVisible(mCompSpeakersButton);

    // Generate ring of speakers.
    static auto constexpr WLAB{ 80 };

    mNumOfSpeakersLabel.setText("# of speakers", juce::NotificationType::dontSendNotification);
    mNumOfSpeakersLabel.setJustificationType(juce::Justification::right);
    mNumOfSpeakersLabel.setFont(mLookAndFeel.getFont());
    mNumOfSpeakersLabel.setLookAndFeel(&mLookAndFeel);
    mNumOfSpeakersLabel.setColour(juce::Label::textColourId, mLookAndFeel.getFontColour());
    mNumOfSpeakersLabel.setBounds(5, 435, 40, 24);
    mListSpeakerBox.getContent()->addAndMakeVisible(mNumOfSpeakersLabel);

    mNumOfSpeakersTextEditor.setTooltip("Number of speakers in the ring");
    mNumOfSpeakersTextEditor.setColour(juce::ToggleButton::textColourId, mLookAndFeel.getFontColour());
    mNumOfSpeakersTextEditor.setLookAndFeel(&mLookAndFeel);
    mNumOfSpeakersTextEditor.setBounds(5 + WLAB, 435, 40, 24);
    mNumOfSpeakersTextEditor.addListener(&mMainContentComponent);
    mListSpeakerBox.getContent()->addAndMakeVisible(mNumOfSpeakersTextEditor);

    mNumOfSpeakersTextEditor.setText("8");
    mNumOfSpeakersTextEditor.setInputRestrictions(3, "0123456789");
    mNumOfSpeakersTextEditor.addListener(this);

    mZenithLabel.setText("Elevation", juce::NotificationType::dontSendNotification);
    mZenithLabel.setJustificationType(juce::Justification::right);
    mZenithLabel.setFont(mLookAndFeel.getFont());
    mZenithLabel.setLookAndFeel(&mLookAndFeel);
    mZenithLabel.setColour(juce::Label::textColourId, mLookAndFeel.getFontColour());
    mZenithLabel.setBounds(105, 435, 80, 24);
    mListSpeakerBox.getContent()->addAndMakeVisible(mZenithLabel);

    mZenithTextEditor.setTooltip("Elevation angle of the ring");
    mZenithTextEditor.setColour(juce::ToggleButton::textColourId, mLookAndFeel.getFontColour());
    mZenithTextEditor.setLookAndFeel(&mLookAndFeel);
    mZenithTextEditor.setBounds(105 + WLAB, 435, 60, 24);
    mZenithTextEditor.addListener(&mMainContentComponent);
    mListSpeakerBox.getContent()->addAndMakeVisible(mZenithTextEditor);

    mZenithTextEditor.setText("0.0");
    mZenithTextEditor.setInputRestrictions(6, "-0123456789.");
    mZenithTextEditor.addListener(this);

    mRadiusLabel.setText("Distance", juce::NotificationType::dontSendNotification);
    mRadiusLabel.setJustificationType(juce::Justification::right);
    mRadiusLabel.setFont(mLookAndFeel.getFont());
    mRadiusLabel.setLookAndFeel(&mLookAndFeel);
    mRadiusLabel.setColour(juce::Label::textColourId, mLookAndFeel.getFontColour());
    mRadiusLabel.setBounds(230, 435, 80, 24);
    mListSpeakerBox.getContent()->addAndMakeVisible(mRadiusLabel);

    mRadiusTextEditor.setTooltip("Distance of the speakers from the center.");
    mRadiusTextEditor.setColour(juce::ToggleButton::textColourId, mLookAndFeel.getFontColour());
    mRadiusTextEditor.setLookAndFeel(&mLookAndFeel);
    mRadiusTextEditor.setBounds(230 + WLAB, 435, 60, 24);
    mRadiusTextEditor.addListener(&mMainContentComponent);
    mListSpeakerBox.getContent()->addAndMakeVisible(mRadiusTextEditor);

    mRadiusTextEditor.setText("1.0");
    mRadiusTextEditor.setInputRestrictions(6, "0123456789.");
    mRadiusTextEditor.addListener(this);

    mOffsetAngleLabel.setText("Offset Angle", juce::NotificationType::dontSendNotification);
    mOffsetAngleLabel.setJustificationType(juce::Justification::right);
    mOffsetAngleLabel.setFont(mLookAndFeel.getFont());
    mOffsetAngleLabel.setLookAndFeel(&mLookAndFeel);
    mOffsetAngleLabel.setColour(juce::Label::textColourId, mLookAndFeel.getFontColour());
    mOffsetAngleLabel.setBounds(375, 435, 80, 24);
    mListSpeakerBox.getContent()->addAndMakeVisible(mOffsetAngleLabel);

    mOffsetAngleTextEditor.setTooltip("Offset angle of the first speaker.");
    mOffsetAngleTextEditor.setColour(juce::ToggleButton::textColourId, mLookAndFeel.getFontColour());
    mOffsetAngleTextEditor.setLookAndFeel(&mLookAndFeel);
    mOffsetAngleTextEditor.setBounds(375 + WLAB, 435, 60, 24);
    mOffsetAngleTextEditor.addListener(&mMainContentComponent);
    mListSpeakerBox.getContent()->addAndMakeVisible(mOffsetAngleTextEditor);

    mOffsetAngleTextEditor.setText("0.0");
    mOffsetAngleTextEditor.setInputRestrictions(6, "-0123456789.");
    mOffsetAngleTextEditor.addListener(this);

    mAddRingButton.setButtonText("Add Ring");
    mAddRingButton.setBounds(520, 435, 100, 24);
    mAddRingButton.addListener(this);
    mAddRingButton.setColour(juce::ToggleButton::textColourId, mLookAndFeel.getFontColour());
    mAddRingButton.setLookAndFeel(&mLookAndFeel);
    mListSpeakerBox.getContent()->addAndMakeVisible(mAddRingButton);

    // Pink noise controls.
    mPinkNoiseToggleButton.setButtonText("Reference Pink Noise");
    mPinkNoiseToggleButton.setBounds(5, 500, 150, 24);
    mPinkNoiseToggleButton.addListener(this);
    mPinkNoiseToggleButton.setColour(juce::ToggleButton::textColourId, mLookAndFeel.getFontColour());
    mPinkNoiseToggleButton.setLookAndFeel(&mLookAndFeel);
    mListSpeakerBox.getContent()->addAndMakeVisible(mPinkNoiseToggleButton);

    mPinkNoiseGainSlider.setTextValueSuffix(" dB");
    mPinkNoiseGainSlider.setBounds(200, 500, 60, 60);
    mPinkNoiseGainSlider.setSliderStyle(juce::Slider::Rotary);
    mPinkNoiseGainSlider.setRotaryParameters(juce::MathConstants<float>::pi * 1.3f,
                                             juce::MathConstants<float>::pi * 2.7f,
                                             true);
    mPinkNoiseGainSlider.setRange(-60, -3, 1);
    mPinkNoiseGainSlider.setValue(-20);
    mPinkNoiseGainSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 20);
    mPinkNoiseGainSlider.setColour(juce::ToggleButton::textColourId, mLookAndFeel.getFontColour());
    mPinkNoiseGainSlider.setLookAndFeel(&mLookAndFeel);
    mPinkNoiseGainSlider.addListener(this);
    mListSpeakerBox.getContent()->addAndMakeVisible(mPinkNoiseGainSlider);

    mListSpeakerBox.getContent()->addAndMakeVisible(mSpeakersTableListBox);

    mListSpeakerBox.repaint();
    mListSpeakerBox.resized();

    setContentNonOwned(&mListSpeakerBox, false);
}

//==============================================================================
void EditSpeakersWindow::initComp()
{
    mSpeakersTableListBox.setModel(this);

    mSpeakersTableListBox.setColour(juce::ListBox::outlineColourId, mLookAndFeel.getWinBackgroundColour());
    mSpeakersTableListBox.setColour(juce::ListBox::backgroundColourId, mLookAndFeel.getWinBackgroundColour());
    mSpeakersTableListBox.setOutlineThickness(1);

    auto & header{ mSpeakersTableListBox.getHeader() };
    header.addColumn("ID", 1, 40, 40, 60, juce::TableHeaderComponent::defaultFlags);
    header.addColumn("X", 2, 70, 50, 120, juce::TableHeaderComponent::defaultFlags);
    header.addColumn("Y", 3, 70, 50, 120, juce::TableHeaderComponent::defaultFlags);
    header.addColumn("Z", 4, 70, 50, 120, juce::TableHeaderComponent::defaultFlags);
    header.addColumn("Azimuth", 5, 70, 50, 120, juce::TableHeaderComponent::defaultFlags);
    header.addColumn("Elevation", 6, 70, 50, 120, juce::TableHeaderComponent::defaultFlags);
    header.addColumn("Distance", 7, 70, 50, 120, juce::TableHeaderComponent::defaultFlags);
    header.addColumn("Output", 8, 70, 50, 120, juce::TableHeaderComponent::defaultFlags);
    header.addColumn("Gain (dB)", 9, 70, 50, 120, juce::TableHeaderComponent::notSortable);
    header.addColumn("Highpass", 10, 70, 50, 120, juce::TableHeaderComponent::notSortable);
    header.addColumn("Direct", 11, 70, 50, 120, juce::TableHeaderComponent::notSortable);
    header.addColumn("delete", 12, 70, 50, 120, juce::TableHeaderComponent::notSortable);

    header.setSortColumnId(1, true); // Sort forwards by the ID column.

    mSpeakersTableListBox.setMultipleSelectionEnabled(true);

    mNumRows = narrow<unsigned>(mMainContentComponent.getSpeakers().size());

    mListSpeakerBox.setBounds(0, 0, getWidth(), getHeight());
    mListSpeakerBox.correctSize(getWidth() - 8, getHeight());
    mSpeakersTableListBox.setSize(getWidth(), 400);

    mSpeakersTableListBox.updateContent();

    mListSpeakerBox.repaint();
    mListSpeakerBox.resized();
    resized();
}

//==============================================================================
struct Sorter {
    int id;
    float value;
    bool directOut;
};

//==============================================================================
bool compareLessThan(Sorter const & a, Sorter const & b)
{
    if (a.directOut && b.directOut) {
        return a.id < b.id;
    }
    if (a.directOut) {
        return false;
    }
    if (b.directOut) {
        return true;
    }
    if (a.value == b.value) {
        return a.id < b.id;
    }

    return a.value < b.value;
}

//==============================================================================
bool compareGreaterThan(Sorter const & a, Sorter const & b)
{
    if (a.directOut && b.directOut) {
        return a.id > b.id;
    }
    if (a.directOut) {
        return false;
    }
    if (b.directOut) {
        return true;
    }
    if (a.value == b.value) {
        return a.id > b.id;
    }

    return a.value > b.value;
}

//==============================================================================
void EditSpeakersWindow::sortOrderChanged(int const newSortColumnId, bool const isForwards)
{
    struct Sorter toSort[MAX_OUTPUTS];

    auto & speakers{ mMainContentComponent.getSpeakers() };
    unsigned index{};
    for (auto const * speaker : speakers) {
        auto & toSortItem{ toSort[index++] };
        toSortItem.id = speaker->getIdSpeaker();
        toSortItem.directOut = speaker->isDirectOut();
        switch (newSortColumnId) {
        case cols::ID:
            toSortItem.value = static_cast<float>(speaker->getIdSpeaker());
            break;
        case cols::X:
            toSortItem.value = speaker->getCoordinate().z;
            break;
        case cols::Y:
            toSortItem.value = speaker->getCoordinate().x;
            break;
        case cols::Z:
            toSortItem.value = speaker->getCoordinate().y;
            break;
        case cols::AZIMUTH:
            toSortItem.value = speaker->getAziZenRad().x;
            break;
        case cols::ELEVATION:
            toSortItem.value = speaker->getAziZenRad().y;
            break;
        case cols::DISTANCE:
            toSortItem.value = speaker->getAziZenRad().z;
            break;
        case cols::OUTPUT_PATCH:
            toSortItem.value = static_cast<float>(speaker->getOutputPatch());
            break;
        default:
            jassertfalse;
        }
    }

    auto const size{ narrow<unsigned>(mMainContentComponent.getSpeakers().size()) };
    if (isForwards) {
        std::sort(toSort, toSort + size, compareLessThan);
    } else {
        std::sort(toSort, toSort + size, compareGreaterThan);
    }

    std::vector<int> newOrder{};
    newOrder.resize(size);
    for (unsigned i{}; i < size; ++i) {
        newOrder[i] = toSort[i].id;
    }
    mMainContentComponent.reorderSpeakers(newOrder);
    updateWinContent();
}

//==============================================================================
void EditSpeakersWindow::sliderValueChanged(juce::Slider * slider)
{
    if (slider == &mPinkNoiseGainSlider) {
        auto const sliderValue{ static_cast<float>(mPinkNoiseGainSlider.getValue()) };
        auto const gain{ std::pow(10.0f, sliderValue / 20.0f) };
        mMainContentComponent.getJackClient()->setPinkNoiseGain(gain);
    }
}

//==============================================================================
void EditSpeakersWindow::buttonClicked(juce::Button * button)
{
    auto const tripletState{ mMainContentComponent.isTripletsShown() };
    auto const sortColumnId{ mSpeakersTableListBox.getHeader().getSortColumnId() };
    auto const sortedForwards{ mSpeakersTableListBox.getHeader().isSortedForwards() };
    auto selectedRow{ mSpeakersTableListBox.getSelectedRow() };

    mMainContentComponent.setShowTriplets(false);

    if (button == &mAddSpeakerButton) {
        if (selectedRow == -1 || selectedRow == (mNumRows - 1)) {
            mMainContentComponent.addSpeaker(sortColumnId, sortedForwards);
            updateWinContent();
            mSpeakersTableListBox.selectRow(getNumRows() - 1);
        } else {
            mMainContentComponent.insertSpeaker(selectedRow, sortColumnId, sortedForwards);
            updateWinContent();
            mSpeakersTableListBox.selectRow(selectedRow + 1);
        }
        mSpeakersTableListBox.getHeader().setSortColumnId(sortColumnId, sortedForwards);
        mMainContentComponent.setNeedToComputeVbap(true);
    } else if (button == &mCompSpeakersButton) {
        if (mMainContentComponent.updateLevelComp()) {
            mMainContentComponent.setShowTriplets(tripletState);
        }
    } else if (button == &mAddRingButton) {
        for (int i{}; i < mNumOfSpeakersTextEditor.getText().getIntValue(); i++) {
            if (selectedRow == -1 || selectedRow == (mNumRows - 1)) {
                mMainContentComponent.addSpeaker(sortColumnId, sortedForwards);
                mNumRows = mMainContentComponent.getSpeakers().size();
                selectedRow = mNumRows - 1;
            } else {
                mMainContentComponent.insertSpeaker(selectedRow, sortColumnId, sortedForwards);
                selectedRow += 1;
                mNumRows = mMainContentComponent.getSpeakers().size();
            }

            auto azimuth{ 360.0f / mNumOfSpeakersTextEditor.getText().getIntValue() * i
                          + mOffsetAngleTextEditor.getText().getFloatValue() };
            if (azimuth > 360.0f) {
                azimuth -= 360.0f;
            } else if (azimuth < 0.0f) {
                azimuth += 360.0f;
            }
            auto const zenith{ mZenithTextEditor.getText().getFloatValue() };
            auto const radius{ mRadiusTextEditor.getText().getFloatValue() };
            mMainContentComponent.getSpeakers()[selectedRow]->setAziZenRad(glm::vec3(azimuth, zenith, radius));
        }
        updateWinContent();
        mSpeakersTableListBox.selectRow(selectedRow);
        // TableList needs different sorting parameters to trigger the sorting function.
        mSpeakersTableListBox.getHeader().setSortColumnId(sortColumnId, !sortedForwards);
        // This is the real sorting!
        mSpeakersTableListBox.getHeader().setSortColumnId(sortColumnId, sortedForwards);
        mMainContentComponent.setNeedToComputeVbap(true);
    } else if (button == &mPinkNoiseToggleButton) {
        mMainContentComponent.getJackClient()->setPinkNoiseActive(mPinkNoiseToggleButton.getToggleState());
    } else if (button->getName().isNotEmpty()
               && (button->getName().getIntValue() >= 0
                   && button->getName().getIntValue() <= mMainContentComponent.getSpeakers().size())) {
        if (mSpeakersTableListBox.getNumSelectedRows() > 1
            && mSpeakersTableListBox.getSelectedRows().contains(button->getName().getIntValue())) {
            for (auto i{ mSpeakersTableListBox.getSelectedRows().size() - 1 }; i >= 0; --i) {
                auto const rowNumber{ mSpeakersTableListBox.getSelectedRows()[i] };
                mMainContentComponent.removeSpeaker(rowNumber);
            }
        } else {
            mMainContentComponent.removeSpeaker(button->getName().getIntValue());
        }
        mMainContentComponent.resetSpeakerIds();
        updateWinContent();
        mSpeakersTableListBox.deselectAllRows();
        mMainContentComponent.setNeedToComputeVbap(true);
    } else {
        auto const row{ button->getName().getIntValue() - 1000 };
        mMainContentComponent.getSpeakers()[row]->setDirectOut(button->getToggleState());
        if (mSpeakersTableListBox.getNumSelectedRows() > 1) {
            for (int i{}; i < mSpeakersTableListBox.getSelectedRows().size(); ++i) {
                auto const rowNumber{ mSpeakersTableListBox.getSelectedRows()[i] };
                mMainContentComponent.getSpeakers()[rowNumber]->setDirectOut(button->getToggleState());
                auto * tog{ dynamic_cast<juce::ToggleButton *>(mSpeakersTableListBox.getCellComponent(cols::DIRECT, rowNumber)) };
                if (tog) {
                    tog->setToggleState(button->getToggleState(), juce::NotificationType::dontSendNotification);
                }
            }
        }
        updateWinContent();
        mMainContentComponent.setNeedToComputeVbap(true);
    }
}

//==============================================================================
void EditSpeakersWindow::textEditorTextChanged(juce::TextEditor & editor)
{
    float value;
    juce::String test;
    if (&editor == &mNumOfSpeakersTextEditor) {
    } else if (&editor == &mZenithTextEditor) {
        test = mZenithTextEditor.getText().retainCharacters(".");
        if (test.length() > 1) {
            mZenithTextEditor.setText(mZenithTextEditor.getText().dropLastCharacters(1), false);
        }
        value = mZenithTextEditor.getText().getFloatValue();
        if (value > 90.0f) {
            mZenithTextEditor.setText(juce::String(90.0f), false);
        } else if (value < -90.0f) {
            mZenithTextEditor.setText(juce::String(-90.0f), false);
        }
    } else if (&editor == &mRadiusTextEditor) {
        test = mRadiusTextEditor.getText().retainCharacters(".");
        if (test.length() > 1) {
            mRadiusTextEditor.setText(mRadiusTextEditor.getText().dropLastCharacters(1), false);
        }
        value = mRadiusTextEditor.getText().getFloatValue();
        if (value > 1.0f) {
            mRadiusTextEditor.setText(juce::String(1.0f), false);
        }
    } else if (&editor == &mOffsetAngleTextEditor) {
        test = mOffsetAngleTextEditor.getText().retainCharacters(".");
        if (test.length() > 1) {
            mOffsetAngleTextEditor.setText(mOffsetAngleTextEditor.getText().dropLastCharacters(1), false);
        }
        value = mOffsetAngleTextEditor.getText().getFloatValue();
        if (value < -180.0f) {
            mOffsetAngleTextEditor.setText(juce::String(-180.0f), false);
        } else if (value > 180.0f) {
            mOffsetAngleTextEditor.setText(juce::String(180.0f), false);
        }
    }
}

//==============================================================================
void EditSpeakersWindow::textEditorReturnKeyPressed(juce::TextEditor & /*textEditor*/)
{
    unfocusAllComponents();
}

//==============================================================================
void EditSpeakersWindow::updateWinContent()
{
    mNumRows = mMainContentComponent.getSpeakers().size();
    mSpeakersTableListBox.updateContent();
    if (mInitialized) {
        mMainContentComponent.setNeedToSaveSpeakerSetup(true);
    }
    mInitialized = true;
}

//==============================================================================
void EditSpeakersWindow::selectedRow(int const value)
{
    juce::MessageManagerLock const mmLock{};
    mSpeakersTableListBox.selectRow(value);
    repaint();
}

//==============================================================================
void EditSpeakersWindow::closeButtonPressed()
{
    auto exitV{ 1 };
    if (mMainContentComponent.needToSaveSpeakerSetup()) {
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
            mMainContentComponent.updateLevelComp();
            mMainContentComponent.handleTimer(false);
            setAlwaysOnTop(false);
            mMainContentComponent.handleSaveAsSpeakerSetup();
            mMainContentComponent.handleTimer(true);
        } else if (exitV == 2) {
            alert.setVisible(false);
            mMainContentComponent.reloadXmlFileSpeaker();
            mMainContentComponent.updateLevelComp();
        }
    }
    if (exitV) {
        mMainContentComponent.getJackClient()->setPinkNoiseActive(false);
        mMainContentComponent.closeSpeakersConfigurationWindow();
    }
}

//==============================================================================
void EditSpeakersWindow::resized()
{
    juce::DocumentWindow::resized();

    mSpeakersTableListBox.setSize(getWidth(), getHeight() - 195);

    mListSpeakerBox.setSize(getWidth(), getHeight());
    mListSpeakerBox.correctSize(getWidth() - 10, getHeight() - 30);

    mAddSpeakerButton.setBounds(5, getHeight() - 180, 100, 22);
    mCompSpeakersButton.setBounds(getWidth() - 105, getHeight() - 180, 100, 22);

    mNumOfSpeakersLabel.setBounds(5, getHeight() - 140, 80, 24);
    mNumOfSpeakersTextEditor.setBounds(5 + 80, getHeight() - 140, 40, 24);
    mZenithLabel.setBounds(120, getHeight() - 140, 80, 24);
    mZenithTextEditor.setBounds(120 + 80, getHeight() - 140, 60, 24);
    mRadiusLabel.setBounds(255, getHeight() - 140, 80, 24);
    mRadiusTextEditor.setBounds(255 + 80, getHeight() - 140, 60, 24);
    mOffsetAngleLabel.setBounds(400, getHeight() - 140, 80, 24);
    mOffsetAngleTextEditor.setBounds(400 + 80, getHeight() - 140, 60, 24);
    mAddRingButton.setBounds(getWidth() - 105, getHeight() - 140, 100, 24);

    mPinkNoiseToggleButton.setBounds(5, getHeight() - 75, 150, 24);
    mPinkNoiseGainSlider.setBounds(180, getHeight() - 100, 60, 60);
}

//==============================================================================
juce::String EditSpeakersWindow::getText(int const columnNumber, int const rowNumber) const
{
    juce::String text{};
    auto & speakers{ mMainContentComponent.getSpeakers() };
    if (mMainContentComponent.getSpeakers().size() > rowNumber) {
        auto & speaker{ *speakers[rowNumber] };
        switch (columnNumber) {
        case cols::ID:
            text = juce::String{ speaker.getIdSpeaker() };
            break;
        case cols::X:
            text = juce::String{ speaker.getCoordinate().z };
            break;
        case cols::Y:
            text = juce::String{ speaker.getCoordinate().x };
            break;
        case cols::Z:
            text = juce::String{ speaker.getCoordinate().y };
            break;
        case cols::AZIMUTH:
            text = juce::String{ speaker.getAziZenRad().x };
            break;
        case cols::ELEVATION:
            text = juce::String{ speaker.getAziZenRad().y };
            break;
        case cols::DISTANCE:
            text = juce::String{ speaker.getAziZenRad().z };
            break;
        case cols::OUTPUT_PATCH:
            text = juce::String{ speaker.getOutputPatch() };
            break;
        case cols::GAIN:
            text = juce::String{ speaker.getGain() };
            break;
        case cols::HIGHPASS:
            text = juce::String{ speaker.getHighPassCutoff() };
            break;
        case cols::DIRECT:
            text = juce::String{ static_cast<int>(speaker.isDirectOut()) };
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
    float diff;
    if (mMainContentComponent.getSpeakersLock().try_lock()) {
        if (mMainContentComponent.getSpeakers().size() > rowNumber) {
            glm::vec3 newP;
            switch (columnNumber) {
            case cols::X:
            {
                newP = mMainContentComponent.getSpeakers()[rowNumber]->getCoordinate();
                auto const val{ getFloatPrecision(newText.getFloatValue(), 3.0f) };
                diff = val - newP.z;
                newP.z = val;
                mMainContentComponent.getSpeakers()[rowNumber]->setCoordinate(newP);
                if (mSpeakersTableListBox.getNumSelectedRows() > 1) {
                    for (int i{}; i < mSpeakersTableListBox.getSelectedRows().size(); ++i) {
                        auto const rowNum = mSpeakersTableListBox.getSelectedRows()[i];
                        if (rowNum == rowNumber) {
                            continue;
                        }
                        newP = mMainContentComponent.getSpeakers()[rowNum]->getCoordinate();
                        if (altDown) {
                            newP.z += diff;
                        } else {
                            newP.z = val;
                        }
                        mMainContentComponent.getSpeakers()[rowNum]->setCoordinate(newP);
                    }
                }
                break;
            }
            case cols::Y:
            {
                newP = mMainContentComponent.getSpeakers()[rowNumber]->getCoordinate();
                auto const val{ getFloatPrecision(newText.getFloatValue(), 3.0f) };
                diff = val - newP.x;
                newP.x = val;
                mMainContentComponent.getSpeakers()[rowNumber]->setCoordinate(newP);
                if (mSpeakersTableListBox.getNumSelectedRows() > 1) {
                    for (int i{}; i < mSpeakersTableListBox.getSelectedRows().size(); ++i) {
                        auto const rowNum{ mSpeakersTableListBox.getSelectedRows()[i] };
                        if (rowNum == rowNumber) {
                            continue;
                        }
                        newP = mMainContentComponent.getSpeakers()[rowNum]->getCoordinate();
                        if (altDown) {
                            newP.x += diff;
                        } else {
                            newP.x = val;
                        }
                        mMainContentComponent.getSpeakers()[rowNum]->setCoordinate(newP);
                    }
                }
                break;
            }
            case cols::Z:
            {
                newP = mMainContentComponent.getSpeakers()[rowNumber]->getCoordinate();
                auto val{ getFloatPrecision(newText.getFloatValue(), 3.0f) };
                diff = val - newP.y;
                val = std::clamp(val, -1.0f, 1.0f);
                newP.y = val;
                mMainContentComponent.getSpeakers()[rowNumber]->setCoordinate(newP);
                if (mSpeakersTableListBox.getNumSelectedRows() > 1) {
                    for (int i{}; i < mSpeakersTableListBox.getSelectedRows().size(); ++i) {
                        auto const rowNum{ mSpeakersTableListBox.getSelectedRows()[i] };
                        if (rowNum == rowNumber) {
                            continue;
                        }
                        newP = mMainContentComponent.getSpeakers()[rowNum]->getCoordinate();
                        if (altDown) {
                            newP.y += diff;
                            newP.y = std::clamp(newP.y, -1.0f, 1.0f);
                        } else {
                            newP.y = val;
                        }
                        mMainContentComponent.getSpeakers()[rowNum]->setCoordinate(newP);
                    }
                }
                break;
            }
            case cols::AZIMUTH:
            {
                newP = mMainContentComponent.getSpeakers()[rowNumber]->getAziZenRad();
                auto val{ getFloatPrecision(newText.getFloatValue(), 3.0f) };
                diff = val - newP.x;
                while (val > 360.0f) {
                    val -= 360.0f;
                }
                while (val < 0.0f) {
                    val += 360.0f;
                }
                newP.x = val;
                mMainContentComponent.getSpeakers()[rowNumber]->setAziZenRad(newP);
                if (mSpeakersTableListBox.getNumSelectedRows() > 1) {
                    for (int i{}; i < mSpeakersTableListBox.getSelectedRows().size(); ++i) {
                        auto const rowNum{ mSpeakersTableListBox.getSelectedRows()[i] };
                        if (rowNum == rowNumber) {
                            continue;
                        }
                        newP = mMainContentComponent.getSpeakers()[rowNum]->getAziZenRad();
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
                        mMainContentComponent.getSpeakers()[rowNum]->setAziZenRad(newP);
                    }
                }
                break;
            }
            case cols::ELEVATION:
            {
                newP = mMainContentComponent.getSpeakers()[rowNumber]->getAziZenRad();
                auto val{ getFloatPrecision(newText.getFloatValue(), 3.0f) };
                diff = val - newP.y;
                val = std::clamp(val, -90.0f, 90.0f);
                newP.y = val;
                mMainContentComponent.getSpeakers()[rowNumber]->setAziZenRad(newP);
                if (mSpeakersTableListBox.getNumSelectedRows() > 1) {
                    for (int i{}; i < mSpeakersTableListBox.getSelectedRows().size(); ++i) {
                        auto const rowNum{ mSpeakersTableListBox.getSelectedRows()[i] };
                        if (rowNum == rowNumber) {
                            continue;
                        }
                        newP = mMainContentComponent.getSpeakers()[rowNum]->getAziZenRad();
                        if (altDown) {
                            newP.y += diff;
                            newP.y = std::clamp(newP.y, -90.0f, 90.0f);
                        } else {
                            newP.y = val;
                        }
                        mMainContentComponent.getSpeakers()[rowNum]->setAziZenRad(newP);
                    }
                }
                break;
            }
            case cols::DISTANCE:
            {
                newP = mMainContentComponent.getSpeakers()[rowNumber]->getAziZenRad();
                float val{};
                if (mMainContentComponent.isRadiusNormalized()
                    && !mMainContentComponent.getSpeakers()[rowNumber]->isDirectOut()) {
                    val = 1.0f;
                } else {
                    val = getFloatPrecision(newText.getFloatValue(), 3.0f);
                }
                diff = val - newP.z;
                val = std::clamp(val, 0.0f, 2.5f);
                newP.z = val;
                mMainContentComponent.getSpeakers()[rowNumber]->setAziZenRad(newP);
                if (mSpeakersTableListBox.getNumSelectedRows() > 1) {
                    for (int i{}; i < mSpeakersTableListBox.getSelectedRows().size(); ++i) {
                        auto const rowNum{ mSpeakersTableListBox.getSelectedRows()[i] };
                        if (rowNum == rowNumber) {
                            continue;
                        }
                        newP = mMainContentComponent.getSpeakers()[rowNum]->getAziZenRad();
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
                        mMainContentComponent.getSpeakers()[rowNum]->setAziZenRad(newP);
                    }
                }
                break;
            }
            case cols::OUTPUT_PATCH:
            {
                mMainContentComponent.setShowTriplets(false);
                auto const oldValue{ mMainContentComponent.getSpeakers()[rowNumber]->getOutputPatch() };
                auto iValue{ std::clamp(newText.getIntValue(), 0, 256) };
                if (!mMainContentComponent.getSpeakers()[rowNumber]->isDirectOut()) {
                    for (auto const * it : mMainContentComponent.getSpeakers()) {
                        if (it == mMainContentComponent.getSpeakers()[rowNumber] || it->isDirectOut()) {
                            continue;
                        }
                        if (it->getOutputPatch() == iValue) {
                            juce::AlertWindow alert("Wrong output patch!    ",
                                                    "Sorry! Output patch number " + juce::String(iValue)
                                                        + " is already used.",
                                                    juce::AlertWindow::WarningIcon);
                            alert.setLookAndFeel(&mLookAndFeel);
                            alert.addButton("OK", 0, juce::KeyPress(juce::KeyPress::returnKey));
                            alert.runModalLoop();
                            iValue = oldValue;
                        }
                    }
                }
                mMainContentComponent.getSpeakers()[rowNumber]->setOutputPatch(iValue);
                break;
            }
            case cols::GAIN:
            {
                auto val{ newText.getFloatValue() };
                diff = val - mMainContentComponent.getSpeakers()[rowNumber]->getGain();
                val = std::clamp(val, -18.0f, 6.0f);
                mMainContentComponent.getSpeakers()[rowNumber]->setGain(val);
                if (mSpeakersTableListBox.getNumSelectedRows() > 1) {
                    for (int i{}; i < mSpeakersTableListBox.getSelectedRows().size(); ++i) {
                        auto const rowNum{ mSpeakersTableListBox.getSelectedRows()[i] };
                        if (rowNum == rowNumber) {
                            continue;
                        }
                        if (altDown) {
                            auto const g{
                                std::clamp(mMainContentComponent.getSpeakers()[rowNum]->getGain() + diff, -18.0f, 6.0f)
                            };
                            mMainContentComponent.getSpeakers()[rowNum]->setGain(g);
                        } else {
                            mMainContentComponent.getSpeakers()[rowNum]->setGain(val);
                        }
                    }
                }
                break;
            }
            case cols::HIGHPASS:
            {
                auto val{ newText.getFloatValue() };
                diff = val - mMainContentComponent.getSpeakers()[rowNumber]->getHighPassCutoff();
                val = std::clamp(val, 0.0f, 150.0f);
                mMainContentComponent.getSpeakers()[rowNumber]->setHighPassCutoff(val);
                if (mSpeakersTableListBox.getNumSelectedRows() > 1) {
                    for (int i{}; i < mSpeakersTableListBox.getSelectedRows().size(); ++i) {
                        auto const rowNum{ mSpeakersTableListBox.getSelectedRows()[i] };
                        if (rowNum == rowNumber) {
                            continue;
                        }
                        if (altDown) {
                            float g = mMainContentComponent.getSpeakers()[rowNum]->getHighPassCutoff() + diff;
                            if (g < 0.0f) {
                                g = 0.0f;
                            } else if (g > 150.0f) {
                                g = 150.0f;
                            }
                            mMainContentComponent.getSpeakers()[rowNum]->setHighPassCutoff(g);
                        } else {
                            mMainContentComponent.getSpeakers()[rowNum]->setHighPassCutoff(val);
                        }
                    }
                }
                break;
            }
            case cols::DIRECT: // Direct Out
                mMainContentComponent.setShowTriplets(false);
                mMainContentComponent.getSpeakers()[rowNumber]->setDirectOut(newText.getIntValue());
                break;
            }
        }
        updateWinContent();
        mMainContentComponent.setNeedToComputeVbap(true);
        mMainContentComponent.getSpeakersLock().unlock();
    }
}

//==============================================================================
// This is overloaded from TableListBoxModel, and should fill in the background of the whole row.
void EditSpeakersWindow::paintRowBackground(juce::Graphics & g,
                                            int const rowNumber,
                                            int const /*width*/,
                                            int const /*height*/,
                                            bool const rowIsSelected)
{
    // TODO : fix the real problem and add the assertion back.
    // jassert(rowNumber < mMainContentComponent.getSpeakers().size());
    if (rowNumber >= mMainContentComponent.getSpeakers().size()) {
        return;
    }

    if (rowIsSelected) {
        if (mMainContentComponent.getSpeakersLock().try_lock()) {
            mMainContentComponent.getSpeakers()[rowNumber]->selectSpeaker();
            mMainContentComponent.getSpeakersLock().unlock();
        }
        g.fillAll(mLookAndFeel.getHighlightColour());
    } else {
        if (mMainContentComponent.getSpeakersLock().try_lock()) {
            mMainContentComponent.getSpeakers()[rowNumber]->unSelectSpeaker();
            mMainContentComponent.getSpeakersLock().unlock();
        }
        if (rowNumber % 2) {
            g.fillAll(mLookAndFeel.getBackgroundColour().withBrightness(0.6f));
        } else {
            g.fillAll(mLookAndFeel.getBackgroundColour().withBrightness(0.7f));
        }
    }
}

//==============================================================================
// This is overloaded from TableListBoxModel, and must paint any cells that aren't using custom components.
void EditSpeakersWindow::paintCell(juce::Graphics & g,
                                   int const rowNumber,
                                   int const columnId,
                                   int const width,
                                   int const height,
                                   bool /*rowIsSelected*/)
{
    g.setColour(juce::Colours::black);
    g.setFont(mFont);

    if (mMainContentComponent.getSpeakersLock().try_lock()) {
        if (mMainContentComponent.getSpeakers().size() > rowNumber) {
            auto const text{ getText(columnId, rowNumber) };
            g.drawText(text, 2, 0, width - 4, height, juce::Justification::centredLeft, true);
        }
        mMainContentComponent.getSpeakersLock().unlock();
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

    if (columnId == cols::DIRECT) {
        auto * tbDirect{ dynamic_cast<juce::ToggleButton *>(existingComponentToUpdate) };
        if (tbDirect == nullptr) {
            tbDirect = new juce::ToggleButton();
        }
        tbDirect->setName(juce::String(rowNumber + 1000));
        tbDirect->setClickingTogglesState(true);
        tbDirect->setBounds(4, 404, 88, 22);
        tbDirect->addListener(this);
        tbDirect->setToggleState(mMainContentComponent.getSpeakers()[rowNumber]->isDirectOut(),
                                 juce::dontSendNotification);
        tbDirect->setLookAndFeel(&mLookAndFeel);
        return tbDirect;
    }
    if (columnId == cols::DELETE) {
        auto * tbRemove{ dynamic_cast<juce::TextButton *>(existingComponentToUpdate) };
        if (tbRemove == nullptr) {
            tbRemove = new juce::TextButton();
        }
        tbRemove->setButtonText("X");
        tbRemove->setName(juce::String(rowNumber));
        tbRemove->setBounds(4, 404, 88, 22);
        tbRemove->addListener(this);
        tbRemove->setColour(juce::ToggleButton::textColourId, mLookAndFeel.getFontColour());
        tbRemove->setLookAndFeel(&mLookAndFeel);
        return tbRemove;
    }

    // The other columns are editable text columns, for which we use the custom juce::Label component
    auto * textLabel{ dynamic_cast<EditableTextCustomComponent *>(existingComponentToUpdate) };
    if (textLabel == nullptr) {
        textLabel = new EditableTextCustomComponent(*this);
    }

    textLabel->setRowAndColumn(rowNumber, columnId);

    if (mMainContentComponent.getModeSelected() == SpatModes::lbap
        || mMainContentComponent.getSpeakers()[rowNumber]->isDirectOut()) {
        if (columnId < cols::X) {
            textLabel->setEditable(false);
        }
    } else {
        if (columnId < cols::AZIMUTH) {
            textLabel->setEditable(false);
        }
    }

    return textLabel;
}

//==============================================================================
SpatModes EditSpeakersWindow::getModeSelected() const
{
    return static_cast<SpatModes>(mMainContentComponent.getModeSelected());
}

//==============================================================================
bool EditSpeakersWindow::getDirectOutForSpeakerRow(int const row) const
{
    return mMainContentComponent.getSpeakers()[row]->isDirectOut();
}