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

#pragma once

#include "Data/sg_LogicStrucs.hpp"
#include "tl/optional.hpp"
#include "sg_Box.hpp"
#include "UI/Windows/SpeakerSetup/SpeakerSetupContainer.hpp"

#define DEBUG_SPEAKER_EDITION 0

namespace gris
{
class MainContentComponent;
class GrisLookAndFeel;

/** This is used to snap the elevation when calculating ring positions. 135.f is short-hand for the mid point
    between 90° (max elevation going up) and 270° (max elevation going down): 90 + (360 − 270)/2 = 135.
*/
static auto constexpr elevationHalfPoint{ 135.f };

//==============================================================================

// Helper struct to trigger static_assert for unsupported types
template<typename T>
struct always_false : std::false_type {
};

struct LabelWrapper {
    explicit LabelWrapper(GrisLookAndFeel & lookAndFeel);
    virtual ~LabelWrapper() { label.setLookAndFeel(nullptr); }
    virtual void setVisible(bool visible) { label.setVisible(visible); }

    juce::Label label;
};

//==============================================================================

struct LabelTextEditorWrapper : public LabelWrapper
{
    explicit LabelTextEditorWrapper(GrisLookAndFeel & lookAndFeel);
    ~LabelTextEditorWrapper() { editor.setLookAndFeel(nullptr); }

    template<typename T>
    T getTextAs()
    {
        auto const text{ editor.getText() };

        if constexpr (std::is_same_v<T, juce::String>)
            return text;
        else if constexpr (std::is_same_v<T, int>)
            return text.getIntValue();
        else if constexpr (std::is_same_v<T, float>)
            return text.getFloatValue();
        else if constexpr (std::is_same_v<T, double>)
            return text.getDoubleValue();
        else
            static_assert(always_false<T>::value, "Unsupported type");

        return {};
    }

    void setVisible(bool visible) override
    {
        LabelWrapper::setVisible(visible);
        editor.setVisible(visible);
    }

    juce::TextEditor editor;
};

//==============================================================================

struct LabelComboBoxWrapper : public LabelWrapper
{
    explicit LabelComboBoxWrapper(GrisLookAndFeel & lookAndFeel);

    juce::String getSelectionAsString(){ return comboBox.getText(); }
    int getSelectionAsInt(){ return comboBox.getText().getIntValue(); }
    void setVisible(bool visible) override
    {
        LabelWrapper::setVisible(visible);
        comboBox.setVisible(visible);
    }

    juce::ComboBox comboBox{};
};

//==============================================================================

/**
 * @class gris::EditSpeakersWindow
 * @brief A window for editing speaker setups.
 *
 * This class provides a user interface for adding, removing, and configuring speakers and speaker groups.
 * It supports direct manipulation of speaker parameters, group operations (such as adding rings or polyhedra of
 * speakers), and integrates with the application's undo/redo system. The window interacts with the main content
 * component and updates the speaker setup in real time. It also provides controls for pink noise testing and diffusion
 * settings.
 *
 * @see gris::MainContentComponent, gris::SpeakerSetupContainer
 */
class EditSpeakersWindow final
    : public juce::DocumentWindow
    , public juce::ValueTree::Listener
    , public juce::ToggleButton::Listener
    , public juce::TextEditor::Listener
    , public juce::Slider::Listener
{
private:
    MainContentComponent & mMainContentComponent;
    const SpatGrisData& spatGrisData;
    GrisLookAndFeel & mLookAndFeel;

    Box mViewportWrapper;

    juce::TextButton mAddSpeakerButton;
    juce::TextButton mSaveAsSpeakerSetupButton;
    juce::TextButton mSaveSpeakerSetupButton;

    //add ring of speakers
    juce::Label mRingTitle;
    LabelTextEditorWrapper mRingSpeakers;
    LabelTextEditorWrapper mRingElevation;
    LabelTextEditorWrapper mRingRadius;
    LabelTextEditorWrapper mRingOffsetAngle;
    juce::TextButton mAddRingButton;

    // add polyhedron of speakers
    juce::Label mPolyTitle;
    LabelComboBoxWrapper mPolyFaces;
    LabelTextEditorWrapper mPolyX;
    LabelTextEditorWrapper mPolyY;
    LabelTextEditorWrapper mPolyZ;
    LabelTextEditorWrapper mPolyRadius;
    juce::TextButton mAddPolyButton;

    //add grid of speakers
    juce::Label mGridTitle;
    LabelComboBoxWrapper mGridAlignment;
    LabelTextEditorWrapper mGridNumCols;
    LabelTextEditorWrapper mGridNumRows;
    LabelTextEditorWrapper mGridX;
    LabelTextEditorWrapper mGridY;
    LabelTextEditorWrapper mGridZ;
    LabelTextEditorWrapper mGridWidth;
    LabelTextEditorWrapper mGridHeight;
    juce::TextButton mAddGridButton;
    void clampGridXYZ (juce::TextEditor& textEditor);

    juce::ToggleButton mPinkNoiseToggleButton;
    juce::Slider mPinkNoiseGainSlider;

    juce::Label mDiffusionLabel;
    juce::Slider mDiffusionSlider;

    SpeakerSetupContainer mSpeakerSetupContainer;

    juce::Font mFont;

    tl::optional<int> mDragStartY{};
    bool mShouldComputeSpeakers{};
    juce::SparseSet<int> mLastSelectedRows{};

public:
    //==============================================================================
    EditSpeakersWindow(juce::String const & name,
                       GrisLookAndFeel & lookAndFeel,
                       MainContentComponent & mainContentComponent,
                       juce::UndoManager& undoMan);
    //==============================================================================
    EditSpeakersWindow() = delete;
    SG_DELETE_COPY_AND_MOVE(EditSpeakersWindow)
    //==============================================================================

    void selectSpeaker(tl::optional<output_patch_t> outputPatch) { mSpeakerSetupContainer.selectSpeaker(outputPatch); }

    void togglePolyhedraExtraWidgets();
    void toggleGridWidgets();

    /** This is called in a variety of places, including in MainContentComponent::refreshSpeakers()
    *   when the spatMode changes.*/
    void updateWinContent();

    /**
     * Returns the speaker ordering exactly as it is displayed in the window.
     */
    juce::Array<output_patch_t> getSpeakerOutputPatchOrder();

private:
    bool isAddingGroup = false;
    //==============================================================================
    void pushSelectionToMainComponent();

    SpeakerData const & getSpeakerData(int rowNum) const;
    [[nodiscard]] output_patch_t getSpeakerOutputPatchForRow(int row) const;
    void computeSpeakers();
    void addSpeakerGroup(int numSpeakers, Position groupPosition, std::function<Position(int)> getSpeakerPosition);
    juce::ValueTree addNewSpeakerToVt (const gris::output_patch_t& newOutputPatch, juce::ValueTree parent, int index);

    //==============================================================================
    // VIRTUALS
    void buttonClicked(juce::Button * button) override;
    void textEditorReturnKeyPressed(juce::TextEditor & textEditor) override;
    void textEditorFocusLost(juce::TextEditor &) override;
    void closeButtonPressed() override;
    bool keyPressed (const juce::KeyPress &key) override;
    void resized () override;
    void sliderValueChanged (juce::Slider* slider) override;
    void valueTreePropertyChanged(juce::ValueTree & vt, const juce::Identifier & property) override;
    void valueTreeChildAdded(juce::ValueTree & parent, juce::ValueTree & child) override;
    void valueTreeChildRemoved(juce::ValueTree & parent, juce::ValueTree & child, int idInParent) override;

    juce::UndoManager& undoManager;
    //==============================================================================
    JUCE_LEAK_DETECTOR(EditSpeakersWindow)
};

} // namespace gris
