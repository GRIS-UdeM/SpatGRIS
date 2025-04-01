/*
 This file is part of SpatGRIS.

 Developers: Gaël Lane Lépine, Samuel Béland, Olivier Bélanger, Nicolas Masson

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

#include "sg_EditSpeakersWindow.hpp"

#include "sg_EditableTextCustomComponent.hpp"
#include "sg_GrisLookAndFeel.hpp"
#include "sg_MainComponent.hpp"
#include "AlgoGRIS/Data/sg_Narrow.hpp"
#include <numbers>
#include <span>

namespace gris
{
//==============================================================================
static Position getLegalSpeakerPosition(Position const & position,
                                        SpatMode const spatMode,
                                        bool const isDirectOutOnly,
                                        [[maybe_unused]] int const modifiedCol)
{
    using Col = EditSpeakersWindow::Cols;

    jassert(modifiedCol == Col::AZIMUTH || modifiedCol == Col::ELEVATION || modifiedCol == Col::DISTANCE
            || modifiedCol == Col::X || modifiedCol == Col::Y || modifiedCol == Col::Z);

    static auto const CONSTRAIN_CARTESIAN
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

    if (spatMode == SpatMode::mbap || isDirectOutOnly) {
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
        CONSTRAIN_CARTESIAN(x, y, z);
    } else if (modifiedCol == Col::Y) {
        y = std::clamp(y, -1.0f, 1.0f);
        CONSTRAIN_CARTESIAN(y, x, z);
    } else {
        jassert(modifiedCol == Col::Z);
        z = std::clamp(z, -1.0f, 1.0f);
        CONSTRAIN_CARTESIAN(z, x, y);
    }
    return Position{ newPosition };
}

//==============================================================================
LabelWrapper::LabelWrapper(GrisLookAndFeel & lookAndFeel)
{
    label.setJustificationType(juce::Justification::right);
    label.setFont(lookAndFeel.getFont());
    label.setLookAndFeel(&lookAndFeel);
    label.setColour(juce::Label::textColourId, lookAndFeel.getFontColour());
}

LabelTextEditorWrapper::LabelTextEditorWrapper(GrisLookAndFeel & lookAndFeel) : LabelWrapper(lookAndFeel)
{
    editor.setColour(juce::ToggleButton::textColourId, lookAndFeel.getFontColour());
    editor.setLookAndFeel(&lookAndFeel);
}

LabelComboBoxWrapper::LabelComboBoxWrapper(GrisLookAndFeel & lookAndFeel) : LabelWrapper(lookAndFeel)
{
    comboBox.setLookAndFeel(&lookAndFeel);
}

//==============================================================================
EditSpeakersWindow::EditSpeakersWindow(juce::String const & name,
                                       GrisLookAndFeel & lookAndFeel,
                                       MainContentComponent & mainContentComponent,
                                       juce::String const & /*configName*/)
    : DocumentWindow(name, lookAndFeel.getBackgroundColour(), allButtons)
    , mMainContentComponent(mainContentComponent)
    , mLookAndFeel(lookAndFeel)
    , mViewportWrapper(lookAndFeel, "Configuration Speakers")
    , mFont(juce::FontOptions().withHeight (14.f))
    , mRingSpeakers(lookAndFeel)
    , mRingElevation(lookAndFeel)
    , mRingRadius(lookAndFeel)
    , mRingOffsetAngle(lookAndFeel)
    , mPolyFaces(lookAndFeel)
    , mPolyX(lookAndFeel)
    , mPolyY(lookAndFeel)
    , mPolyZ(lookAndFeel)
    , mPolyRadius(lookAndFeel)
    , mPolyAzimuthOffset(lookAndFeel)
    , mPolyElevOffset(lookAndFeel)
{
    JUCE_ASSERT_MESSAGE_THREAD;

    auto setupButton = [this](juce::TextButton & b, juce::StringRef text) {
        b.setButtonText(text);
        b.addListener(this);
        b.setColour(juce::ToggleButton::textColourId, mLookAndFeel.getFontColour());
        b.setLookAndFeel(&mLookAndFeel);
        mViewportWrapper.getContent()->addAndMakeVisible(b);
    };

    setupButton(mAddSpeakerButton, "Add Speaker");
    setupButton(mSaveAsSpeakerSetupButton, "Save As...");
    setupButton(mSaveSpeakerSetupButton, "Save");

    auto setupWrapper = [this](LabelWrapper * w,
                               juce::StringRef labelText,
                               juce::StringRef tooltip,
                               juce::StringRef editorText,
                               int maxLength,
                               juce::StringRef allowedCharacters,
                               juce::StringArray comboItemList = {}) {
        w->label.setText(labelText, juce::NotificationType::dontSendNotification);
        mViewportWrapper.getContent()->addAndMakeVisible(w->label);

        if (auto * labelTextEditor{ dynamic_cast<LabelTextEditorWrapper *>(w) }) {
            labelTextEditor->editor.setTooltip(tooltip);
            labelTextEditor->editor.setText(editorText, false);
            labelTextEditor->editor.setInputRestrictions(maxLength, allowedCharacters);
            labelTextEditor->editor.addListener(this);

            mViewportWrapper.getContent()->addAndMakeVisible(labelTextEditor->editor);
        } else if (auto * labelComboBox{ dynamic_cast<LabelComboBoxWrapper *>(w) }) {
            labelComboBox->comboBox.addItemList(comboItemList, 1);
            labelComboBox->comboBox.setSelectedId(1);
            labelComboBox->comboBox.setTooltip(tooltip);
            mViewportWrapper.getContent()->addAndMakeVisible(labelComboBox->comboBox);
        }
    };

    // Generate ring of speakers.
    setupWrapper(&mRingSpeakers, "# of speakers", "Number of speakers in the ring.", "8", 3, "0123456789");
    setupWrapper(&mRingElevation, "Elevation", "Elevation angle of the ring.", "0.0", 6, "-0123456789.");
    setupWrapper(&mRingRadius, "Distance", "Distance of the speakers from the center.", "1.0", 6, "0123456789.");
    setupWrapper(&mRingOffsetAngle, "Offset Angle", "Offset angle of the first speaker.", "0.0", 6, "-0123456789.");
    setupButton(mAddRingButton, "Add Ring");

    // Polyhedron of speakers.
    setupWrapper(&mPolyFaces, "# of faces", "Number of faces/speakers for the polyhedron.", {}, {}, {}, { "4", "6", "8", "12", "20" });
    setupWrapper(&mPolyX, "X", "X position for the center of the polyhedron, in the [-1.667, 1.667] range", "0", 4, "-0123456789.");
    setupWrapper(&mPolyY, "Y", "Y position for the center of the polyhedron.", "0", 4, "-0123456789.");
    setupWrapper(&mPolyZ, "Z", "Z position for the center of the polyhedron.", "0", 4, "-0123456789.");
    setupWrapper(&mPolyRadius, "Radius", "Radius for the polyhedron.", "1", 4, "0123456789.");
    setupWrapper(&mPolyAzimuthOffset, "Azimuth offset", "Azimuth rotation for the polyhedron shape, in degrees.", "0", 3, "0123456789");
    setupWrapper(&mPolyElevOffset, "Elevation offset", "Azimuth rotation for the polyhedron shape, in degrees.", "0", 3, "0123456789");
    setupButton(mAddPolyButton, "Add Polyhedron");
    togglePolyhedraExtraWidgets();

    // Pink noise controls.
    mPinkNoiseToggleButton.setButtonText("Reference Pink Noise");
    mPinkNoiseToggleButton.addListener(this);
    mPinkNoiseToggleButton.setColour(juce::ToggleButton::textColourId, mLookAndFeel.getFontColour());
    mPinkNoiseToggleButton.setLookAndFeel(&mLookAndFeel);
    mViewportWrapper.getContent()->addAndMakeVisible(mPinkNoiseToggleButton);

    mPinkNoiseGainSlider.setTextValueSuffix(" dB");
    mPinkNoiseGainSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
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
    mViewportWrapper.getContent()->addAndMakeVisible(mPinkNoiseGainSlider);

    // Sound diffusion controls
    mDiffusionLabel.setText("Global Sound Diffusion", juce::NotificationType::dontSendNotification);
    mDiffusionLabel.setFont(mLookAndFeel.getFont());
    mDiffusionLabel.setLookAndFeel(&mLookAndFeel);
    mDiffusionLabel.setColour(juce::Label::textColourId, mLookAndFeel.getFontColour());
    mViewportWrapper.getContent()->addAndMakeVisible(mDiffusionLabel);

    mDiffusionSlider.setTooltip("Adjuts the spreading range of sources sound");
    mDiffusionSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    mDiffusionSlider.setRotaryParameters(juce::MathConstants<float>::pi * 1.3f,
                                         juce::MathConstants<float>::pi * 2.7f,
                                         true);
    mDiffusionSlider.setRange(0.0f, 1.0f, 0.01f);
    mDiffusionSlider.setValue(mMainContentComponent.getData().speakerSetup.diffusion);
    mDiffusionSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 20);
    mDiffusionSlider.setColour(juce::ToggleButton::textColourId, mLookAndFeel.getFontColour());
    mDiffusionSlider.setLookAndFeel(&mLookAndFeel);
    mDiffusionSlider.addListener(this);
    mDiffusionSlider.setEnabled(mMainContentComponent.getData().project.spatMode == SpatMode::mbap
                                || mMainContentComponent.getData().project.spatMode == SpatMode::hybrid);
    mViewportWrapper.getContent()->addAndMakeVisible(mDiffusionSlider);

    mViewportWrapper.getContent()->addAndMakeVisible(mSpeakersTableListBox);

    mViewportWrapper.repaint();
    mViewportWrapper.resized();

    mViewportWrapper.addMouseListener(this, true);

    setResizable(true, true);
    setUsingNativeTitleBar(true);
    setAlwaysOnTop(true);

    setContentNonOwned(&mViewportWrapper, false);
    auto const & controlsComponent{ *mainContentComponent.getControlsComponent() };

    static constexpr auto WIDTH = 850;
    static constexpr auto HEIGHT = 600;
    static constexpr auto TITLE_BAR_HEIGHT = 30;

    setBounds(controlsComponent.getScreenX(), controlsComponent.getScreenY() + TITLE_BAR_HEIGHT, WIDTH, HEIGHT);

    DocumentWindow::setVisible(true);
}

//==============================================================================
EditSpeakersWindow::~EditSpeakersWindow()
{
    mSpeakersTableListBox.setModel(nullptr);
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

    mViewportWrapper.setBounds(0, 0, getWidth(), getHeight());
    mViewportWrapper.correctSize(getWidth() - 8, getHeight());
    mSpeakersTableListBox.setSize(getWidth(), 400);

    mSpeakersTableListBox.updateContent();

    mViewportWrapper.repaint();
    mViewportWrapper.resized();
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
    jassert(slider == &mPinkNoiseGainSlider || slider == &mDiffusionSlider);

    if (slider == &mPinkNoiseGainSlider && mPinkNoiseToggleButton.getToggleState()) {
        dbfs_t const db{ narrow<float>(mPinkNoiseGainSlider.getValue()) };
        mMainContentComponent.setPinkNoiseGain(db);
    } else if (slider == &mDiffusionSlider) {
        mMainContentComponent.setSpeakerSetupDiffusion(static_cast<float>(slider->getValue()));
        mShouldComputeSpeakers = true;

        if (juce::KeyPress::isKeyCurrentlyDown(juce::KeyPress::returnKey)) {
            computeSpeakers();
        }
    }
}

//==============================================================================
void EditSpeakersWindow::addSpeakerGroup(int numSpeakers, std::function<Position(int)> getSpeakerPosition)
{
    static auto constexpr GET_SELECTED_ROW = [](juce::TableListBox const & tableListBox) -> tl::optional<int> {
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

    output_patch_t newOutputPatch{};

    if (mMainContentComponent.getMaxSpeakerOutputPatch().get() + numSpeakers > MAX_NUM_SPEAKERS) {
        return;
    }

    for (int i{}; i < numSpeakers; i++) {
        tl::optional<output_patch_t> outputPatch{};
        tl::optional<int> index{};

        if (selectedRow) {
            outputPatch = getSpeakerOutputPatchForRow(*selectedRow);
            index = *selectedRow;
            *selectedRow += 1;
        }

        newOutputPatch = mMainContentComponent.addSpeaker(outputPatch, index);
        mNumRows = speakers.size();

        mMainContentComponent.setSpeakerPosition(newOutputPatch, getSpeakerPosition(i));
    }
    mMainContentComponent.refreshSpeakers();
    updateWinContent();
    // TableList needs different sorting parameters to trigger the sorting function.
    mSpeakersTableListBox.getHeader().setSortColumnId(sortColumnId, !sortedForwards);
    // This is the real sorting!
    mSpeakersTableListBox.getHeader().setSortColumnId(sortColumnId, sortedForwards);
    selectSpeaker(newOutputPatch);
    mShouldComputeSpeakers = true;
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
    auto selectedRow{ GET_SELECTED_ROW(mSpeakersTableListBox) };

    // mMainContentComponent.setShowTriplets(false);

    if (button == &mAddSpeakerButton) {
        // Add speaker button
        if (mMainContentComponent.getMaxSpeakerOutputPatch().get() >= MAX_NUM_SPEAKERS) {
            return;
        }

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
        mShouldComputeSpeakers = true;
    } else if (button == &mSaveAsSpeakerSetupButton) {
        // Save as... speaker button
        mShouldComputeSpeakers = true;
        computeSpeakers();
        mMainContentComponent.saveAsEditedSpeakerSetup();
    } else if (button == &mSaveSpeakerSetupButton) {
        // Save speaker button
        mShouldComputeSpeakers = true;
        computeSpeakers();
        mMainContentComponent.saveEditedSpeakerSetup();
    } else if (button == &mAddRingButton) {
        auto getPosition = [this](int i) -> Position {
            const auto numSpeakers{ mRingSpeakers.getText<float>() };

            // Calculate the azimuth angle by distributing speakers evenly in a circle
            // -360.0f / numSpeakers * i -> Spreads the speakers evenly around the circle
            // -mRingOffsetAngle.getText<float>() -> Applies an offset to shift the ring
            // +90.0f -> Aligns the 0-degree point with the correct reference direction
            degrees_t azimuth{ -360.0f / numSpeakers * narrow<float>(i) - mRingOffsetAngle.getText<float>() + 90.0f };

            // If elevation is below the 135° elevationHalfPoint, treat it as part of the lower hemisphere (0° to 90°),
            // otherwise, assume it's in the upper hemisphere (270° to 360°).
            auto const elev{ mRingElevation.getText<float>() };
            degrees_t const zenith{ elev < elevationHalfPoint ? std::clamp(elev, 0.0f, 90.0f)
                                                              : std::clamp(elev, 270.0f, 360.0f) };

            auto const radius{ mRingRadius.getText<float>() };
            return Position{ PolarVector{ radians_t{ azimuth.centered() }, radians_t{ zenith }, radius } };
        };

        addSpeakerGroup(mRingSpeakers.getText<int>(), getPosition);
    } else if (button == &mAddPolyButton) {
        const auto getPosition = [this](int i) -> Position {
            const auto numFaces = mPolyFaces.getSelectionAsInt();
            const auto radius = mPolyRadius.getText<float>();
            const auto azimOffset = mPolyAzimuthOffset.getText<float>() * PI.get() / 180.0f; // Convert to radians
            const auto elevOffset = mPolyElevOffset.getText<float>() * PI.get() / 180.0f;    // Convert to radians
            const auto centerX = mPolyX.getText<float>();
            const auto centerY = mPolyY.getText<float>();
            const auto centerZ = mPolyZ.getText<float>();

            using Vec3 = std::array<float, 3>;
            const auto vertices = [&numFaces]() -> std::span<const Vec3> {
                static constexpr std::array<Vec3, 4> tetrahedron
                    = { Vec3{ 1.f, 1.f, 1.f }, { -1.f, -1.f, 1.f }, { -1.f, 1.f, -1.f }, { 1.f, -1.f, -1.f } };

                static constexpr std::array<Vec3, 6> cube
                    = { Vec3{ 1.f, 0.f, 0.f }, { -1.f, 0.f, 0.f }, { 0.f, 1.f, 0.f },
                        { 0.f, -1.f, 0.f },    { 0.f, 0.f, 1.f },  { 0.f, 0.f, -1.f } };

                static constexpr std::array<Vec3, 8> octahedron
                    = { Vec3{ 1.f, 1.f, 1.f }, { 1.f, 1.f, -1.f },  { 1.f, -1.f, 1.f },  { 1.f, -1.f, -1.f },
                        { -1.f, 1.f, 1.f },    { -1.f, 1.f, -1.f }, { -1.f, -1.f, 1.f }, { -1.f, -1.f, -1.f } };

                static constexpr std::array<Vec3, 12> dodecahedron = {
                    Vec3{ 0.f, 1.f, 1.618f }, { 0.f, 1.f, -1.618f }, { 0.f, -1.f, 1.618f }, { 0.f, -1.f, -1.618f },
                    { 1.f, 1.618f, 0.f },     { 1.f, -1.618f, 0.f }, { -1.f, 1.618f, 0.f }, { -1.f, -1.618f, 0.f },
                    { 1.618f, 0.f, 1.f },     { 1.618f, 0.f, -1.f }, { -1.618f, 0.f, 1.f }, { -1.618f, 0.f, -1.f }
                };

                static constexpr std::array<Vec3, 20> icosahedron
                    = { Vec3{ 1.f, 1.f, 1.f },     { 1.f, 1.f, -1.f },       { 1.f, -1.f, 1.f },
                        { 1.f, -1.f, -1.f },       { -1.f, 1.f, 1.f },       { -1.f, 1.f, -1.f },
                        { -1.f, -1.f, 1.f },       { -1.f, -1.f, -1.f },     { 0.f, 1.618f, 0.618f },
                        { 0.f, 1.618f, -0.618f },  { 0.f, -1.618f, 0.618f }, { 0.f, -1.618f, -0.618f },
                        { 0.618f, 0.f, 1.618f },   { 0.618f, 0.f, -1.618f }, { -0.618f, 0.f, 1.618f },
                        { -0.618f, 0.f, -1.618f }, { 1.618f, 0.618f, 0.f },  { 1.618f, -0.618f, 0.f },
                        { -1.618f, 0.618f, 0.f },  { -1.618f, -0.618f, 0.f } };

                switch (numFaces) {
                case 4:
                    return std::span{ tetrahedron };
                case 6:
                    return std::span{ cube };
                case 8:
                    return std::span{ octahedron };
                case 12:
                    return std::span{ dodecahedron };
                case 20:
                    return std::span{ icosahedron };
                default:
                    jassertfalse;
                    return std::span{ tetrahedron };
                }
            }();

            if (i < 0 || i >= vertices.size()) {
                jassertfalse;
                return Position{ { 0, 0, 0 } };
            }

            const auto & curVertex = vertices[i];
            const auto norm = std::hypot(curVertex[0], curVertex[1], curVertex[2]);

            // Normalize and scale to radius
            const auto x = radius * curVertex[0] / norm;
            const auto y = radius * curVertex[1] / norm;
            const auto z = radius * curVertex[2] / norm;

            // Apply azimuth rotation (around Z-axis)
            const auto sinAzim{ std::sin(azimOffset) };
            const auto cosAzim{ std::cos(azimOffset) };
            const auto xAzim = x * cosAzim - y * sinAzim;
            const auto yAzim = x * sinAzim + y * cosAzim;

            // Apply elevation rotation (around Y-axis)
            const auto sinElev{ std::sin(elevOffset) };
            const auto cosElev{ std::cos(elevOffset) };
            const auto xRot = xAzim * cosElev + z * sinElev;
            const auto zRot = -xAzim * sinElev + z * cosElev;

            return Position{ CartesianVector{ centerX + xRot, centerY + yAzim, centerZ + zRot } };
        };

        const auto numFaces{ mPolyFaces.getSelectionAsInt() };
        jassert(numFaces == 4 || numFaces == 6 || numFaces == 8 || numFaces == 12 || numFaces == 20);
        addSpeakerGroup(numFaces, getPosition);
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
        mShouldComputeSpeakers = true;
    } else {
        // Direct out
        jassert(button->getName().getIntValue() >= DIRECT_OUT_BUTTON_ID_OFFSET);
        auto const row{ button->getName().getIntValue() - DIRECT_OUT_BUTTON_ID_OFFSET };
        auto const outputPatch{ getSpeakerOutputPatchForRow(row) };
        mMainContentComponent.speakerDirectOutOnlyChanged(outputPatch, button->getToggleState());
        updateWinContent();
        mShouldComputeSpeakers = true;
    }

    computeSpeakers();
}

//==============================================================================
void EditSpeakersWindow::textEditorReturnKeyPressed(juce::TextEditor & /*textEditor*/)
{
    JUCE_ASSERT_MESSAGE_THREAD;

    unfocusAllComponents();
}

//==============================================================================
void EditSpeakersWindow::textEditorFocusLost(juce::TextEditor & textEditor)
{
    JUCE_ASSERT_MESSAGE_THREAD;

    const auto intValue {textEditor.getText().getIntValue()};
    const auto floatValue{ textEditor.getText().getFloatValue() };
    const auto spatMode{ mMainContentComponent.getData().speakerSetup.spatMode };

    // technically in dome/vbap mode the polyhedra is clamped right on the radius, so this calculation is moot, but
    // leaving the option here for a variable dome radius, in case it's ever useful in the future.
    const auto maxPolyRadius{ spatMode == SpatMode::mbap ? MBAP_EXTENDED_RADIUS : NORMAL_RADIUS };
    const auto clampPolyXYZ = [&textEditor, &floatValue, radius{ mPolyRadius.getText<float>() }, &maxPolyRadius]() {
        if ((floatValue - radius < -maxPolyRadius))
            textEditor.setText(juce::String(radius - maxPolyRadius));
        else if (floatValue + radius > maxPolyRadius)
            textEditor.setText(juce::String(maxPolyRadius - radius));
    };

    if (&textEditor == &mRingSpeakers.editor) {
        auto const value{ std::clamp(intValue, 2, 64) };
        textEditor.setText(juce::String{ value }, false);
        mShouldComputeSpeakers = true;
    } else if (&textEditor == &mRingElevation.editor) {
        auto const value{ floatValue < elevationHalfPoint ? std::clamp(floatValue, 0.0f, 90.0f)
                                             : std::clamp(floatValue, 270.0f, 360.0f) };
        textEditor.setText(juce::String{ value, 1 }, false);
        mShouldComputeSpeakers = true;
    } else if (&textEditor == &mRingRadius.editor) {
        juce::ScopedReadLock const lock{ mMainContentComponent.getLock() };
        auto const minRadius{ spatMode == SpatMode::mbap ? 0.001f : NORMAL_RADIUS };
        auto const maxRadius{ spatMode == SpatMode::mbap ? SQRT3 : NORMAL_RADIUS };
        auto const value{ std::clamp(floatValue, minRadius, maxRadius) };
        textEditor.setText(juce::String{ value, 1 }, false);
        mShouldComputeSpeakers = true;
    } else if (&textEditor == &mRingOffsetAngle.editor) {
        auto const value{ std::clamp(floatValue, -360.0f, 360.0f) };
        textEditor.setText(juce::String{ value, 1 }, false);
        mShouldComputeSpeakers = true;
    } else if (&textEditor == &mPolyX.editor){
        clampPolyXYZ();
    } else if (&textEditor == &mPolyY.editor) {
        clampPolyXYZ();
    } else if (&textEditor == &mPolyZ.editor) {
        clampPolyXYZ();
    } else if (&textEditor == &mPolyRadius.editor) {
        const auto x{ mPolyX.getText<float>() };
        const auto y{ mPolyY.getText<float>() };
        const auto z{ mPolyZ.getText<float>() };

        if (x - floatValue < -maxPolyRadius)
            textEditor.setText(juce::String(x - maxPolyRadius));
        else if (x + floatValue > maxPolyRadius)
            textEditor.setText(juce::String(maxPolyRadius - x));

        else if (y - floatValue < -maxPolyRadius)
            textEditor.setText(juce::String(y - maxPolyRadius));
        else if (y + floatValue > maxPolyRadius)
            textEditor.setText(juce::String(maxPolyRadius - y));

        else if (z - floatValue < -maxPolyRadius)
            textEditor.setText(juce::String(z - maxPolyRadius));
        else if (z + floatValue > maxPolyRadius)
            textEditor.setText(juce::String(maxPolyRadius - z));
    }

    computeSpeakers();
}

void EditSpeakersWindow::togglePolyhedraExtraWidgets()
{
    const auto showExtendedPolyWidgets = mMainContentComponent.getData().speakerSetup.spatMode != SpatMode::vbap;
    mPolyX.setVisible(showExtendedPolyWidgets);
    mPolyY.setVisible(showExtendedPolyWidgets);
    mPolyZ.setVisible(showExtendedPolyWidgets);
    mPolyRadius.setVisible(showExtendedPolyWidgets);
}

//==============================================================================
void EditSpeakersWindow::updateWinContent()
{
    JUCE_ASSERT_MESSAGE_THREAD;

    mNumRows = mMainContentComponent.getData().speakerSetup.speakers.size();
    mSpeakersTableListBox.updateContent();
    mDiffusionSlider.setValue(mMainContentComponent.getData().speakerSetup.diffusion);
    mDiffusionSlider.setEnabled(mMainContentComponent.getData().project.spatMode == SpatMode::mbap
                                || mMainContentComponent.getData().project.spatMode == SpatMode::hybrid);

togglePolyhedraExtraWidgets();
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

    mMainContentComponent.setPinkNoiseGain(tl::nullopt);
    mMainContentComponent.closeSpeakersConfigurationWindow();
}

//==============================================================================
bool EditSpeakersWindow::keyPressed(const juce::KeyPress &key)
{
    auto const key_w{ juce::KeyPress(87) };
    if (key.getModifiers().isCommandDown() && key.isKeyCurrentlyDown(key_w.getKeyCode())) {
        mMainContentComponent.setPinkNoiseGain(tl::nullopt);
        mMainContentComponent.closeSpeakersConfigurationWindow();
        return true;
    }
    return false;
}

//==============================================================================
void EditSpeakersWindow::resized()
{
    JUCE_ASSERT_MESSAGE_THREAD;

    //viewport
    auto bounds{ getLocalBounds() };
    mViewportWrapper.setBounds(bounds);
    mViewportWrapper.correctSize(getWidth() - 10, getHeight() - 30);

    //speaker table
    auto const secondRowH{ 24 };
    auto const bottomPanelH{ 195 + secondRowH };
    mSpeakersTableListBox.setSize(getWidth(), getHeight() - bottomPanelH);

    //first row of bottom panel with add speaker, save as, and save buttons
    auto const firstRowH {22};
    auto const firstRowY{ getHeight() - 180 };
    mAddSpeakerButton.setBounds(5, firstRowY, 100, firstRowH);
    mSaveAsSpeakerSetupButton.setBounds(getWidth() - 210, firstRowY, 100, firstRowH);
    mSaveSpeakerSetupButton.setBounds(getWidth() - 105, firstRowY, 100, firstRowH);

    //second row of bottom panel with rings of speakers
    auto const secondRowY{ getHeight() - 140 };
    auto const positionWidget = [](LabelWrapper* w, int x, int y, int lw, int ew) {
        w->label.setBounds(x, y, lw, secondRowH);
        if (auto * lew{ dynamic_cast<LabelTextEditorWrapper*>(w) })
            lew->editor.setBounds(x + lw, y, ew, secondRowH);
        else if (auto * lcw{ dynamic_cast<LabelComboBoxWrapper *>(w) })
            lcw->comboBox.setBounds(x + lw, y, ew, secondRowH);
    };

    positionWidget(&mRingSpeakers, 5, secondRowY, 80, 40);
    positionWidget(&mRingElevation, 120, secondRowY, 80, 60);
    positionWidget(&mRingRadius, 255, secondRowY, 80, 60);
    positionWidget(&mRingOffsetAngle, 400, secondRowY, 80, 60);
    mAddRingButton.setBounds(getWidth() - 105, secondRowY, 100, 24);

    //third row of bottom panel with polyhedra controls
    auto const thirdRowY { secondRowY + secondRowH + 2};
    positionWidget(&mPolyFaces, 5, thirdRowY, 70, 50);
    auto startingX {120};
    auto const shortlabelW {30};
    auto const shortEditorW{ 40 };
    positionWidget(&mPolyX, startingX, thirdRowY, shortlabelW, shortEditorW);
    startingX += shortlabelW + shortEditorW;
    positionWidget(&mPolyY, startingX, thirdRowY, shortlabelW, shortEditorW);
    startingX += shortlabelW + shortEditorW;
    positionWidget(&mPolyZ, startingX, thirdRowY, shortlabelW, shortEditorW);
    startingX += shortlabelW + shortEditorW;
    positionWidget(&mPolyRadius, startingX, thirdRowY, 50, 40);
    startingX += 50 + 40;
    positionWidget(&mPolyAzimuthOffset, startingX, thirdRowY, 90, 40);
    startingX += 90 + 40;
    positionWidget(&mPolyElevOffset, startingX, thirdRowY, 100, 40);
    mAddPolyButton.setBounds(getWidth() - 105, thirdRowY, 100, 24);

    mPinkNoiseToggleButton.setBounds(5, getHeight() - 70, 150, 24);
    mPinkNoiseGainSlider.setBounds(170, getHeight() - 95, 60, 60);
    mDiffusionLabel.setBounds(getWidth() - 247, getHeight() - 70, 160, 24);
    mDiffusionSlider.setBounds(getWidth() - 88, getHeight() - 95, 60, 60);
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
        return juce::String{ speaker.position.getCartesian().z, 2 };
    case Cols::AZIMUTH:
        return juce::String{ degrees_t{ HALF_PI - speaker.position.getPolar().azimuth }.madePositive().get(), 1 };
    case Cols::ELEVATION:
        return juce::String{ degrees_t{ speaker.position.getPolar().elevation }.madePositive().get(), 1 };
    case Cols::DISTANCE:
        return juce::String{ speaker.position.getPolar().length, 2 };
    case Cols::OUTPUT_PATCH:
        return juce::String{ outputPatch.get() };
    case Cols::GAIN:
        return juce::String{ speaker.gain.get(), 1 };
    case Cols::HIGHPASS:
        return juce::String{
            speaker.highpassData.map_or([](SpeakerHighpassData const & data) { return data.freq.get(); }, float{}),
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
        case Cols::GAIN:
        case Cols::HIGHPASS:
            return true;
        case Cols::AZIMUTH:
        case Cols::ELEVATION:
            return spatMode == SpatMode::vbap;
        case Cols::X:
        case Cols::Y:
        case Cols::Z:
            return spatMode == SpatMode::mbap;
        case Cols::DRAG_HANDLE:
        case Cols::DIRECT_TOGGLE:
        case Cols::DELETE_BUTTON:
            return false;
        case Cols::DISTANCE:
            return spatMode == SpatMode::vbap && speaker.isDirectOutOnly;
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
                    getLegalSpeakerPosition(oldPosition.translatedX(diff), spatMode, isDirectOutOnly, Cols::X)
                };
                mMainContentComponent.setSpeakerPosition(outputPatch_, newPosition);
            }
            mShouldComputeSpeakers = true;
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
                    getLegalSpeakerPosition(oldPosition.translatedY(diff), spatMode, isDirectOutOnly, Cols::Y)
                };
                mMainContentComponent.setSpeakerPosition(outputPatch_, newPosition);
            }
            mShouldComputeSpeakers = true;
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
                    getLegalSpeakerPosition(oldPosition.translatedZ(diff), spatMode, isDirectOutOnly, Cols::Z)
                };
                mMainContentComponent.setSpeakerPosition(outputPatch_, newPosition);
            }
            mShouldComputeSpeakers = true;
            break;
        }
        case Cols::AZIMUTH: {
            radians_t const diff{ degrees_t{ 90.0f } - degrees_t{ newText.getFloatValue() }
                                  - degrees_t{ speaker.position.getPolar().azimuth } };
            for (int i{}; i < selectedRows.size(); ++i) {
                auto const outputPatch_{ getSpeakerOutputPatchForRow(selectedRows[i]) };
                auto const & speaker_{ speakers[outputPatch_] };
                auto const & oldPosition{ speaker_.position };
                auto const & isDirectOutOnly{ speaker_.isDirectOutOnly };
                auto const newPosition{ getLegalSpeakerPosition(oldPosition.rotatedBalancedAzimuth(diff),
                                                                spatMode,
                                                                isDirectOutOnly,
                                                                Cols::AZIMUTH) };
                mMainContentComponent.setSpeakerPosition(outputPatch_, newPosition);
            }
            mShouldComputeSpeakers = true;
            break;
        }
        case Cols::ELEVATION: {
            radians_t const diff{ degrees_t{ newText.getFloatValue() }
                                  - degrees_t{ speaker.position.getPolar().elevation } };
            for (int i{}; i < selectedRows.size(); ++i) {
                auto const outputPatch_{ getSpeakerOutputPatchForRow(selectedRows[i]) };
                auto const & speaker_{ speakers[outputPatch_] };
                auto const & oldPosition{ speaker_.position };
                auto const & isDirectOutOnly{ speaker_.isDirectOutOnly };
                auto const newPosition{ getLegalSpeakerPosition(oldPosition.elevatedClipped(diff),
                                                                spatMode,
                                                                isDirectOutOnly,
                                                                Cols::ELEVATION) };
                mMainContentComponent.setSpeakerPosition(outputPatch_, newPosition);
            }
            mShouldComputeSpeakers = true;
            break;
        }
        case Cols::DISTANCE: {
            auto const diff{ newText.getFloatValue() - speaker.position.getPolar().length };
            for (int i{}; i < selectedRows.size(); ++i) {
                auto const outputPatch_{ getSpeakerOutputPatchForRow(selectedRows[i]) };
                auto const & speaker_{ speakers[outputPatch_] };
                auto const & oldPosition{ speaker_.position };
                auto const & isDirectOutOnly{ speaker_.isDirectOutOnly };
                auto const newPosition{ getLegalSpeakerPosition(oldPosition.pushedWithPositiveRadius(diff),
                                                                spatMode,
                                                                isDirectOutOnly,
                                                                Cols::DISTANCE) };
                mMainContentComponent.setSpeakerPosition(outputPatch_, newPosition);
            }
            mShouldComputeSpeakers = true;
            break;
        }
        case Cols::OUTPUT_PATCH: {
            mMainContentComponent.setShowTriplets(false);
            auto const & oldOutputPatch{ outputPatch };
            output_patch_t newOutputPatch{ std::clamp(newText.getIntValue(), 1, MAX_NUM_SPEAKERS) };
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
            mShouldComputeSpeakers = true;
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
            static constexpr hz_t OFF_FREQ{ 0.0f };
            static constexpr hz_t MIN_FREQ{ 20.0f };
            static constexpr hz_t MAX_FREQ{ 150.0f };
            hz_t val{ newText.getFloatValue() };
            auto diff = val
                        - speaker.highpassData.map_or([](SpeakerHighpassData const & data) { return data.freq; },
                                                      hz_t{ OFF_FREQ });
            auto mapValue = [&](hz_t value, hz_t modDiff) {
                if (value <= OFF_FREQ && modDiff <= OFF_FREQ) {
                    value = OFF_FREQ;
                } else if (value < MIN_FREQ && value > OFF_FREQ && modDiff < OFF_FREQ) {
                    value = OFF_FREQ;
                } else {
                    value = std::clamp(value, MIN_FREQ, MAX_FREQ);
                }
                return value;
            };
            val = mapValue(val, diff);
            mMainContentComponent.setSpeakerHighPassFreq(outputPatch, val);
            if (mSpeakersTableListBox.getNumSelectedRows() > 1) {
                for (int i{}; i < mSpeakersTableListBox.getSelectedRows().size(); ++i) {
                    auto const rowNum{ mSpeakersTableListBox.getSelectedRows()[i] };
                    if (rowNum == rowNumber) {
                        continue;
                    }
                    if (altDown) {
                        auto g{ speaker.highpassData->freq + diff };
                        g = mapValue(g, diff);
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
            mMainContentComponent.speakerDirectOutOnlyChanged(outputPatch, newText.getIntValue());
            mShouldComputeSpeakers = true;
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
void EditSpeakersWindow::computeSpeakers()
{
    if (mShouldComputeSpeakers) {
        mMainContentComponent.refreshSpeakers();
        mShouldComputeSpeakers = false;
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
            if (spatMode == SpatMode::vbap && speaker.isDirectOutOnly) {
                return EditionType::valueDraggable;
            }
            return EditionType::notEditable;
        case Cols::X:
        case Cols::Y:
        case Cols::Z:
            if (spatMode == SpatMode::mbap) {
                return EditionType::valueDraggable;
            }
            return EditionType::notEditable;
        case Cols::AZIMUTH:
        case Cols::ELEVATION:
            if (spatMode == SpatMode::vbap) {
                return EditionType::valueDraggable;
            }
            return EditionType::notEditable;
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
    textLabel->setColor(editionType != EditionType::notEditable);

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

    auto order = mMainContentComponent.getData().speakerSetup.ordering;
    order.swap(selectedRow, newIndex);

    mMainContentComponent.reorderSpeakers(order);
    if (mSpeakersTableListBox.contains(eventRel)) {
        *mDragStartY += rowDiff * rowHeight;
    }
    mSpeakersTableListBox.selectRow(newIndex);

    updateWinContent();
    mShouldComputeSpeakers = true;
}

//==============================================================================
void EditSpeakersWindow::mouseUp(juce::MouseEvent const & /*event*/)
{
    JUCE_ASSERT_MESSAGE_THREAD;

    computeSpeakers();
}

} // namespace gris
