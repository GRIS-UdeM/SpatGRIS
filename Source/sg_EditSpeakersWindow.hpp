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

#include "AlgoGRIS/Data/sg_LogicStrucs.hpp"
#include "AlgoGRIS/tl/optional.hpp"
#include "sg_Box.hpp"
#include "UI/Windows/SpeakerSetup/SpeakerSetupContainer.hpp"

#define USE_OLD_SPEAKER_SETUP_VIEW 0

namespace gris
{
class EditableTextCustomComponent;
class MainContentComponent;
class GrisLookAndFeel;

/** used to snap the elevation when calculating ring positions. 135.f is short-hand for the mid point
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

    int getSelectionAsInt(){ return comboBox.getText().getIntValue(); }
    void setVisible(bool visible) override
    {
        LabelWrapper::setVisible(visible);
        comboBox.setVisible(visible);
    }

    juce::ComboBox comboBox{};
};

//==============================================================================
class EditSpeakersWindow final
    : public juce::DocumentWindow
#if USE_OLD_SPEAKER_SETUP_VIEW
    , public juce::TableListBoxModel
#else
    , public juce::ValueTree::Listener
#endif
    , public juce::ToggleButton::Listener
    , public juce::TextEditor::Listener
    , public juce::Slider::Listener
{
#if USE_OLD_SPEAKER_SETUP_VIEW
    static constexpr auto DIRECT_OUT_BUTTON_ID_OFFSET = 1000;
#endif

public:
    struct Cols {
        static constexpr int DRAG_HANDLE = 1;
        static constexpr int OUTPUT_PATCH = 2;
        static constexpr int X = 3;
        static constexpr int Y = 4;
        static constexpr int Z = 5;
        static constexpr int AZIMUTH = 6;
        static constexpr int ELEVATION = 7;
        static constexpr int DISTANCE = 8;
        static constexpr int GAIN = 9;
        static constexpr int HIGHPASS = 10;
        static constexpr int DIRECT_TOGGLE = 11;
        static constexpr int DELETE_BUTTON = 12;
    };

#if ! USE_OLD_SPEAKER_SETUP_VIEW
    void saveSpeakerSetup () { mSpeakerSetupContainer.saveSpeakerSetup(); }
#endif

private:
    static auto constexpr MIN_COL_WIDTH = 50;
    static auto constexpr MAX_COL_WIDTH = 120;
    static auto constexpr DEFAULT_COL_WIDTH = 70;

    MainContentComponent & mMainContentComponent;
    const SpatGrisData& spatGrisData;
    GrisLookAndFeel & mLookAndFeel;

    Box mViewportWrapper;

    juce::TextButton mAddSpeakerButton;
    juce::TextButton mSaveAsSpeakerSetupButton;
    juce::TextButton mSaveSpeakerSetupButton;

    //add ring of speakers
    LabelTextEditorWrapper mRingSpeakers;
    LabelTextEditorWrapper mRingElevation;
    LabelTextEditorWrapper mRingRadius;
    LabelTextEditorWrapper mRingOffsetAngle;
    juce::TextButton mAddRingButton;

    // add polyhedron of speakers
    LabelComboBoxWrapper mPolyFaces;
    LabelTextEditorWrapper mPolyX;
    LabelTextEditorWrapper mPolyY;
    LabelTextEditorWrapper mPolyZ;
    LabelTextEditorWrapper mPolyRadius;
    LabelTextEditorWrapper mPolyAzimuthOffset;
    LabelTextEditorWrapper mPolyElevOffset;

    juce::TextButton mAddPolyButton;

    juce::ToggleButton mPinkNoiseToggleButton;
    juce::Slider mPinkNoiseGainSlider;

    juce::Label mDiffusionLabel;
    juce::Slider mDiffusionSlider;

#if USE_OLD_SPEAKER_SETUP_VIEW
    juce::TableListBox mSpeakersTableListBox;
#else
    SpeakerSetupContainer mSpeakerSetupContainer;
#endif
    juce::Font mFont;

    int mNumRows{};
    tl::optional<int> mDragStartY{};
    bool mShouldComputeSpeakers{};
    juce::SparseSet<int> mLastSelectedRows{};
    //==============================================================================
    friend EditableTextCustomComponent;

public:
    //==============================================================================
    EditSpeakersWindow(juce::String const & name,
                       GrisLookAndFeel & lookAndFeel,
                       MainContentComponent & mainContentComponent,
                       juce::UndoManager& undoMan);
    //==============================================================================
    EditSpeakersWindow() = delete;
    ~EditSpeakersWindow() override;
    SG_DELETE_COPY_AND_MOVE(EditSpeakersWindow)
    //==============================================================================
#if USE_OLD_SPEAKER_SETUP_VIEW
    void initComp();
    void selectRow(tl::optional<int> value);
#endif
    void selectSpeaker(tl::optional<output_patch_t> outputPatch);

    void togglePolyhedraExtraWidgets();

    /** This is called in a variety of places, including in MainContentComponent::refreshSpeakers()
    *   when the spatMode changes.*/
    void updateWinContent();

private:
    bool isAddingGroup = false;
    //==============================================================================
    void pushSelectionToMainComponent();
#if USE_OLD_SPEAKER_SETUP_VIEW
    [[nodiscard]] juce::String getText(int columnNumber, int rowNumber) const;
    void setText(int columnNumber, int rowNumber, juce::String const & newText, bool altDown = false);
#endif
    bool isMouseOverDragHandle(juce::MouseEvent const & event);
    SpeakerData const & getSpeakerData(int rowNum) const;
    [[nodiscard]] output_patch_t getSpeakerOutputPatchForRow(int row) const;
    void computeSpeakers();
    void addSpeakerGroup(int numSpeakers, Position groupPosition, std::function<Position(int)> getSpeakerPosition);
    juce::ValueTree addNewSpeakerToVt (const gris::output_patch_t& newOutputPatch, juce::ValueTree parent, tl::optional<int> index);
    //==============================================================================
    // VIRTUALS
    void buttonClicked(juce::Button * button) override;
    void textEditorReturnKeyPressed(juce::TextEditor & textEditor) override;
    void textEditorFocusLost(juce::TextEditor &) override;
    void closeButtonPressed() override;
    bool keyPressed (const juce::KeyPress &key) override;
    void resized () override;
    void sliderValueChanged (juce::Slider* slider) override;
#if USE_OLD_SPEAKER_SETUP_VIEW
    [[nodiscard]] int getNumRows () override { return this->mNumRows; }
    void sortOrderChanged(int newSortColumnId, bool isForwards) override;
    
    void paintRowBackground(juce::Graphics & g, int rowNumber, int width, int height, bool rowIsSelected) override;
    void paintCell(juce::Graphics &, int, int, int , int, bool ) override {}
    [[nodiscard]] Component * refreshComponentForCell(int rowNumber,
                                                      int columnId,
                                                      bool isRowSelected,
                                                      Component * existingComponentToUpdate) override;
    void mouseDown(juce::MouseEvent const & event) override;

    void mouseDrag(juce::MouseEvent const & event) override;
    void mouseUp(juce::MouseEvent const & event) override;
#else
    void valueTreePropertyChanged(juce::ValueTree & vt, const juce::Identifier & property) override;
    void valueTreeChildAdded(juce::ValueTree & parent, juce::ValueTree & child) override;
    void valueTreeChildRemoved(juce::ValueTree & parent, juce::ValueTree & child, int idInParent) override;
    //void valueTreeChildOrderChanged(juce::ValueTree & parent, int oldChildId, int newChildId) override;
    //void valueTreeParentChanged(juce::ValueTree & childWithNewParent) override;
    //void valueTreeRedirected(juce::ValueTree &) override { jassertfalse; }

    juce::UndoManager& undoManager;
#endif
    //==============================================================================
    JUCE_LEAK_DETECTOR(EditSpeakersWindow)
};

} // namespace gris
