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

#pragma once

#include "LogicStrucs.hpp"
#include "lib/tl/optional.hpp"

#include "macros.h"
DISABLE_WARNINGS
#include <JuceHeader.h>
ENABLE_WARNINGS

#include "AudioProcessor.h"
#include "Box.h"

class EditableTextCustomComponent;
class MainContentComponent;
class GrisLookAndFeel;

//==============================================================================
class EditSpeakersWindow final
    : public juce::DocumentWindow
    , public juce::TableListBoxModel
    , public juce::ToggleButton::Listener
    , public juce::TextEditor::Listener
    , public juce::Slider::Listener
{
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

private:
    static auto constexpr MIN_COL_WIDTH = 50;
    static auto constexpr MAX_COL_WIDTH = 120;
    static auto constexpr DEFAULT_COL_WIDTH = 70;

    MainContentComponent & mMainContentComponent;
    GrisLookAndFeel & mLookAndFeel;

    Box mListSpeakerBox;

    juce::TextButton mAddSpeakerButton;
    juce::TextButton mCompSpeakersButton;

    juce::Label mNumOfSpeakersLabel;
    juce::TextEditor mNumOfSpeakersTextEditor;
    juce::Label mZenithLabel;
    juce::TextEditor mZenithTextEditor;
    juce::Label mRadiusLabel;
    juce::TextEditor mRadiusTextEditor;
    juce::Label mOffsetAngleLabel;
    juce::TextEditor mOffsetAngleTextEditor;
    juce::TextButton mAddRingButton;

    juce::ToggleButton mPinkNoiseToggleButton;
    juce::Slider mPinkNoiseGainSlider;

    juce::TableListBox mSpeakersTableListBox;
    juce::Font mFont;

    int mNumRows{};
    tl::optional<int> mDragStartY{};
    //==============================================================================
    friend EditableTextCustomComponent;

public:
    //==============================================================================
    EditSpeakersWindow(juce::String const & name,
                       GrisLookAndFeel & lookAndFeel,
                       MainContentComponent & mainContentComponent,
                       juce::String const & configName);
    //==============================================================================
    EditSpeakersWindow() = delete;
    ~EditSpeakersWindow() override = default;
    //==============================================================================
    EditSpeakersWindow(EditSpeakersWindow const &) = delete;
    EditSpeakersWindow(EditSpeakersWindow &&) = delete;
    EditSpeakersWindow & operator=(EditSpeakersWindow const &) = delete;
    EditSpeakersWindow & operator=(EditSpeakersWindow &&) = delete;
    //==============================================================================
    void initComp();
    void selectRow(tl::optional<int> const value);
    void selectSpeaker(tl::optional<output_patch_t> outputPatch);
    void updateWinContent(bool needToSaveSpeakerSetup);

private:
    //==============================================================================
    [[nodiscard]] SpatMode getModeSelected() const;
    [[nodiscard]] juce::String getText(int columnNumber, int rowNumber) const;
    void setText(int columnNumber, int rowNumber, juce::String const & newText, bool altDown = false);
    bool isMouseOverDragHandle(juce::MouseEvent const & event);
    SpeakerData const & getSpeakerData(int rowNum) const;
    [[nodiscard]] output_patch_t getSpeakerOutputPatchForRow(int row) const;
    //==============================================================================
    // VIRTUALS
    [[nodiscard]] int getNumRows() override { return this->mNumRows; }
    void buttonClicked(juce::Button * button) override;
    void textEditorTextChanged(juce::TextEditor & editor) override;
    void textEditorReturnKeyPressed(juce::TextEditor & textEditor) override;
    void closeButtonPressed() override;
    void sliderValueChanged(juce::Slider * slider) override;
    void sortOrderChanged(int newSortColumnId, bool isForwards) override;
    void resized() override;
    void paintRowBackground(juce::Graphics & g, int rowNumber, int width, int height, bool rowIsSelected) override;
    void paintCell(juce::Graphics & g, int rowNumber, int columnId, int width, int height, bool rowIsSelected) override;
    [[nodiscard]] Component * refreshComponentForCell(int rowNumber,
                                                      int columnId,
                                                      bool isRowSelected,
                                                      Component * existingComponentToUpdate) override;
    void mouseDown(juce::MouseEvent const & event) override;
    void mouseDrag(juce::MouseEvent const & event) override;
    //==============================================================================
    JUCE_LEAK_DETECTOR(EditSpeakersWindow)
};
