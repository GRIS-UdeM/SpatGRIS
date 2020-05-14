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

#ifndef EDITSPEAKERSWINDOW_H
#define EDITSPEAKERSWINDOW_H

#include "../JuceLibraryCode/JuceHeader.h"

class Box;
class EditableTextCustomComponent;
class MainContentComponent;
class GrisLookAndFeel;

//==============================================================================
class EditSpeakersWindow : public juce::DocumentWindow,
                           public juce::TableListBoxModel,
                           public juce::ToggleButton::Listener,
                           public juce::TextEditor::Listener,
                           public juce::Slider::Listener
{
public:
    friend EditableTextCustomComponent; // TODO: temporary solution whiling refactoring is going on...
    //==============================================================================
    // DEFAULTS
    EditSpeakersWindow(juce::String const& name, GrisLookAndFeel& lookAndFeel, MainContentComponent& mainContentComponent, juce::String const& configName);
    ~EditSpeakersWindow() = default;
    
    void initComp();
    void selectedRow(int value);
    void updateWinContent();
    //==============================================================================
    // VIRTUALS
private:
    //==============================================================================
    int getModeSelected() const;
    bool getDirectOutForSpeakerRow(int row) const;
    String getText(int columnNumber, int rowNumber) const;

    void setText(int columnNumber, int rowNumber, String const& newText, bool altDown = false);
    //==============================================================================
    // VIRTUALS
    int getNumRows() override { return this->numRows; };
    void buttonClicked(Button *button) override;
    void textEditorTextChanged(juce::TextEditor& editor) override;
    void textEditorReturnKeyPressed(juce::TextEditor& textEditor) override;
    void closeButtonPressed() override;
    void sliderValueChanged (juce::Slider* slider) override;
    void sortOrderChanged(int newSortColumnId, bool isForwards) override;
    void resized() override;
    void paintRowBackground(Graphics& g, int rowNumber, int width, int height, bool rowIsSelected) override;
    void paintCell(Graphics& g, int rowNumber, int columnId, int width, int height, bool rowIsSelected) override;
    Component * refreshComponentForCell(int rowNumber, int columnId, bool isRowSelected, Component *existingComponentToUpdate) override;
private:
    //==============================================================================
    MainContentComponent& mainContentComponent;
    GrisLookAndFeel& grisFeel;

    std::unique_ptr<Box> boxListSpeaker;
    
    juce::TextButton butAddSpeaker;
    juce::TextButton butcompSpeakers;

    juce::Label      rNumOfSpeakersLabel;
    juce::TextEditor rNumOfSpeakers;
    juce::Label      rZenithLabel;
    juce::TextEditor rZenith;
    juce::Label      rRadiusLabel;
    juce::TextEditor rRadius;
    juce::Label      rOffsetAngleLabel;
    juce::TextEditor rOffsetAngle;
    juce::TextButton butAddRing;

    juce::ToggleButton pinkNoise;
    juce::Slider       pinkNoiseGain;

    juce::TableListBox tableListSpeakers;
    juce::Font font;
    int numRows;
    bool initialized{ false };
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EditSpeakersWindow);
};

#endif // EDITSPEAKERSWINDOW_H