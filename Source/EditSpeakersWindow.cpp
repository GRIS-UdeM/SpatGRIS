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

#include "EditSpeakersWindow.hpp"

#include "EditableTextCustomComponent.hpp"
#include "GrisLookAndFeel.hpp"
#include "MainComponent.hpp"
#include "Narrow.hpp"

//==============================================================================
static Position getLegalPosition(Position const & position,
                                 SpatMode const spatMode,
                                 bool const isDirectOutOnly,
                                 [[maybe_unused]] int const modifiedCol)
{
    using Col = EditSpeakersWindow::Cols;

    jassert(modifiedCol == Col::AZIMUTH || modifiedCol == Col::ELEVATION || modifiedCol == Col::DISTANCE
            || modifiedCol == Col::X || modifiedCol == Col::Y || modifiedCol == Col::Z);

    static auto const constrainCartesian
        = [](float const & valueModified, float & valueToAdjust, float & valueToTryToKeepIntact) {
              auto const valueModified2{ valueModified * valueModified };
              auto const lengthWithoutValueToAdjust{ valueModified2 + valueToTryToKeepIntact * valueToTryToKeepIntact };

              if (lengthWithoutValueToAdjust > 1.0f) {
                  auto const sign{ valueToTryToKeepIntact < 0.0f ? -1.0f : 1.0f };
                  auto const length{ std::sqrt(1.0f - valueModified2) };
                  valueToTryToKeepIntact = sign * length;
                  valueToAdjust = 0.0f;
                  return;
              }

              auto const sign{ valueToAdjust < 0.0f ? -1.0f : 1.0f };
              auto const length{ std::sqrt(1.0f - lengthWithoutValueToAdjust) };
              valueToAdjust = sign * length;
          };

    if (spatMode == SpatMode::lbap || isDirectOutOnly) {
        return Position{ position.getCartesian().clampedToFarField() };
    }

    if (modifiedCol == Col::AZIMUTH || modifiedCol == Col::ELEVATION) {
        return position.normalized();
    }

    auto newPosition{ position.getCartesian() };

    auto & x{ newPosition.x };
    auto & y{ newPosition.y };
    auto & z{ newPosition.z };
    if (modifiedCol == Col::X) {
        x = std::clamp(x, -1.0f, 1.0f);
        constrainCartesian(x, y, z);
    } else if (modifiedCol == Col::Y) {
        y = std::clamp(y, -1.0f, 1.0f);
        constrainCartesian(y, x, z);
    } else {
        jassert(modifiedCol == Col::Z);
        z = std::clamp(z, 0.0f, 1.0f);
        constrainCartesian(z, x, y);
    }
    return Position{ newPosition };
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
    JUCE_ASSERT_MESSAGE_THREAD;

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
    mListSpeakerBox.getContent()->addAndMakeVisible(mNumOfSpeakersTextEditor);

    mNumOfSpeakersTextEditor.setText("8", false);
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
    mPinkNoiseGainSlider.setRange(LEGAL_PINK_NOISE_GAIN_RANGE.getStart().get(),
                                  LEGAL_PINK_NOISE_GAIN_RANGE.getEnd().get(),
                                  1.0);
    mPinkNoiseGainSlider.setValue(DEFAULT_PINK_NOISE_DB.get());
    mPinkNoiseGainSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 20);
    mPinkNoiseGainSlider.setColour(juce::ToggleButton::textColourId, mLookAndFeel.getFontColour());
    mPinkNoiseGainSlider.setLookAndFeel(&mLookAndFeel);
    mPinkNoiseGainSlider.addListener(this);
    mListSpeakerBox.getContent()->addAndMakeVisible(mPinkNoiseGainSlider);

    mListSpeakerBox.getContent()->addAndMakeVisible(mSpeakersTableListBox);

    mListSpeakerBox.repaint();
    mListSpeakerBox.resized();

    mListSpeakerBox.addMouseListener(this, true);

    setResizable(true, true);
    setUsingNativeTitleBar(true);
    setAlwaysOnTop(true);

    setContentNonOwned(&mListSpeakerBox, false);
    auto const & controlsComponent{ *mainContentComponent.getControlsComponent() };

    static constexpr auto WIDTH = 850;
    static constexpr auto HEIGHT = 600;
    static constexpr auto TITLE_BAR_HEIGHT = 30;

    setBounds(controlsComponent.getScreenX(), controlsComponent.getScreenY() + TITLE_BAR_HEIGHT, WIDTH, HEIGHT);

    DocumentWindow::setVisible(true);
}

//==============================================================================
void EditSpeakersWindow::initComp()
{
    JUCE_ASSERT_MESSAGE_THREAD;

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

    mNumRows = mMainContentComponent.getData().speakerSetup.speakers.size();

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
    JUCE_ASSERT_MESSAGE_THREAD;

    static auto const EXTRACT_VALUE = [](SpeakersData::ConstNode const & speaker, int const sortColumn) -> float {
        auto const & position{ speaker.value->position };
        switch (sortColumn) {
        case Cols::X:
            return position.getCartesian().x;
        case Cols::Y:
            return position.getCartesian().y;
        case Cols::Z:
            return position.getCartesian().z;
        case Cols::AZIMUTH:
            return position.getPolar().azimuth.get();
        case Cols::ELEVATION:
            return position.getPolar().elevation.get();
        case Cols::DISTANCE:
            return position.getPolar().length;
        case Cols::OUTPUT_PATCH:
            return static_cast<float>(speaker.key.get());
        default:
            jassertfalse;
            return 0.0f;
        }
    };

    if (newSortColumnId == 0) {
        return;
    }

    auto const & speakers{ mMainContentComponent.getData().speakerSetup.speakers };
    std::vector<std::pair<float, output_patch_t>> valuesToSort{};
    valuesToSort.reserve(narrow<size_t>(speakers.size()));
    std::transform(speakers.cbegin(),
                   speakers.cend(),
                   std::back_inserter(valuesToSort),
                   [newSortColumnId](SpeakersData::ConstNode const speaker) {
                       return std::make_pair(EXTRACT_VALUE(speaker, newSortColumnId), speaker.key);
                   });
    if (isForwards) {
        std::sort(valuesToSort.begin(), valuesToSort.end(), std::less());
    } else {
        std::sort(valuesToSort.begin(), valuesToSort.end(), std::greater_equal());
    }
    juce::Array<output_patch_t> newOrder{};
    newOrder.resize(narrow<int>(valuesToSort.size()));
    std::transform(valuesToSort.cbegin(),
                   valuesToSort.cend(),
                   newOrder.begin(),
                   [](std::pair<float, output_patch_t> const & entry) { return entry.second; });

    mMainContentComponent.reorderSpeakers(std::move(newOrder));
    updateWinContent();
}

//==============================================================================
void EditSpeakersWindow::sliderValueChanged(juce::Slider * slider)
{
    JUCE_ASSERT_MESSAGE_THREAD;
    jassert(slider == &mPinkNoiseGainSlider);

    if (slider == &mPinkNoiseGainSlider && mPinkNoiseToggleButton.getToggleState()) {
        dbfs_t const db{ narrow<float>(mPinkNoiseGainSlider.getValue()) };
        mMainContentComponent.setPinkNoiseGain(db);
    }
}

//==============================================================================
void EditSpeakersWindow::buttonClicked(juce::Button * button)
{
    JUCE_ASSERT_MESSAGE_THREAD;

    static auto const GET_SELECTED_ROW = [](juce::TableListBox const & tableListBox) -> tl::optional<int> {
        auto const selectedRow{ tableListBox.getSelectedRow() };
        if (selectedRow < 0) {
            return tl::nullopt;
        }
        return selectedRow;
    };

    auto const sortColumnId{ mSpeakersTableListBox.getHeader().getSortColumnId() };
    auto const sortedForwards{ mSpeakersTableListBox.getHeader().isSortedForwards() };
    auto const & speakers{ mMainContentComponent.getData().speakerSetup.speakers };
    auto selectedRow{ GET_SELECTED_ROW(mSpeakersTableListBox) };

    mMainContentComponent.setShowTriplets(false);

    if (button == &mAddSpeakerButton) {
        // Add speaker button
        tl::optional<output_patch_t> outputPatch{};
        tl::optional<int> index{};

        if (selectedRow) {
            outputPatch = getSpeakerOutputPatchForRow(*selectedRow);
            index = *selectedRow + 1;
        }

        auto const newOutputPatch{ mMainContentComponent.addSpeaker(outputPatch, index) };
        mMainContentComponent.refreshSpeakers();
        updateWinContent();
        selectSpeaker(newOutputPatch);
        mSpeakersTableListBox.getHeader().setSortColumnId(sortColumnId, sortedForwards); // TODO: necessary?
        mShouldRefreshSpeakers = true;
    } else if (button == &mCompSpeakersButton) {
        // Compute speaker button
        auto const success{ mMainContentComponent.refreshSpeakers() };
        mShouldRefreshSpeakers = !success;
    } else if (button == &mAddRingButton) {
        // Add ring button
        output_patch_t newOutputPatch;
        for (int i{}; i < mNumOfSpeakersTextEditor.getText().getIntValue(); i++) {
            tl::optional<output_patch_t> outputPatch{};
            tl::optional<int> index{};

            if (selectedRow) {
                outputPatch = getSpeakerOutputPatchForRow(*selectedRow);
                index = *selectedRow;
                *selectedRow += 1;
            }

            newOutputPatch = mMainContentComponent.addSpeaker(outputPatch, index);
            mNumRows = speakers.size();

            degrees_t azimuth{ -360.0f / narrow<float>(mNumOfSpeakersTextEditor.getText().getIntValue())
                                   * narrow<float>(i)
                               - mOffsetAngleTextEditor.getText().getFloatValue() + 90.0f };
            azimuth = azimuth.centered();
            degrees_t const zenith{ std::clamp(mZenithTextEditor.getText().getFloatValue(), 0.0f, 90.0f) };
            auto const radius{ mRadiusTextEditor.getText().getFloatValue() };

            mMainContentComponent.setSpeakerPosition(newOutputPatch, PolarVector{ azimuth, zenith, radius });
        }
        mMainContentComponent.refreshSpeakers();
        updateWinContent();
        // TableList needs different sorting parameters to trigger the sorting function.
        mSpeakersTableListBox.getHeader().setSortColumnId(sortColumnId, !sortedForwards);
        // This is the real sorting!
        mSpeakersTableListBox.getHeader().setSortColumnId(sortColumnId, sortedForwards);
        selectSpeaker(newOutputPatch);
        mShouldRefreshSpeakers = true;
    } else if (button == &mPinkNoiseToggleButton) {
        // Pink noise button
        tl::optional<dbfs_t> newPinkNoiseLevel{};
        if (mPinkNoiseToggleButton.getToggleState()) {
            newPinkNoiseLevel = dbfs_t{ static_cast<float>(mPinkNoiseGainSlider.getValue()) };
        }
        mMainContentComponent.setPinkNoiseGain(newPinkNoiseLevel);
    } else if (button->getName().getIntValue() < DIRECT_OUT_BUTTON_ID_OFFSET) {
        // Delete button
        auto const row{ button->getName().getIntValue() };
        auto const speakerId{ mMainContentComponent.getData().speakerSetup.ordering[row] };
        mMainContentComponent.removeSpeaker(speakerId);
        updateWinContent();
        mSpeakersTableListBox.deselectAllRows();
        mShouldRefreshSpeakers = true;
    } else {
        // Direct out
        jassert(button->getName().getIntValue() >= DIRECT_OUT_BUTTON_ID_OFFSET);
        auto const row{ button->getName().getIntValue() - DIRECT_OUT_BUTTON_ID_OFFSET };
        auto const outputPatch{ getSpeakerOutputPatchForRow(row) };
        mMainContentComponent.speakerOnlyDirectOutChanged(outputPatch, button->getToggleState());
        updateWinContent();
        mShouldRefreshSpeakers = true;
    }
}

//==============================================================================
void EditSpeakersWindow::textEditorTextChanged(juce::TextEditor & /*editor*/)
{
    JUCE_ASSERT_MESSAGE_THREAD;
}

//==============================================================================
void EditSpeakersWindow::textEditorReturnKeyPressed(juce::TextEditor & textEditor)
{
    JUCE_ASSERT_MESSAGE_THREAD;

    unfocusAllComponents();
    if (&textEditor == &mNumOfSpeakersTextEditor) {
        auto const value{ std::clamp(mNumOfSpeakersTextEditor.getText().getIntValue(), 2, 64) };
        mNumOfSpeakersTextEditor.setText(juce::String{ value }, false);
        return;
    }
    if (&textEditor == &mZenithTextEditor) {
        auto const value{ std::clamp(mZenithTextEditor.getText().getFloatValue(), 0.0f, 90.0f) };
        mZenithTextEditor.setText(juce::String{ value, 1 }, false);
        return;
    }
    if (&textEditor == &mRadiusTextEditor) {
        juce::ScopedReadLock const lock{ mMainContentComponent.getLock() };
        auto const spatMode{ mMainContentComponent.getData().speakerSetup.spatMode };
        auto const minRadius{ spatMode == SpatMode::vbap ? 1.0f : 0.001f };
        auto const maxRadius{ spatMode == SpatMode::vbap ? 1.0f : SQRT3 };
        auto const value{ std::clamp(mRadiusTextEditor.getText().getFloatValue(), minRadius, maxRadius) };
        mRadiusTextEditor.setText(juce::String{ value, 1 }, false);
        return;
    }
    if (&textEditor == &mOffsetAngleTextEditor) {
        auto const value{ std::clamp(mOffsetAngleTextEditor.getText().getFloatValue(), -360.0f, 360.0f) };
        mOffsetAngleTextEditor.setText(juce::String{ value, 1 }, false);
        return;
    }
    jassertfalse;
}

//==============================================================================
void EditSpeakersWindow::textEditorFocusLost(juce::TextEditor & textEditor)
{
    JUCE_ASSERT_MESSAGE_THREAD;

    textEditorReturnKeyPressed(textEditor);
}

//==============================================================================
void EditSpeakersWindow::updateWinContent()
{
    JUCE_ASSERT_MESSAGE_THREAD;

    mNumRows = mMainContentComponent.getData().speakerSetup.speakers.size();
    mSpeakersTableListBox.updateContent();
}

//==============================================================================
void EditSpeakersWindow::pushSelectionToMainComponent() const
{
    JUCE_ASSERT_MESSAGE_THREAD;

    auto const selectedRows{ mSpeakersTableListBox.getSelectedRows() };
    if (selectedRows == mLastSelectedRows) {
        return;
    }

    juce::Array<output_patch_t> selection{};
    selection.ensureStorageAllocated(selectedRows.size());
    for (int i{}; i < selectedRows.size(); ++i) {
        auto const row{ selectedRows[i] };
        auto const outputPatch{ getSpeakerOutputPatchForRow(row) };
        selection.add(outputPatch);
    }

    mMainContentComponent.setSelectedSpeakers(std::move(selection));
}

//==============================================================================
void EditSpeakersWindow::selectRow(tl::optional<int> const value)
{
    JUCE_ASSERT_MESSAGE_THREAD;

    juce::MessageManagerLock const mmLock{};
    mSpeakersTableListBox.selectRow(value.value_or(-1));
    repaint();
}

//==============================================================================
void EditSpeakersWindow::selectSpeaker(tl::optional<output_patch_t> const outputPatch)
{
    auto const getSelectedRow = [this](output_patch_t const id) {
        juce::ScopedReadLock const lock{ mMainContentComponent.getLock() };
        auto const & displayOrder{ mMainContentComponent.getData().speakerSetup.ordering };
        jassert(displayOrder.contains(id));
        return displayOrder.indexOf(id);
    };

    selectRow(outputPatch.map(getSelectedRow));
    pushSelectionToMainComponent();
}

//==============================================================================
void EditSpeakersWindow::closeButtonPressed()
{
    JUCE_ASSERT_MESSAGE_THREAD;

    if (mShouldRefreshSpeakers) {
        mMainContentComponent.refreshSpeakers();
    }
    mMainContentComponent.setPinkNoiseGain(tl::nullopt);
    mMainContentComponent.closeSpeakersConfigurationWindow();
}

//==============================================================================
void EditSpeakersWindow::resized()
{
    JUCE_ASSERT_MESSAGE_THREAD;

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
    JUCE_ASSERT_MESSAGE_THREAD;

    auto const & data{ mMainContentComponent.getData() };
    jassert(data.speakerSetup.speakers.size() > rowNumber);
    auto const outputPatch{ getSpeakerOutputPatchForRow(rowNumber) };
    auto const & speaker{ data.speakerSetup.speakers[outputPatch] };
    switch (columnNumber) {
    case Cols::X:
        return juce::String{ speaker.position.getCartesian().x, 2 };
    case Cols::Y:
        return juce::String{ speaker.position.getCartesian().y, 2 };
    case Cols::Z:
        return juce::String{ std::max(speaker.position.getCartesian().z, 0.0f), 2 };
    case Cols::AZIMUTH:
        return juce::String{ (HALF_PI - speaker.position.getPolar().azimuth).toDegrees().madePositive().get(), 1 };
    case Cols::ELEVATION:
        return juce::String{ speaker.position.getPolar().elevation.toDegrees().madePositive().get(), 1 };
    case Cols::DISTANCE:
        return juce::String{ speaker.position.getPolar().length, 2 };
    case Cols::OUTPUT_PATCH:
        return juce::String{ outputPatch.get() };
    case Cols::GAIN:
        return juce::String{ speaker.gain.get(), 1 };
    case Cols::HIGHPASS:
        return juce::String{
            speaker.highpassData.map_or([](SpeakerHighpassData const & data) { return data.freq.get(); }, 0.0f),
            1
        };
    case Cols::DIRECT_TOGGLE:
        return juce::String{ static_cast<int>(speaker.isDirectOutOnly) };
    case Cols::DRAG_HANDLE:
        return "=";
    default:
        jassertfalse;
        return "";
    }
}

//==============================================================================
void EditSpeakersWindow::setText(int const columnNumber,
                                 int const rowNumber,
                                 juce::String const & newText,
                                 bool const altDown)
{
    JUCE_ASSERT_MESSAGE_THREAD;
    juce::ScopedReadLock lock{ mMainContentComponent.getLock() };

    auto const spatMode{ mMainContentComponent.getData().speakerSetup.spatMode };

    auto const isEditable = [&](int const col, SpeakerData const & speaker) {
        switch (col) {
        case Cols::OUTPUT_PATCH:
        case Cols::AZIMUTH:
        case Cols::ELEVATION:
        case Cols::GAIN:
        case Cols::HIGHPASS:
        case Cols::X:
        case Cols::Y:
        case Cols::Z:
            return true;
        case Cols::DRAG_HANDLE:
        case Cols::DIRECT_TOGGLE:
        case Cols::DELETE_BUTTON:
            return false;
        case Cols::DISTANCE:
            return spatMode == SpatMode::lbap || speaker.isDirectOutOnly;
        default:
            jassertfalse;
        }
        return false;
    };

    auto const & speakers{ mMainContentComponent.getData().speakerSetup.speakers };

    if (speakers.size() > rowNumber) {
        auto const selectedRows{ mSpeakersTableListBox.getSelectedRows() };
        auto const outputPatch{ getSpeakerOutputPatchForRow(rowNumber) };
        auto const & speaker{ speakers[outputPatch] };

        if (!isEditable(columnNumber, speaker)) {
            return;
        }

        switch (columnNumber) {
        case Cols::X: {
            auto const diff{ newText.getFloatValue() - speaker.position.getCartesian().x };
            for (int i{}; i < selectedRows.size(); ++i) {
                auto const outputPatch_{ getSpeakerOutputPatchForRow(selectedRows[i]) };
                auto const & speaker_{ speakers[outputPatch_] };
                auto const & oldPosition{ speaker_.position };
                auto const & isDirectOutOnly{ speaker_.isDirectOutOnly };
                auto const newPosition{
                    getLegalPosition(oldPosition.translatedX(diff), spatMode, isDirectOutOnly, Cols::X)
                };
                mMainContentComponent.setSpeakerPosition(outputPatch_, newPosition);
            }
            mShouldRefreshSpeakers = true;
            break;
        }
        case Cols::Y: {
            auto const diff{ newText.getFloatValue() - speaker.position.getCartesian().y };
            for (int i{}; i < selectedRows.size(); ++i) {
                auto const outputPatch_{ getSpeakerOutputPatchForRow(selectedRows[i]) };
                auto const & speaker_{ speakers[outputPatch_] };
                auto const & oldPosition{ speaker_.position };
                auto const & isDirectOutOnly{ speaker_.isDirectOutOnly };
                auto const newPosition{
                    getLegalPosition(oldPosition.translatedY(diff), spatMode, isDirectOutOnly, Cols::Y)
                };
                mMainContentComponent.setSpeakerPosition(outputPatch_, newPosition);
            }
            mShouldRefreshSpeakers = true;
            break;
        }
        case Cols::Z: {
            auto const diff{ newText.getFloatValue() - speaker.position.getCartesian().z };
            for (int i{}; i < selectedRows.size(); ++i) {
                auto const outputPatch_{ getSpeakerOutputPatchForRow(selectedRows[i]) };
                auto const & speaker_{ speakers[outputPatch_] };
                auto const & oldPosition{ speaker_.position };
                auto const & isDirectOutOnly{ speaker_.isDirectOutOnly };
                auto const newPosition{
                    getLegalPosition(oldPosition.translatedZ(diff), spatMode, isDirectOutOnly, Cols::Z)
                };
                mMainContentComponent.setSpeakerPosition(outputPatch_, newPosition);
            }
            mShouldRefreshSpeakers = true;
            break;
        }
        case Cols::AZIMUTH: {
            auto const diff{ (HALF_PI - degrees_t{ newText.getFloatValue() }) - speaker.position.getPolar().azimuth };
            for (int i{}; i < selectedRows.size(); ++i) {
                auto const outputPatch_{ getSpeakerOutputPatchForRow(selectedRows[i]) };
                auto const & speaker_{ speakers[outputPatch_] };
                auto const & oldPosition{ speaker_.position };
                auto const & isDirectOutOnly{ speaker_.isDirectOutOnly };
                auto const newPosition{
                    getLegalPosition(oldPosition.rotatedBalancedAzimuth(diff), spatMode, isDirectOutOnly, Cols::AZIMUTH)
                };
                mMainContentComponent.setSpeakerPosition(outputPatch_, newPosition);
            }
            mShouldRefreshSpeakers = true;
            break;
        }
        case Cols::ELEVATION: {
            auto const diff{ degrees_t{ newText.getFloatValue() } - speaker.position.getPolar().elevation };
            for (int i{}; i < selectedRows.size(); ++i) {
                auto const outputPatch_{ getSpeakerOutputPatchForRow(selectedRows[i]) };
                auto const & speaker_{ speakers[outputPatch_] };
                auto const & oldPosition{ speaker_.position };
                auto const & isDirectOutOnly{ speaker_.isDirectOutOnly };
                auto const newPosition{
                    getLegalPosition(oldPosition.elevatedClipped(diff), spatMode, isDirectOutOnly, Cols::ELEVATION)
                };
                mMainContentComponent.setSpeakerPosition(outputPatch_, newPosition);
            }
            mShouldRefreshSpeakers = true;
            break;
        }
        case Cols::DISTANCE: {
            auto const diff{ newText.getFloatValue() - speaker.position.getPolar().length };
            for (int i{}; i < selectedRows.size(); ++i) {
                auto const outputPatch_{ getSpeakerOutputPatchForRow(selectedRows[i]) };
                auto const & speaker_{ speakers[outputPatch_] };
                auto const & oldPosition{ speaker_.position };
                auto const & isDirectOutOnly{ speaker_.isDirectOutOnly };
                auto const newPosition{ getLegalPosition(oldPosition.pushedWithPositiveRadius(diff),
                                                         spatMode,
                                                         isDirectOutOnly,
                                                         Cols::DISTANCE) };
                mMainContentComponent.setSpeakerPosition(outputPatch_, newPosition);
            }
            mShouldRefreshSpeakers = true;
            break;
        }
        case Cols::OUTPUT_PATCH: {
            mMainContentComponent.setShowTriplets(false);
            auto const & oldOutputPatch{ outputPatch };
            output_patch_t newOutputPatch{ std::clamp(newText.getIntValue(), 0, MAX_NUM_SPEAKERS) };
            if (newOutputPatch != oldOutputPatch) {
                if (speakers.contains(newOutputPatch)) {
                    juce::AlertWindow alert("Wrong output patch!    ",
                                            "Sorry! Output patch number " + juce::String(newOutputPatch.get())
                                                + " is already used.",
                                            juce::AlertWindow::WarningIcon);
                    alert.setLookAndFeel(&mLookAndFeel);
                    alert.addButton("OK", 0, juce::KeyPress(juce::KeyPress::returnKey));
                    alert.runModalLoop();
                } else {
                    mMainContentComponent.speakerOutputPatchChanged(oldOutputPatch, newOutputPatch);
                }
            }
            mShouldRefreshSpeakers = true;
            break;
        }
        case Cols::GAIN: {
            static constexpr dbfs_t MIN_GAIN{ -18.0f };
            static constexpr dbfs_t MAX_GAIN{ 6.0f };
            dbfs_t val{ newText.getFloatValue() };
            auto diff = val - speaker.gain;
            val = std::clamp(val, MIN_GAIN, MAX_GAIN);
            mMainContentComponent.setSpeakerGain(outputPatch, val);
            if (selectedRows.size() > 1) {
                for (int i{}; i < selectedRows.size(); ++i) {
                    auto const rowNum{ selectedRows[i] };
                    if (rowNum == rowNumber) {
                        continue;
                    }
                    if (altDown) {
                        auto const g{ std::clamp(speaker.gain + diff, MIN_GAIN, MAX_GAIN) };
                        mMainContentComponent.setSpeakerGain(outputPatch, g);
                    } else {
                        mMainContentComponent.setSpeakerGain(outputPatch, val);
                    }
                }
            }
            break;
        }
        case Cols::HIGHPASS: {
            static constexpr hz_t MIN_FREQ{ 0.0f };
            static constexpr hz_t MAX_FREQ{ 150.0f };
            hz_t val{ newText.getFloatValue() };
            auto diff
                = val
                  - speaker.highpassData.map_or([](SpeakerHighpassData const & data) { return data.freq; }, MIN_FREQ);
            val = std::clamp(val, MIN_FREQ, MAX_FREQ);
            mMainContentComponent.setSpeakerHighPassFreq(outputPatch, val);
            if (mSpeakersTableListBox.getNumSelectedRows() > 1) {
                for (int i{}; i < mSpeakersTableListBox.getSelectedRows().size(); ++i) {
                    auto const rowNum{ mSpeakersTableListBox.getSelectedRows()[i] };
                    if (rowNum == rowNumber) {
                        continue;
                    }
                    if (altDown) {
                        auto const g{ std::clamp(speaker.highpassData->freq + diff, MIN_FREQ, MAX_FREQ) };
                        mMainContentComponent.setSpeakerHighPassFreq(outputPatch, g);
                    } else {
                        mMainContentComponent.setSpeakerHighPassFreq(outputPatch, val);
                    }
                }
            }
            break;
        }
        case Cols::DIRECT_TOGGLE:
            mMainContentComponent.setShowTriplets(false);
            mMainContentComponent.speakerOnlyDirectOutChanged(outputPatch, newText.getIntValue());
            mShouldRefreshSpeakers = true;
            break;
        default:
            break;
        }
    }
    updateWinContent(); // necessary?
    mMainContentComponent.refreshViewportConfig();
}

//==============================================================================
bool EditSpeakersWindow::isMouseOverDragHandle(juce::MouseEvent const & event)
{
    JUCE_ASSERT_MESSAGE_THREAD;

    auto const positionRelativeToSpeakersTableListBox{ event.getEventRelativeTo(&mSpeakersTableListBox).getPosition() };
    auto const speakersTableListBoxBounds{ mSpeakersTableListBox.getBounds() };

    auto const draggableWidth{ mSpeakersTableListBox.getHeader().getColumnWidth(Cols::DRAG_HANDLE) };
    return speakersTableListBoxBounds.contains(positionRelativeToSpeakersTableListBox)
           && positionRelativeToSpeakersTableListBox.getX() < draggableWidth;
}

//==============================================================================
SpeakerData const & EditSpeakersWindow::getSpeakerData(int const rowNum) const
{
    JUCE_ASSERT_MESSAGE_THREAD;

    auto const outputPatch{ getSpeakerOutputPatchForRow(rowNum) };
    return mMainContentComponent.getData().speakerSetup.speakers[outputPatch];
}

//==============================================================================
output_patch_t EditSpeakersWindow::getSpeakerOutputPatchForRow(int const row) const
{
    JUCE_ASSERT_MESSAGE_THREAD;

    auto const & data{ mMainContentComponent.getData() };
    jassert(row >= 0 && row < data.speakerSetup.ordering.size());
    auto const result{ data.speakerSetup.ordering[row] };
    jassert(data.speakerSetup.speakers.contains(result));
    return result;
}

//==============================================================================
// This is overloaded from TableListBoxModel, and should fill in the background of the whole row.
void EditSpeakersWindow::paintRowBackground(juce::Graphics & g,
                                            int const rowNumber,
                                            int const /*width*/,
                                            int const /*height*/,
                                            bool const rowIsSelected)
{
    JUCE_ASSERT_MESSAGE_THREAD;

    juce::ScopedReadLock const lock{ mMainContentComponent.getLock() };
    auto const & speakers{ mMainContentComponent.getData().speakerSetup.speakers };
    if (rowNumber >= speakers.size()) {
        return;
    }

    if (rowIsSelected) {
        // mMainContentComponent.handleSpeakerSelected(outputPatch);
        g.fillAll(mLookAndFeel.getHighlightColour());
    } else {
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
    JUCE_ASSERT_MESSAGE_THREAD;
}

//==============================================================================
juce::Component * EditSpeakersWindow::refreshComponentForCell(int const rowNumber,
                                                              int const columnId,
                                                              bool const /*isRowSelected*/,
                                                              Component * existingComponentToUpdate)
{
    JUCE_ASSERT_MESSAGE_THREAD;

    juce::ScopedReadLock const lock{ mMainContentComponent.getLock() };
    auto const outputPatch{ getSpeakerOutputPatchForRow(rowNumber) };
    auto const & data{ mMainContentComponent.getData() };
    auto const & speaker{ data.speakerSetup.speakers[outputPatch] };

    if (columnId == Cols::DIRECT_TOGGLE) {
        auto * toggleButton{ dynamic_cast<juce::ToggleButton *>(existingComponentToUpdate) };
        if (toggleButton == nullptr) {
            toggleButton = new juce::ToggleButton();
        }
        toggleButton->setName(juce::String(rowNumber + DIRECT_OUT_BUTTON_ID_OFFSET));
        toggleButton->setClickingTogglesState(true);
        toggleButton->setBounds(4, 404, 88, 22);
        toggleButton->addListener(this);
        toggleButton->setToggleState(speaker.isDirectOutOnly, juce::dontSendNotification);
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

    auto const & spatMode{ mMainContentComponent.getData().speakerSetup.spatMode };

    auto const getEditionType = [=]() -> EditionType {
        switch (columnId) {
        case Cols::DISTANCE:
            if (spatMode == SpatMode::lbap || speaker.isDirectOutOnly) {
                return EditionType::valueDraggable;
            }
            return EditionType::notEditable;
        case Cols::X:
        case Cols::Y:
        case Cols::Z:
        case Cols::AZIMUTH:
        case Cols::ELEVATION:
        case Cols::GAIN:
        case Cols::HIGHPASS:
            return EditionType::valueDraggable;
        case Cols::OUTPUT_PATCH:
            return EditionType::editable;
        case Cols::DELETE_BUTTON:
        case Cols::DIRECT_TOGGLE:
            return EditionType::notEditable;
        case Cols::DRAG_HANDLE:
            return EditionType::reorderDraggable;
        default:
            jassertfalse;
            return {};
        }
    };

    static auto const GET_MOUSE_CURSOR = [](EditionType const editionType) {
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
    textLabel->setMouseCursor(GET_MOUSE_CURSOR(editionType));
    textLabel->setEditable(editionType == EditionType::editable || editionType == EditionType::valueDraggable);

    return textLabel;
}

//==============================================================================
void EditSpeakersWindow::mouseDown(juce::MouseEvent const & event)
{
    JUCE_ASSERT_MESSAGE_THREAD;

    if (isMouseOverDragHandle(event)) {
        mDragStartY = event.getEventRelativeTo(&mSpeakersTableListBox).getPosition().getY();
    } else {
        mDragStartY = tl::nullopt;
    }
    pushSelectionToMainComponent();
}

//==============================================================================
void EditSpeakersWindow::mouseDrag(juce::MouseEvent const & event)
{
    JUCE_ASSERT_MESSAGE_THREAD;

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

    auto order{ mMainContentComponent.getData().speakerSetup.ordering };
    order.swap(selectedRow, newIndex);

    mMainContentComponent.reorderSpeakers(order);
    if (mSpeakersTableListBox.contains(eventRel)) {
        *mDragStartY += rowDiff * rowHeight;
    }
    mSpeakersTableListBox.selectRow(newIndex);

    updateWinContent();
}
