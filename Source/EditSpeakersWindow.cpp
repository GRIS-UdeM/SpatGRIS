/*
 This file is part of SpatGRIS.

 Developers: Samuel Béland, Olivier Bélanger, Nicolas Masson

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

#include "EditSpeakersWindow.h"

#include "EditableTextCustomComponent.h"
#include "GrisLookAndFeel.h"
#include "MainComponent.h"
#include "Speaker.h"
#include "narrow.hpp"

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
    mPinkNoiseGainSlider.setRange(MIN_PINK_NOISE_DB.get(), MAX_PINK_NOISE_DB.get(), 1.0);
    mPinkNoiseGainSlider.setValue(DEFAULT_PINK_NOISE_DB.get());
    mPinkNoiseGainSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 20);
    mPinkNoiseGainSlider.setColour(juce::ToggleButton::textColourId, mLookAndFeel.getFontColour());
    mPinkNoiseGainSlider.setLookAndFeel(&mLookAndFeel);
    mPinkNoiseGainSlider.addListener(this);
    mListSpeakerBox.getContent()->addAndMakeVisible(mPinkNoiseGainSlider);

    mListSpeakerBox.getContent()->addAndMakeVisible(mSpeakersTableListBox);

    mListSpeakerBox.repaint();
    mListSpeakerBox.resized();

    // add drag listener
    mListSpeakerBox.addMouseListener(this, true);

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
    static auto constexpr DRAG_COL_WIDTH{ MIN_COL_WIDTH / 5 * 3 };
    header.addColumn("",
                     Cols::DRAG_HANDLE,
                     DRAG_COL_WIDTH,
                     DRAG_COL_WIDTH,
                     DRAG_COL_WIDTH,
                     juce::TableHeaderComponent::ColumnPropertyFlags::notResizableOrSortable);
    header.addColumn("Output",
                     Cols::OUTPUT_PATCH,
                     DEFAULT_COL_WIDTH,
                     MIN_COL_WIDTH,
                     MAX_COL_WIDTH,
                     juce::TableHeaderComponent::defaultFlags);
    header.addColumn("X",
                     Cols::X,
                     DEFAULT_COL_WIDTH,
                     MIN_COL_WIDTH,
                     MAX_COL_WIDTH,
                     juce::TableHeaderComponent::defaultFlags);
    header.addColumn("Y",
                     Cols::Y,
                     DEFAULT_COL_WIDTH,
                     MIN_COL_WIDTH,
                     MAX_COL_WIDTH,
                     juce::TableHeaderComponent::defaultFlags);
    header.addColumn("Z",
                     Cols::Z,
                     DEFAULT_COL_WIDTH,
                     MIN_COL_WIDTH,
                     MAX_COL_WIDTH,
                     juce::TableHeaderComponent::defaultFlags);
    header.addColumn("Azimuth",
                     Cols::AZIMUTH,
                     DEFAULT_COL_WIDTH,
                     MIN_COL_WIDTH,
                     MAX_COL_WIDTH,
                     juce::TableHeaderComponent::defaultFlags);
    header.addColumn("Elevation",
                     Cols::ELEVATION,
                     DEFAULT_COL_WIDTH,
                     MIN_COL_WIDTH,
                     MAX_COL_WIDTH,
                     juce::TableHeaderComponent::defaultFlags);
    header.addColumn("Distance",
                     Cols::DISTANCE,
                     DEFAULT_COL_WIDTH,
                     MIN_COL_WIDTH,
                     MAX_COL_WIDTH,
                     juce::TableHeaderComponent::defaultFlags);
    header.addColumn("Gain (dB)",
                     Cols::GAIN,
                     DEFAULT_COL_WIDTH,
                     MIN_COL_WIDTH,
                     MAX_COL_WIDTH,
                     juce::TableHeaderComponent::notSortable);
    header.addColumn("Highpass",
                     Cols::HIGHPASS,
                     DEFAULT_COL_WIDTH,
                     MIN_COL_WIDTH,
                     MAX_COL_WIDTH,
                     juce::TableHeaderComponent::notSortable);
    header.addColumn("Direct",
                     Cols::DIRECT_TOGGLE,
                     DEFAULT_COL_WIDTH,
                     MIN_COL_WIDTH,
                     MAX_COL_WIDTH,
                     juce::TableHeaderComponent::notSortable);
    header.addColumn("delete",
                     Cols::DELETE_BUTTON,
                     DEFAULT_COL_WIDTH,
                     MIN_COL_WIDTH,
                     MAX_COL_WIDTH,
                     juce::TableHeaderComponent::notSortable);

    mSpeakersTableListBox.setMultipleSelectionEnabled(true);

    mNumRows = mMainContentComponent.getSpeakers().size();

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
    for (auto const & speaker : speakers) {
        auto & toSortItem{ toSort[index++] };
        toSortItem.id = speaker.getSpeakerId().get();
        toSortItem.directOut = speaker.isDirectOut();
        switch (newSortColumnId) {
        case Cols::X:
            toSortItem.value = speaker.getCartesianCoords().z;
            break;
        case Cols::Y:
            toSortItem.value = speaker.getCartesianCoords().x;
            break;
        case Cols::Z:
            toSortItem.value = speaker.getCartesianCoords().y;
            break;
        case Cols::AZIMUTH:
            toSortItem.value = speaker.getPolarCoords().x;
            break;
        case Cols::ELEVATION:
            toSortItem.value = speaker.getPolarCoords().y;
            break;
        case Cols::DISTANCE:
            toSortItem.value = speaker.getPolarCoords().z;
            break;
        case Cols::OUTPUT_PATCH:
            toSortItem.value = static_cast<float>(speaker.getOutputPatch().get());
            break;
        default:
            jassertfalse;
        }
    }

    auto const size{ mMainContentComponent.getSpeakers().size() };
    if (isForwards) {
        std::sort(toSort, toSort + size, compareLessThan);
    } else {
        std::sort(toSort, toSort + size, compareGreaterThan);
    }

    juce::Array<speaker_id_t> newOrder{};
    newOrder.resize(size);
    for (int i{}; i < size; ++i) {
        newOrder[i] = speaker_id_t{ toSort[i].id };
    }
    mMainContentComponent.reorderSpeakers(std::move(newOrder));
    updateWinContent(false);
}

//==============================================================================
void EditSpeakersWindow::sliderValueChanged(juce::Slider * slider)
{
    if (slider == &mPinkNoiseGainSlider) {
        dbfs_t const db{ narrow<float>(mPinkNoiseGainSlider.getValue()) };
        mMainContentComponent.getAudioProcessor().setPinkNoiseGain(db);
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
            mMainContentComponent.addSpeaker();
            updateWinContent(true);
            mSpeakersTableListBox.selectRow(getNumRows() - 1);
        } else {
            mMainContentComponent.insertSpeaker(selectedRow);
            updateWinContent(true);
            mSpeakersTableListBox.selectRow(selectedRow + 1);
        }
        mSpeakersTableListBox.getHeader().setSortColumnId(sortColumnId, sortedForwards);
        mMainContentComponent.setNeedToComputeVbap(true);
    } else if (button == &mCompSpeakersButton) {
        if (mMainContentComponent.refreshSpeakers()) {
            mMainContentComponent.setShowTriplets(tripletState);
        }
    } else if (button == &mAddRingButton) {
        for (int i{}; i < mNumOfSpeakersTextEditor.getText().getIntValue(); i++) {
            if (selectedRow == -1 || selectedRow == (mNumRows - 1)) {
                mMainContentComponent.addSpeaker();
                mNumRows = mMainContentComponent.getSpeakers().size();
                selectedRow = mNumRows - 1;
            } else {
                mMainContentComponent.insertSpeaker(selectedRow);
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
        updateWinContent(true);
        mSpeakersTableListBox.selectRow(selectedRow);
        // TableList needs different sorting parameters to trigger the sorting function.
        mSpeakersTableListBox.getHeader().setSortColumnId(sortColumnId, !sortedForwards);
        // This is the real sorting!
        mSpeakersTableListBox.getHeader().setSortColumnId(sortColumnId, sortedForwards);
        mMainContentComponent.setNeedToComputeVbap(true);
    } else if (button == &mPinkNoiseToggleButton) {
        mMainContentComponent.getAudioProcessor().setPinkNoiseActive(mPinkNoiseToggleButton.getToggleState());
    } else if (button->getName().isNotEmpty()
               && (button->getName().getIntValue() >= 0
                   && button->getName().getIntValue() <= mMainContentComponent.getSpeakers().size())) {
        // Delete button
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
        updateWinContent(true);
        mSpeakersTableListBox.deselectAllRows();
        mMainContentComponent.setNeedToComputeVbap(true);
    } else {
        // Direct out
        auto const row{ button->getName().getIntValue() - 1000 };
        mMainContentComponent.getSpeakers()[row]->setDirectOut(button->getToggleState());
        if (mSpeakersTableListBox.getNumSelectedRows() > 1) {
            for (int i{}; i < mSpeakersTableListBox.getSelectedRows().size(); ++i) {
                auto const rowNumber{ mSpeakersTableListBox.getSelectedRows()[i] };
                mMainContentComponent.getSpeakers()[rowNumber]->setDirectOut(button->getToggleState());
                auto * tog{ dynamic_cast<juce::ToggleButton *>(
                    mSpeakersTableListBox.getCellComponent(Cols::DIRECT_TOGGLE, rowNumber)) };
                if (tog) {
                    tog->setToggleState(button->getToggleState(), juce::NotificationType::dontSendNotification);
                } else {
                    jassertfalse;
                }
            }
        }
        updateWinContent(true);
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
void EditSpeakersWindow::updateWinContent(bool const needToSaveSpeakerSetup)
{
    mNumRows = mMainContentComponent.getSpeakers().size();
    mSpeakersTableListBox.updateContent();
    if (needToSaveSpeakerSetup) {
        mMainContentComponent.setNeedToSaveSpeakerSetup(true);
    }
}

//==============================================================================
void EditSpeakersWindow::selectRow(int const value)
{
    juce::MessageManagerLock const mmLock{};
    mSpeakersTableListBox.selectRow(value);
    repaint();
}

//==============================================================================
void EditSpeakersWindow::selectSpeaker(output_patch_t const outputPatch)
{
    juce::ScopedLock const sl{ mMainContentComponent.getSpeakersLock() };
    int row{};
    for (auto const * speaker : mMainContentComponent.getSpeakers()) {
        if (speaker->getOutputPatch() == outputPatch) {
            selectRow(row);
            return;
        }
        ++row;
    }
    selectRow(-1);
}

//==============================================================================
void EditSpeakersWindow::closeButtonPressed()
{
    enum Button { save, no, cancel };
    int exitV{ no };
    if (mMainContentComponent.needToSaveSpeakerSetup()) {
        juce::AlertWindow alert("Closing Speaker Setup Window !",
                                "Do you want to compute and save the current setup ?",
                                juce::AlertWindow::WarningIcon);
        alert.setLookAndFeel(&mLookAndFeel);
        alert.addButton("Save", Button::save, juce::KeyPress(juce::KeyPress::returnKey));
        alert.addButton("No", Button::no);
        alert.addButton("Cancel", Button::cancel, juce::KeyPress(juce::KeyPress::escapeKey));
        exitV = alert.runModalLoop();

        if (exitV == Button::save) {
            alert.setVisible(false);
            auto const valid{ mMainContentComponent.refreshSpeakers() };
            if (!valid) {
                return;
            }
            mMainContentComponent.handleTimer(false);
            setAlwaysOnTop(false);
            mMainContentComponent.handleSaveAsSpeakerSetup();
            mMainContentComponent.handleTimer(true);
        } else if (exitV == Button::no) {
            alert.setVisible(false);
            mMainContentComponent.reloadXmlFileSpeaker();
            mMainContentComponent.refreshSpeakers();
        }
    }
    if (exitV != cancel) {
        mMainContentComponent.getAudioProcessor().setPinkNoiseActive(false);
        mMainContentComponent.closeSpeakersConfigurationWindow();
    }
}

//==============================================================================
void EditSpeakersWindow::resized()
{
    DocumentWindow::resized();

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
        case Cols::X:
            text = juce::String{ speaker.getCartesianCoords().z, 2 };
            break;
        case Cols::Y:
            text = juce::String{ speaker.getCartesianCoords().x, 2 };
            break;
        case Cols::Z:
            text = juce::String{ speaker.getCartesianCoords().y, 2 };
            break;
        case Cols::AZIMUTH:
            text = juce::String{ speaker.getPolarCoords().x, 2 };
            break;
        case Cols::ELEVATION:
            text = juce::String{ speaker.getPolarCoords().y, 2 };
            break;
        case Cols::DISTANCE:
            text = juce::String{ speaker.getPolarCoords().z, 2 };
            break;
        case Cols::OUTPUT_PATCH:
            text = juce::String{ speaker.getOutputPatch().get() };
            break;
        case Cols::GAIN:
            text = juce::String{ speaker.getGain(), 2 };
            break;
        case Cols::HIGHPASS:
            text = juce::String{ speaker.getHighPassCutoff(), 2 };
            break;
        case Cols::DIRECT_TOGGLE:
            text = juce::String{ static_cast<int>(speaker.isDirectOut()) };
            break;
        case Cols::DRAG_HANDLE:
            text = "=";
            break;
        default:
            jassertfalse;
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
    juce::ScopedTryLock sl{ mMainContentComponent.getSpeakersLock() };
    if (sl.isLocked()) {
        if (mMainContentComponent.getSpeakers().size() > rowNumber) {
            auto const selectedRows{ mSpeakersTableListBox.getSelectedRows() };
            auto & speaker{ *mMainContentComponent.getSpeakers()[rowNumber] };

            switch (columnNumber) {
            case Cols::X: {
                auto newP = speaker.getCartesianCoords();
                auto const val{ std::clamp(newText.getFloatValue(), -1.0f, 1.0f) };
                auto diff = val - newP.z;
                newP.z = val;
                speaker.setCoordinate(newP);
                if (selectedRows.size() > 1) {
                    for (int i{}; i < selectedRows.size(); ++i) {
                        auto const rowNum = selectedRows[i];
                        if (rowNum == rowNumber) {
                            continue;
                        }
                        newP = mMainContentComponent.getSpeakers()[rowNum]->getCartesianCoords();
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
            case Cols::Y: {
                auto newP = speaker.getCartesianCoords();
                auto const val{ std::clamp(newText.getFloatValue(), -1.0f, 1.0f) };
                auto diff = val - newP.x;
                newP.x = val;
                speaker.setCoordinate(newP);
                if (selectedRows.size() > 1) {
                    for (int i{}; i < selectedRows.size(); ++i) {
                        auto const rowNum{ selectedRows[i] };
                        if (rowNum == rowNumber) {
                            continue;
                        }
                        newP = mMainContentComponent.getSpeakers()[rowNum]->getCartesianCoords();
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
            case Cols::Z: {
                auto newP = speaker.getCartesianCoords();
                auto const val{ std::clamp(newText.getFloatValue(), 0.0f, 1.0f) };
                auto diff = val - newP.y;
                newP.y = val;
                speaker.setCoordinate(newP);
                if (selectedRows.size() > 1) {
                    for (int i{}; i < selectedRows.size(); ++i) {
                        auto const rowNum{ selectedRows[i] };
                        if (rowNum == rowNumber) {
                            continue;
                        }
                        newP = mMainContentComponent.getSpeakers()[rowNum]->getCartesianCoords();
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
            case Cols::AZIMUTH: {
                auto newP = speaker.getPolarCoords();
                auto val{ getFloatPrecision(newText.getFloatValue(), 3.0f) };
                auto diff = val - newP.x;
                while (val > 360.0f) {
                    val -= 360.0f;
                }
                while (val < 0.0f) {
                    val += 360.0f;
                }
                newP.x = val;
                speaker.setAziZenRad(newP);
                if (selectedRows.size() > 1) {
                    for (int i{}; i < selectedRows.size(); ++i) {
                        auto const rowNum{ selectedRows[i] };
                        if (rowNum == rowNumber) {
                            continue;
                        }
                        newP = mMainContentComponent.getSpeakers()[rowNum]->getPolarCoords();
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
            case Cols::ELEVATION: {
                auto newP = speaker.getPolarCoords();
                auto const val{ std::clamp(newText.getFloatValue(), -90.0f, 90.0f) };
                auto diff = val - newP.y;
                newP.y = val;
                speaker.setAziZenRad(newP);
                if (selectedRows.size() > 1) {
                    for (int i{}; i < selectedRows.size(); ++i) {
                        auto const rowNum{ selectedRows[i] };
                        if (rowNum == rowNumber) {
                            continue;
                        }
                        newP = mMainContentComponent.getSpeakers()[rowNum]->getPolarCoords();
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
            case Cols::DISTANCE: {
                auto newP = speaker.getPolarCoords();
                float val{};
                if (mMainContentComponent.isRadiusNormalized() && !speaker.isDirectOut()) {
                    val = 1.0f;
                } else {
                    val = getFloatPrecision(newText.getFloatValue(), 3.0f);
                }
                auto diff = val - newP.z;
                val = std::clamp(val, 0.0f, juce::MathConstants<float>::sqrt2);
                newP.z = val;
                speaker.setAziZenRad(newP);
                if (selectedRows.size() > 1) {
                    for (int i{}; i < selectedRows.size(); ++i) {
                        auto const rowNum{ selectedRows[i] };
                        if (rowNum == rowNumber) {
                            continue;
                        }
                        newP = mMainContentComponent.getSpeakers()[rowNum]->getPolarCoords();
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
            case Cols::OUTPUT_PATCH: {
                mMainContentComponent.setShowTriplets(false);
                auto const oldValue{ speaker.getOutputPatch() };
                auto iValue{ output_patch_t{ std::clamp(newText.getIntValue(), 0, 256) } };
                if (!speaker.isDirectOut()) {
                    for (auto const * it : mMainContentComponent.getSpeakers()) {
                        if (it == mMainContentComponent.getSpeakers()[rowNumber] || it->isDirectOut()) {
                            continue;
                        }
                        if (it->getOutputPatch() == iValue) {
                            juce::AlertWindow alert("Wrong output patch!    ",
                                                    "Sorry! Output patch number " + juce::String(iValue.get())
                                                        + " is already used.",
                                                    juce::AlertWindow::WarningIcon);
                            alert.setLookAndFeel(&mLookAndFeel);
                            alert.addButton("OK", 0, juce::KeyPress(juce::KeyPress::returnKey));
                            alert.runModalLoop();
                            iValue = oldValue;
                        }
                    }
                }
                speaker.setOutputPatch(iValue);
                break;
            }
            case Cols::GAIN: {
                auto val{ newText.getFloatValue() };
                auto diff = val - speaker.getGain();
                val = std::clamp(val, -18.0f, 6.0f);
                speaker.setGain(val);
                if (selectedRows.size() > 1) {
                    for (int i{}; i < selectedRows.size(); ++i) {
                        auto const rowNum{ selectedRows[i] };
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
            case Cols::HIGHPASS: {
                auto val{ newText.getFloatValue() };
                auto diff = val - speaker.getHighPassCutoff();
                val = std::clamp(val, 0.0f, 150.0f);
                speaker.setHighPassCutoff(val);
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
            case Cols::DIRECT_TOGGLE:
                mMainContentComponent.setShowTriplets(false);
                speaker.setDirectOut(newText.getIntValue());
                break;
            default:
                break;
            }
        }
        updateWinContent(true); // necessary?
        mMainContentComponent.setNeedToComputeVbap(true);
    }
}

//==============================================================================
bool EditSpeakersWindow::isMouseOverDragHandle(juce::MouseEvent const & event)
{
    auto const positionRelativeToSpeakersTableListBox{ event.getEventRelativeTo(&mSpeakersTableListBox).getPosition() };
    auto const speakersTableListBoxBounds{ mSpeakersTableListBox.getBounds() };

    auto const draggableWidth{ mSpeakersTableListBox.getHeader().getColumnWidth(Cols::DRAG_HANDLE) };
    return speakersTableListBoxBounds.contains(positionRelativeToSpeakersTableListBox)
           && positionRelativeToSpeakersTableListBox.getX() < draggableWidth;
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
        juce::ScopedTryLock const sl{ mMainContentComponent.getSpeakersLock() };
        if (sl.isLocked()) {
            mMainContentComponent.getSpeakers()[rowNumber]->selectSpeaker();
        }
        g.fillAll(mLookAndFeel.getHighlightColour());
    } else {
        juce::ScopedTryLock const sl{ mMainContentComponent.getSpeakersLock() };
        if (sl.isLocked()) {
            mMainContentComponent.getSpeakers()[rowNumber]->unSelectSpeaker();
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
void EditSpeakersWindow::paintCell(juce::Graphics & /*g*/,
                                   int const /*rowNumber*/,
                                   int const /*columnId*/,
                                   int const /*width */,
                                   int const /*height*/,
                                   bool /*rowIsSelected*/)
{
}

//==============================================================================
juce::Component * EditSpeakersWindow::refreshComponentForCell(int const rowNumber,
                                                              int const columnId,
                                                              bool const isRowSelected,
                                                              Component * existingComponentToUpdate)
{
    juce::ignoreUnused(isRowSelected);

    if (columnId == Cols::DIRECT_TOGGLE) {
        auto * toggleButton{ dynamic_cast<juce::ToggleButton *>(existingComponentToUpdate) };
        if (toggleButton == nullptr) {
            toggleButton = new juce::ToggleButton();
        }
        toggleButton->setName(juce::String(rowNumber + 1000));
        toggleButton->setClickingTogglesState(true);
        toggleButton->setBounds(4, 404, 88, 22);
        toggleButton->addListener(this);
        toggleButton->setToggleState(mMainContentComponent.getSpeakers()[rowNumber]->isDirectOut(),
                                     juce::dontSendNotification);
        toggleButton->setLookAndFeel(&mLookAndFeel);
        return toggleButton;
    }
    if (columnId == Cols::DELETE_BUTTON) {
        auto * textButton{ dynamic_cast<juce::TextButton *>(existingComponentToUpdate) };
        if (textButton == nullptr) {
            textButton = new juce::TextButton();
        }
        textButton->setButtonText("X");
        textButton->setName(juce::String(rowNumber));
        textButton->setBounds(4, 404, 88, 22);
        textButton->addListener(this);
        textButton->setColour(juce::ToggleButton::textColourId, mLookAndFeel.getFontColour());
        textButton->setLookAndFeel(&mLookAndFeel);
        return textButton;
    }

    enum class EditionType { notEditable, editable, valueDraggable, reorderDraggable };

    auto const getEditionType = [this, rowNumber, columnId]() -> EditionType {
        switch (columnId) {
        case Cols::X:
        case Cols::Y:
        case Cols::Z:
            if (mMainContentComponent.getModeSelected() == SpatMode::lbap
                || mMainContentComponent.getSpeakers()[rowNumber]->isDirectOut()) {
                return EditionType::valueDraggable;
            }
            return EditionType::notEditable;
        case Cols::AZIMUTH:
        case Cols::DISTANCE:
        case Cols::ELEVATION:
        case Cols::GAIN:
        case Cols::HIGHPASS:
            return EditionType::valueDraggable;
        case Cols::OUTPUT_PATCH:
            return EditionType::editable;
        case Cols::DELETE_BUTTON:
        case Cols::DIRECT_TOGGLE:
        case Cols::DRAG_HANDLE:
            return EditionType::reorderDraggable;
        }
        jassertfalse;
        return {};
    };

    static auto const getMouseCursor = [](EditionType const editionType) {
        switch (editionType) {
        case EditionType::valueDraggable:
            return juce::MouseCursor::StandardCursorType::UpDownResizeCursor;
        case EditionType::reorderDraggable:
            return juce::MouseCursor::StandardCursorType::DraggingHandCursor;
        default:
            return juce::MouseCursor::StandardCursorType::NormalCursor;
        }
    };

    // The other columns are editable text columns, for which we use the custom juce::Label component
    auto * textLabel{ dynamic_cast<EditableTextCustomComponent *>(existingComponentToUpdate) };
    if (textLabel == nullptr) {
        textLabel = new EditableTextCustomComponent{ *this };
        if (columnId == Cols::DRAG_HANDLE) {
            textLabel->setJustificationType(juce::Justification::centred);
            textLabel->setEditable(false);
            textLabel->setMouseCursor(juce::MouseCursor::StandardCursorType::DraggingHandCursor);
        }
    }

    auto const editionType{ getEditionType() };
    textLabel->setRowAndColumn(rowNumber, columnId);
    textLabel->setMouseCursor(getMouseCursor(editionType));
    textLabel->setEditable(editionType == EditionType::editable || editionType == EditionType::valueDraggable);

    return textLabel;
}

//==============================================================================
void EditSpeakersWindow::mouseDown(juce::MouseEvent const & event)
{
    if (isMouseOverDragHandle(event)) {
        mDragStartY = event.getEventRelativeTo(&mSpeakersTableListBox).getPosition().getY();
    } else {
        mDragStartY = std::nullopt;
    }
}

//==============================================================================
void EditSpeakersWindow::mouseDrag(juce::MouseEvent const & event)
{
    if (!mDragStartY) {
        return;
    }

    auto const eventRel{ event.getEventRelativeTo(&mSpeakersTableListBox).getPosition() };

    auto const rowHeight{ mSpeakersTableListBox.getRowHeight() };
    auto const yDiff{ eventRel.getY() - *mDragStartY };
    auto const rowDiff{ narrow<int>(std::round(narrow<float>(yDiff) / narrow<float>(rowHeight))) };
    auto const numRows{ mSpeakersTableListBox.getNumRows() };

    if (rowDiff == 0) {
        // no movement
        return;
    }

    auto const getCurrentSpeakerOrder = [this]() -> std::vector<speaker_id_t> {
        auto const & speakers{ mMainContentComponent.getSpeakers() };
        std::vector<speaker_id_t> result{};
        result.resize(narrow<size_t>(speakers.size()));
        std::transform(std::begin(speakers),
                       std::end(speakers),
                       std::begin(result),
                       [](Speaker const * speaker) -> speaker_id_t { return speaker->getId(); });
        return result;
    };

    // TODO : make this work for multiple selections
    auto const selectedRow{ mSpeakersTableListBox.getSelectedRow() };
    if (selectedRow < 0 || selectedRow >= mSpeakersTableListBox.getNumRows()) {
        jassertfalse;
        return;
    }

    auto const newIndex{ std::clamp(selectedRow + rowDiff, 0, numRows - 1) };
    if (newIndex == selectedRow) {
        return;
    }

    auto order{ getCurrentSpeakerOrder() };
    std::swap(order[selectedRow], order[newIndex]);

    mMainContentComponent.reorderSpeakers(order);
    if (mSpeakersTableListBox.contains(eventRel)) {
        *mDragStartY += rowDiff * rowHeight;
    }
    mSpeakersTableListBox.selectRow(newIndex);

    mMainContentComponent.setNeedToSaveSpeakerSetup(true);
    updateWinContent(true);
}

//==============================================================================
SpatMode EditSpeakersWindow::getModeSelected() const
{
    return static_cast<SpatMode>(mMainContentComponent.getModeSelected());
}

//==============================================================================
bool EditSpeakersWindow::getDirectOutForSpeakerRow(int const row) const
{
    return mMainContentComponent.getSpeakers()[row]->isDirectOut();
}
