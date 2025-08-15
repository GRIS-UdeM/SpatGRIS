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

#include "AlgoGRIS/Data/sg_Narrow.hpp"
#include "sg_GrisLookAndFeel.hpp"
#include "sg_MainComponent.hpp"
#include <numbers>
#include <span>

namespace gris
{
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
                                       juce::UndoManager& undoMan)
    : DocumentWindow(name, lookAndFeel.getBackgroundColour(), allButtons)
    , mMainContentComponent(mainContentComponent)
    , spatGrisData(mainContentComponent.getData())
    , mLookAndFeel(lookAndFeel)
    , mViewportWrapper(lookAndFeel)
    , mRingSpeakers(lookAndFeel)
    , mRingElevation(lookAndFeel)
    , mRingRadius(lookAndFeel)
    , mRingOffsetAngle(lookAndFeel)
    , mPolyFaces(lookAndFeel)
    , mPolyX(lookAndFeel)
    , mPolyY(lookAndFeel)
    , mPolyZ(lookAndFeel)
    , mPolyRadius(lookAndFeel)
    , mSpeakerSetupContainer(spatGrisData.appData.lastSpeakerSetup, spatGrisData.speakerSetup.speakerSetupValueTree, undoMan, [this]() { pushSelectionToMainComponent (); })
    , mFont(juce::FontOptions().withHeight(14.f))
    , undoManager (undoMan)
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
    mAddSpeakerButton.setColour(juce::TextButton::buttonColourId, lookAndFeel.mImportantColor);
    setupButton(mSaveAsSpeakerSetupButton, "Save As...");
    setupButton(mSaveSpeakerSetupButton, "Save");

    auto setupLabel = [this](juce::Label& label, juce::StringRef labelText) {

      // Sound diffusion controls
      label.setText(labelText, juce::NotificationType::dontSendNotification);
      label.setFont(mLookAndFeel.getFont());
      label.setLookAndFeel(&mLookAndFeel);
      label.setColour(juce::Label::textColourId, mLookAndFeel.getFontColour());
      mViewportWrapper.getContent()->addAndMakeVisible(label);
    };

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
            labelComboBox->comboBox.setSelectedId(4);
            labelComboBox->comboBox.setTooltip(tooltip);
            mViewportWrapper.getContent()->addAndMakeVisible(labelComboBox->comboBox);
        }
    };

    // Generate ring of speakers.
    setupLabel(mRingTitle, "Ring parameters:");
    setupWrapper(&mRingSpeakers, "# of speakers", "Number of speakers in the ring.", "8", 3, "0123456789");
    setupWrapper(&mRingElevation, "Elevation", "Elevation angle of the ring.", "0.0", 6, "-0123456789.");
    setupWrapper(&mRingRadius, "Distance", "Distance of the speakers from the center.", "1.0", 6, "0123456789.");
    setupWrapper(&mRingOffsetAngle, "Offset Angle", "Offset angle of the first speaker.", "0.0", 6, "-0123456789.");
    setupButton(mAddRingButton, "Add Ring");
    mAddRingButton.setColour(juce::TextButton::buttonColourId, lookAndFeel.mImportantColor);

    // Polyhedron of speakers.
    setupLabel(mPolyTitle, "Polyhedron parameters:");
    setupWrapper(&mPolyFaces,
                 "# of faces",
                 "Number of faces/speakers for the polyhedron.",
                 {},
                 {},
                 {},
                 { "4", "6", "8", "12", "20" });
    setupWrapper(&mPolyX,
                 "X",
                 "X position for the center of the polyhedron, in the [-1.667, 1.667] range",
                 "0.5",
                 4,
                 "-0123456789.");
    setupWrapper(&mPolyY, "Y", "Y position for the center of the polyhedron.", "0", 4, "-0123456789.");
    setupWrapper(&mPolyZ, "Z", "Z position for the center of the polyhedron.", "0.15", 4, "-0123456789.");
    setupWrapper(&mPolyRadius, "Radius", "Radius for the polyhedron.", ".05", 4, "0123456789.");
    setupButton(mAddPolyButton, "Add Polyhedron");
    mAddPolyButton.setColour(juce::TextButton::buttonColourId, lookAndFeel.mImportantColor);

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
    setupLabel(mDiffusionLabel, "Global Sound Diffusion");

    mDiffusionSlider.setTooltip("Adjuts the spreading range of sources sound");
    mDiffusionSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    mDiffusionSlider.setRotaryParameters(juce::MathConstants<float>::pi * 1.3f,
                                         juce::MathConstants<float>::pi * 2.7f,
                                         true);
    mDiffusionSlider.setRange(0.0f, 1.0f, 0.01f);
    mDiffusionSlider.setValue(spatGrisData.speakerSetup.diffusion);
    mDiffusionSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 20);
    mDiffusionSlider.setColour(juce::ToggleButton::textColourId, mLookAndFeel.getFontColour());
    mDiffusionSlider.setLookAndFeel(&mLookAndFeel);
    mDiffusionSlider.addListener(this);
    mDiffusionSlider.setEnabled(spatGrisData.project.spatMode == SpatMode::mbap
                                || spatGrisData.project.spatMode == SpatMode::hybrid);
    mViewportWrapper.getContent()->addAndMakeVisible(mDiffusionSlider);

    setDraggable (false);
    mViewportWrapper.getContent()->addAndMakeVisible(mSpeakerSetupContainer);
    mSpeakerSetupContainer.addValueTreeListener (this);

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
void EditSpeakersWindow::addSpeakerGroup(int numSpeakers, Position groupPosition, std::function<Position(int)> getSpeakerPosition)
{
    if (mMainContentComponent.getMaxSpeakerOutputPatch().get() + numSpeakers > MAX_NUM_SPEAKERS)
        return;

    auto [mainGroup, indexInMainGroup] = mSpeakerSetupContainer.getMainSpeakerGroupAndIndex ();

    // create the new speaker group and add it in the value tree
    juce::ValueTree newGroup(SPEAKER_GROUP);
    newGroup.setProperty(SPEAKER_GROUP_NAME, "new group", &undoManager);
    newGroup.setProperty(CARTESIAN_POSITION, juce::VariantConverter<Position>::toVar(groupPosition), &undoManager);
    newGroup.setProperty (UUID, juce::Uuid {}.toString (), &undoManager);
    mainGroup.addChild(newGroup, indexInMainGroup + 1, &undoManager);

    //get the output patch to copy, if it exists
    tl::optional<output_patch_t> outputPatchToCopy;
    if (spatGrisData.speakerSetup.ordering.contains (output_patch_t (indexInMainGroup)))
        outputPatchToCopy = getSpeakerOutputPatchForRow (indexInMainGroup);

    output_patch_t newOutputPatch {};
    for (int i{}; i < numSpeakers; ++i) {

        //create the speaker in main component
        newOutputPatch = mMainContentComponent.addSpeaker(outputPatchToCopy, ++indexInMainGroup);

        //add the speaker to the value tree
        auto newSpeakerVt = addNewSpeakerToVt (newOutputPatch, newGroup, i);

        newSpeakerVt.setProperty(CARTESIAN_POSITION,
                                 juce::VariantConverter<Position>::toVar(getSpeakerPosition(i)),
                                 &undoManager);
        if (auto const speakerPosition{ SpeakerData::getAbsoluteSpeakerPosition(newSpeakerVt) })
            mMainContentComponent.setSpeakerPosition(newOutputPatch, *speakerPosition);
    }

    mMainContentComponent.requestSpeakerRefresh ();

    selectSpeaker(newOutputPatch);

    mShouldComputeSpeakers = true;
}

juce::ValueTree EditSpeakersWindow::addNewSpeakerToVt(const gris::output_patch_t & newOutputPatch,
                                                      juce::ValueTree parent,
                                                      int index)
{
#if DEBUG_SPEAKER_EDITION
    DBG("EditSpeakersWindow::addNewSpeakerToVt() adding output patch " << newOutputPatch.toString() << " at index "
                                                                       << juce::String(index));
#endif

    auto const & newSpeaker = spatGrisData.speakerSetup.speakers[newOutputPatch];
    jassert(parent.isValid());

    juce::ValueTree newSpeakerVt(SPEAKER);
    newSpeakerVt.setProperty(CARTESIAN_POSITION,
                             juce::VariantConverter<Position>::toVar(newSpeaker.position),
                             &undoManager);
    newSpeakerVt.setProperty(SPEAKER_PATCH_ID, newOutputPatch.get(), &undoManager);
    newSpeakerVt.setProperty(IO_STATE, sliceStateToString(newSpeaker.state), &undoManager);
    newSpeakerVt.setProperty(GAIN, newSpeaker.gain.get(), &undoManager);
    newSpeakerVt.setProperty(DIRECT_OUT_ONLY, newSpeaker.isDirectOutOnly, &undoManager);
    newSpeakerVt.setProperty (UUID, juce::Uuid ().toString (), nullptr);

    parent.addChild(newSpeakerVt, index, &undoManager);

    return newSpeakerVt;
}

//==============================================================================
void EditSpeakersWindow::buttonClicked(juce::Button * button)
{
    JUCE_ASSERT_MESSAGE_THREAD;

    if (button == &mAddSpeakerButton) {

        if (mMainContentComponent.getMaxSpeakerOutputPatch().get() >= MAX_NUM_SPEAKERS)
            return;

        auto [mainGroup, indexInMainGroup] = mSpeakerSetupContainer.getMainSpeakerGroupAndIndex ();

        tl::optional<output_patch_t> outputPatchToCopy;
        if (spatGrisData.speakerSetup.ordering.contains (output_patch_t (indexInMainGroup)))
            outputPatchToCopy = getSpeakerOutputPatchForRow (indexInMainGroup);

        auto const newOutputPatch{ mMainContentComponent.addSpeaker(outputPatchToCopy, ++indexInMainGroup) };

        auto [curGroup, indexInCurGroup] = mSpeakerSetupContainer.getMainSpeakerGroupAndIndex();
        addNewSpeakerToVt(newOutputPatch, curGroup, ++indexInCurGroup);

    } else if (button == &mSaveAsSpeakerSetupButton) {

        mShouldComputeSpeakers = true;
        computeSpeakers();
        mMainContentComponent.saveAsEditedSpeakerSetup();

    } else if (button == &mSaveSpeakerSetupButton) {

        mShouldComputeSpeakers = true;
        computeSpeakers();
        mMainContentComponent.saveEditedSpeakerSetup ();

    } else if (button == &mAddRingButton) {
        auto const getSpeakerPosition = [this](int i) -> Position {
            const auto numSpeakers{ mRingSpeakers.getTextAs<float>() };

            // Calculate the azimuth angle by distributing speakers evenly in a circle
            // -360.0f / numSpeakers * i -> Spreads the speakers evenly around the circle
            // -mRingOffsetAngle.getText<float>() -> Applies an offset to shift the ring
            // +90.0f -> Aligns the 0-degree point with the correct reference direction
            degrees_t azimuth{ -360.0f / numSpeakers * narrow<float>(i) - mRingOffsetAngle.getTextAs<float>() + 90.0f };

            // If elevation is below the 135° elevationHalfPoint, treat it as part of the lower hemisphere (0° to 90°),
            // otherwise, assume it's in the upper hemisphere (270° to 360°).
            auto const elev{ mRingElevation.getTextAs<float>() };
            degrees_t const zenith{ elev < elevationHalfPoint ? std::clamp(elev, 0.0f, 90.0f)
                                                              : std::clamp(elev, 270.0f, 360.0f) };

            auto const radius{ mRingRadius.getTextAs<float>() };
            return Position{ PolarVector{ radians_t{ azimuth.centered() }, radians_t{ zenith }, radius } };
        };

        //for now all rings are centered at the origin
        auto const groupPosition = Position { { 0.f, 0.f, 0.f } };

        addSpeakerGroup(mRingSpeakers.getTextAs<int>(), groupPosition, getSpeakerPosition);
    } else if (button == &mAddPolyButton) {

        auto const numFaces { mPolyFaces.getSelectionAsInt () };
        jassert (numFaces == 4 || numFaces == 6 || numFaces == 8 || numFaces == 12 || numFaces == 20);

        // in vbap, the group position is always at the origin
        Position groupPosition = Position{ { 0.f, 0.f, 0.f } };
        if (spatGrisData.speakerSetup.spatMode != SpatMode::vbap) {
            groupPosition = Position{
                CartesianVector{ mPolyX.getTextAs<float>(), mPolyY.getTextAs<float>(), mPolyZ.getTextAs<float>() }
            };
        }

        auto const getSpeakerPosition = [this](int i) -> Position {
            const auto numFaces = mPolyFaces.getSelectionAsInt();
            const auto radius = mPolyRadius.getTextAs<float>();

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

                static const std::array<Vec3, 12> dodecahedron = {
                    Vec3{ -1.15217f, 0.f, 1.51342f }, { 0.57984f, 1.f, 1.51053f },   { 0.57984f, -1.f, 1.51053f },
                    { -0.93358f, -1.618f, 0.35837f }, { -1.86890f, 0.f, -0.35374f }, { -0.93358f, 1.618f, 0.35837f },
                    { 0.93358f, 1.618f, -0.35837f },  { 1.86890f, 0.f, 0.35374f },   { 0.93358f, -1.618f, -0.35837f },
                    { -0.57984f, 1.f, -1.51053f },    { 1.15217f, 0.f, -1.51342f },  { -0.57984f, -1.f, -1.51053f }
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

            return Position { CartesianVector{ x, y, z } };
        };
        isAddingGroup = true;
        addSpeakerGroup(numFaces, groupPosition, getSpeakerPosition);
        isAddingGroup = false;
    } else if (button == &mPinkNoiseToggleButton) {
        // Pink noise button
        tl::optional<dbfs_t> newPinkNoiseLevel{};
        if (mPinkNoiseToggleButton.getToggleState()) {
            newPinkNoiseLevel = dbfs_t{ static_cast<float>(mPinkNoiseGainSlider.getValue()) };
        }
        mMainContentComponent.setPinkNoiseGain(newPinkNoiseLevel);
    }
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

    const auto intValue{ textEditor.getText().getIntValue() };
    const auto floatValue{ textEditor.getText().getFloatValue() };
    const auto spatMode{ spatGrisData.speakerSetup.spatMode };

    // technically in dome/vbap mode the polyhedra is clamped right on the radius, so this calculation is moot, but
    // leaving the option here for a variable dome radius, in case it's ever useful in the future.
    const auto maxPolyRadius{ spatMode == SpatMode::mbap ? MBAP_EXTENDED_RADIUS : NORMAL_RADIUS };
    const auto clampPolyXYZ = [&textEditor, &floatValue, radius{ mPolyRadius.getTextAs<float>() }, &maxPolyRadius]() {
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
    } else if (&textEditor == &mPolyX.editor) {
        clampPolyXYZ();
    } else if (&textEditor == &mPolyY.editor) {
        clampPolyXYZ();
    } else if (&textEditor == &mPolyZ.editor) {
        clampPolyXYZ();
    } else if (&textEditor == &mPolyRadius.editor) {
        const auto x{ mPolyX.getTextAs<float>() };
        const auto y{ mPolyY.getTextAs<float>() };
        const auto z{ mPolyZ.getTextAs<float>() };

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
    const auto showExtendedPolyWidgets = spatGrisData.speakerSetup.spatMode != SpatMode::vbap;
    mPolyX.setVisible(showExtendedPolyWidgets);
    mPolyY.setVisible(showExtendedPolyWidgets);
    mPolyZ.setVisible(showExtendedPolyWidgets);
    mPolyRadius.setVisible(showExtendedPolyWidgets);
}


//==============================================================================
void EditSpeakersWindow::updateWinContent()
{
    JUCE_ASSERT_MESSAGE_THREAD;

    mSpeakerSetupContainer.reload (spatGrisData.speakerSetup.speakerSetupValueTree);
    mSpeakerSetupContainer.setSpatMode (spatGrisData.speakerSetup.spatMode);

    mDiffusionSlider.setValue(spatGrisData.speakerSetup.diffusion);
    mDiffusionSlider.setEnabled(spatGrisData.project.spatMode == SpatMode::mbap
                                || spatGrisData.project.spatMode == SpatMode::hybrid);

    togglePolyhedraExtraWidgets();
}

//==============================================================================
void EditSpeakersWindow::pushSelectionToMainComponent()
{
    JUCE_ASSERT_MESSAGE_THREAD;

    mMainContentComponent.setSelectedSpeakers(mSpeakerSetupContainer.getSelectedSpeakers());
}

//==============================================================================
void EditSpeakersWindow::closeButtonPressed()
{
    JUCE_ASSERT_MESSAGE_THREAD;

    mMainContentComponent.setPinkNoiseGain(tl::nullopt);
    mMainContentComponent.closeSpeakersConfigurationWindow();
}

//==============================================================================
bool EditSpeakersWindow::keyPressed(const juce::KeyPress & key)
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

    // viewport
    auto bounds{ getLocalBounds() };
    mViewportWrapper.setBounds(bounds);
    mViewportWrapper.correctSize(getWidth() - 10, getHeight() - 30);

    // speaker table
    auto const rowH{ 24 };
    auto const bottomPanelH{ 195 + rowH };
    mSpeakerSetupContainer.setSize(getWidth(), getHeight() - bottomPanelH);

    // first row of bottom panel with add speaker
    auto const rowsStart = getHeight() - 200;
    auto const rowSpacing{ 10 };
    auto const firstRowY{ rowsStart };
    mAddSpeakerButton.setBounds(getWidth() - 105, firstRowY, 100, rowH);

    // second row of bottom panel with rings of speakers
    auto const secondRowY{ rowsStart + rowH + rowSpacing };
    auto const positionWidget = [](LabelWrapper * w, int x, int y, int lw, int ew) {
        // the fine adjustment (+1 y on the label position and +1 -2 on
        // the editor's y and height are there to match the size of the
        // comboboxes and the positionning with the one of the buttons.
        w->label.setBounds(x, y + 1, lw, rowH);
        if (auto * lew{ dynamic_cast<LabelTextEditorWrapper *>(w) })
            lew->editor.setBounds(x + lw, y + 1, ew, rowH - 2);
        else if (auto * lcw{ dynamic_cast<LabelComboBoxWrapper *>(w) })
            lcw->comboBox.setBounds(x + lw, y + 1, ew, rowH - 2);
    };
    auto currentX = 130;
    auto const labelW{ 80 };
    auto const shortLabelW{ 33 };

    auto const editorW{ 60 };
    auto const increment{ 140 };

    mRingTitle.setBounds(5, secondRowY, currentX, rowH);
    positionWidget(&mRingSpeakers, currentX, secondRowY, labelW, editorW);
    currentX += increment;
    positionWidget(&mRingElevation, currentX, secondRowY, labelW, editorW);
    currentX += increment;
    positionWidget(&mRingRadius, currentX, secondRowY, labelW, editorW);
    currentX += increment;
    positionWidget(&mRingOffsetAngle, currentX, secondRowY, labelW, editorW);
    currentX += increment;
    mAddRingButton.setBounds(getWidth() - 105, secondRowY, 100, rowH);

    // third row of bottom panel with polyhedra controls
    auto const thirdRowY{ rowsStart + (rowH + rowSpacing) * 2 };
    currentX = 130 ;
    mPolyTitle.setBounds(5, thirdRowY, currentX, rowH);
    positionWidget(&mPolyFaces, currentX, thirdRowY, labelW, editorW);
    currentX += labelW + editorW;
    positionWidget(&mPolyX, currentX, thirdRowY, shortLabelW, editorW);
    currentX += shortLabelW + editorW;
    positionWidget(&mPolyY, currentX, thirdRowY, shortLabelW, editorW);
    currentX += shortLabelW + editorW + 1;
    positionWidget(&mPolyZ, currentX, thirdRowY, shortLabelW, editorW);
    // +2 just to align the editors a bit better.
    currentX += shortLabelW + editorW;
    positionWidget(&mPolyRadius, currentX, thirdRowY, labelW, editorW);
    mAddPolyButton.setBounds(getWidth() - 105, thirdRowY, 100, rowH);

    // "Fourth" row with pink noise, diffusion and the save buttons.
    // This row needs to be placed a bit lower due to the height of the knobs.
    auto const sliderHeight{ 60 };
    auto const fourthRowY{ rowsStart + (rowH + rowSpacing) * 3 + sliderHeight};
    mPinkNoiseToggleButton.setBounds(5, getHeight() - 70, 150, rowH);
    mPinkNoiseGainSlider.setBounds(170, getHeight() - 95, 60, sliderHeight);
    mDiffusionLabel.setBounds(260, getHeight() - 70, 160, rowH);
    mDiffusionSlider.setBounds(260+165, getHeight() - 95, 60, sliderHeight);
    mPinkNoiseToggleButton.setBounds(5, getHeight() - 70, 150, rowH);

    // save buttons. the getHeight() - 57 are there to match the position of the buttons
    // to the middle of the two ToggleButtons on the same line.
    mSaveAsSpeakerSetupButton.setBounds(getWidth() - 210, getHeight() - 57, 100, rowH);
    mSaveSpeakerSetupButton.setBounds(getWidth() - 105, getHeight() - 57, 100, rowH);

}

//==============================================================================
SpeakerData const & EditSpeakersWindow::getSpeakerData(int const rowNum) const
{
    JUCE_ASSERT_MESSAGE_THREAD;

    auto const outputPatch{ getSpeakerOutputPatchForRow(rowNum) };
    return spatGrisData.speakerSetup.speakers[outputPatch];
}

//==============================================================================
output_patch_t EditSpeakersWindow::getSpeakerOutputPatchForRow(int const row) const
{
    JUCE_ASSERT_MESSAGE_THREAD;

    auto const & data{ spatGrisData };
    jassert(row >= 0 && row < data.speakerSetup.ordering.size());
    auto const result{ data.speakerSetup.ordering[row] };
    jassert(data.speakerSetup.speakers.contains(result));
    return result;
}

//==============================================================================
void EditSpeakersWindow::computeSpeakers()
{
    if (mShouldComputeSpeakers) {
        mMainContentComponent.requestSpeakerRefresh ();
        mShouldComputeSpeakers = false;
    }
}

void EditSpeakersWindow::valueTreePropertyChanged(juce::ValueTree & vt, const juce::Identifier & property)
{
    auto const newVal{ vt[property] };

    if (vt.getType() == SPEAKER_GROUP) {
        if (property == CARTESIAN_POSITION || property == YAW || property == PITCH || property == ROLL) {
            for (auto speakerVt : vt) {
                jassert(speakerVt.getType() == SPEAKER);
                output_patch_t const outputPatch{ speakerVt.getProperty(SPEAKER_PATCH_ID) };
                if (auto const speakerPosition{ SpeakerData::getAbsoluteSpeakerPosition(speakerVt) })
                    mMainContentComponent.setSpeakerPosition(outputPatch, *speakerPosition);
            }

            mShouldComputeSpeakers = true;
        }
        else if (property != SPEAKER_GROUP_NAME){
            // unhandled property
            jassertfalse;
        }
    } else if (vt.getType() == SPEAKER) {
        output_patch_t const outputPatch{ vt[SPEAKER_PATCH_ID] };

        if (property == CARTESIAN_POSITION) {
            if (auto const speakerPosition{ SpeakerData::getAbsoluteSpeakerPosition(vt) }) {
                mMainContentComponent.setSpeakerPosition(outputPatch, *speakerPosition);
                mShouldComputeSpeakers = true;
            }
        } else if (property == DIRECT_OUT_ONLY) {
            auto const directOutOnly{ vt[DIRECT_OUT_ONLY] };
            mMainContentComponent.setShowTriplets(false);
            mMainContentComponent.speakerDirectOutOnlyChanged(outputPatch, directOutOnly);
        } else if (property == GAIN) {
            mMainContentComponent.setSpeakerGain(outputPatch, dbfs_t{ newVal });
        } else if (property == HIGHPASS_FREQ) {
            mMainContentComponent.setSpeakerHighPassFreq(outputPatch, hz_t{ newVal });
        } else if (property == NEXT_SPEAKER_PATCH_ID && !undoManager.isPerformingUndoRedo()) {
            // we don't want to do anything in here when undoing/redoing
            if (vt.hasProperty(NEXT_SPEAKER_PATCH_ID)) {
                mMainContentComponent.setShowTriplets(false);

                // first make sure we have a change in patch numbers
                auto const & oldOutputPatch{ outputPatch };
                output_patch_t newOutputPatch{ vt[NEXT_SPEAKER_PATCH_ID] };
                if (newOutputPatch != oldOutputPatch) {
                    // check that the new output patch isn't in use
                    if (spatGrisData.speakerSetup.speakers.contains(newOutputPatch)) {
                        // it is in use, so show an error
                        juce::AlertWindow alert("Wrong output patch!    ",
                                                "Sorry! Output patch number " + juce::String(newOutputPatch.get())
                                                    + " is already used.",
                                                juce::AlertWindow::WarningIcon);
                        alert.setLookAndFeel(&mLookAndFeel);
                        alert.addButton("OK", 0, juce::KeyPress(juce::KeyPress::returnKey));
                        alert.runModalLoop();

                        // and revert to the previous patch value, forcing the change message to be sent
                        vt.setProperty(SPEAKER_PATCH_ID, oldOutputPatch.get(), nullptr);
                        vt.sendPropertyChangeMessage(SPEAKER_PATCH_ID);

                    } else {
                        // we're all good so go ahead with the change
                        mMainContentComponent.speakerOutputPatchChanged(oldOutputPatch, newOutputPatch);
                        vt.setProperty(SPEAKER_PATCH_ID, newOutputPatch.get(), nullptr);
                        mShouldComputeSpeakers = true;
                    }
                }

                vt.removeProperty(NEXT_SPEAKER_PATCH_ID, nullptr);
            }
        } else if (property != SPEAKER_PATCH_ID && property != NEXT_SPEAKER_PATCH_ID) {
            // unhandled property
            jassertfalse;
        }
    } else if (property != SPAT_MODE) {
        // unhandled property
        jassertfalse;
    }
}

void EditSpeakersWindow::valueTreeChildAdded(juce::ValueTree & parent, juce::ValueTree & child)
{
    auto const childType{ child.getType() };
    auto const index { parent.indexOf (child) };
    const auto childOutputPatch = output_patch_t (child[SPEAKER_PATCH_ID]);

#if DEBUG_SPEAKER_EDITION
    if (childType == SPEAKER_GROUP)
        DBG ("EditSpeakersWindow::valueTreeChildAdded() called for SPEAKER_GROUP_NAME: " << child[SPEAKER_GROUP_NAME].toString() << " and index " << juce::String (index));
    else if (childType == SPEAKER)
        DBG ("EditSpeakersWindow::valueTreeChildAdded() called for SPEAKER_PATCH_ID: " << child[SPEAKER_PATCH_ID].toString () << " and index " << juce::String (index));
#endif

    if (childType == SPEAKER) {
        int indexAdjustment{ 0 };
        if (parent[SPEAKER_GROUP_NAME] != MAIN_SPEAKER_GROUP_NAME) {
            auto const mainGroup = parent.getParent();
            // we only support one level of grouping for now
            jassert(mainGroup[SPEAKER_GROUP_NAME] == MAIN_SPEAKER_GROUP_NAME);
            indexAdjustment = mainGroup.indexOf(parent);
        }
        mMainContentComponent.addSpeaker(*SpeakerData::fromVt(child), index + indexAdjustment, childOutputPatch);
    }

    if (!isAddingGroup) {
        mMainContentComponent.requestSpeakerRefresh ();
        mShouldComputeSpeakers = true;
    }
}

void EditSpeakersWindow::valueTreeChildRemoved(juce::ValueTree & /*parent*/, juce::ValueTree & child, [[maybe_unused]] int index)
{

    auto const childType { child.getType () };

#if DEBUG_SPEAKER_EDITION
    if (childType == SPEAKER_GROUP)
        DBG ("EditSpeakersWindow::valueTreeChildRemoved() called for SPEAKER_GROUP_NAME" << child[SPEAKER_GROUP_NAME].toString () << " and index " << juce::String (index));
    else if (childType == SPEAKER)
        DBG ("EditSpeakersWindow::valueTreeChildRemoved() called for SPEAKER_PATCH_ID" << child[SPEAKER_PATCH_ID].toString () << " and index " << juce::String (index));
#endif

    if (childType == SPEAKER) {
        output_patch_t outputPatch {child[SPEAKER_PATCH_ID]};
        mMainContentComponent.removeSpeaker (outputPatch, ! mSpeakerSetupContainer.isDeletingGroup());
    }
}

} // namespace gris
