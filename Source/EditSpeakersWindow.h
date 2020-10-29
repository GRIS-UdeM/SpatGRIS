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

#pragma once

#include "macros.h"

DISABLE_WARNINGS
#include <JuceHeader.h>
ENABLE_WARNINGS

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
    friend EditableTextCustomComponent; // TODO: temporary solution whiling refactoring is going on...
    //==============================================================================
    // DEFAULTS
    EditSpeakersWindow(juce::String const & name,
                       GrisLookAndFeel & lookAndFeel,
                       MainContentComponent & mainContentComponent,
                       juce::String const & configName);
    ~EditSpeakersWindow() final = default;

    void initComp();
    void selectedRow(int value);
    void updateWinContent();

private:
    //==============================================================================
    int getModeSelected() const;
    bool getDirectOutForSpeakerRow(int row) const;
    juce::String getText(int columnNumber, int rowNumber) const;

    void setText(int columnNumber, int rowNumber, String const & newText, bool altDown = false);
    //==============================================================================
    // VIRTUALS
    int getNumRows() final { return this->numRows; }

    void buttonClicked(Button * button) final;
    void textEditorTextChanged(juce::TextEditor & editor) final;
    void textEditorReturnKeyPressed(juce::TextEditor & textEditor) final;
    void closeButtonPressed() final;
    void sliderValueChanged(juce::Slider * slider) final;
    void sortOrderChanged(int newSortColumnId, bool isForwards) final;
    void resized() final;
    void paintRowBackground(Graphics & g, int rowNumber, int width, int height, bool rowIsSelected) final;
    void paintCell(Graphics & g, int rowNumber, int columnId, int width, int height, bool rowIsSelected) final;

    Component * refreshComponentForCell(int rowNumber,
                                        int columnId,
                                        bool isRowSelected,
                                        Component * existingComponentToUpdate) final;

private:
    //==============================================================================
    MainContentComponent & mainContentComponent;
    GrisLookAndFeel & grisFeel;

    Box boxListSpeaker;

    juce::TextButton butAddSpeaker;
    juce::TextButton butcompSpeakers;

    juce::Label rNumOfSpeakersLabel;
    juce::TextEditor rNumOfSpeakers;
    juce::Label rZenithLabel;
    juce::TextEditor rZenith;
    juce::Label rRadiusLabel;
    juce::TextEditor rRadius;
    juce::Label rOffsetAngleLabel;
    juce::TextEditor rOffsetAngle;
    juce::TextButton butAddRing;

    juce::ToggleButton pinkNoise;
    juce::Slider pinkNoiseGain;

    juce::TableListBox tableListSpeakers;
    juce::Font font;

    int numRows;
    bool initialized{ false };
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EditSpeakersWindow);
};
