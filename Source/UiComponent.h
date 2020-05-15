/*
 This file is part of SpatGRIS2.
 
 Developers: Olivier Belanger, Nicolas Masson
 
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

#ifndef UICOMPONENT_H
#define UICOMPONENT_H

#include <iostream>

#include "../JuceLibraryCode/JuceHeader.h"

#include "GrisLookAndFeel.h"
#include "JackClientGRIS.h"

class LevelComponent;
class MainContentComponent;

//==============================================================================
//======================================= BOX ========================================
class Box final : public Component
{
public:
    Box(GrisLookAndFeel *feel, String title="", bool verticalScrollbar=false, bool horizontalScrollbar=true);
    ~Box();
    //==============================================================================
    Component       * getContent()       { return this->content ? this->content : this; }
    Component const * getContent() const { return this->content ? this->content : this; }
    void resized() final;
    void correctSize(unsigned int width, unsigned int height);
    void paint(Graphics &g) final;
private:
    //==============================================================================
    juce::Component *content;
    juce::Viewport *viewport;
    GrisLookAndFeel *grisFeel;
    juce::Colour bgColour;
    juce::String title;
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Box)
};

//==============================================================================
//======================================= BoxClient ==================================
class BoxClient final 
    : public Component
    , public TableListBoxModel
    , public ToggleButton::Listener
{
public:
    BoxClient(MainContentComponent * parent, GrisLookAndFeel *feel);
    ~BoxClient() final = default;
    //==============================================================================
    void updateContentCli();
    void buttonClicked(Button *button) final;
    void setBounds(int x, int y, int width, int height);
    String getText(const int columnNumber, const int rowNumber) const;
    void setValue(const int rowNumber,const int columnNumber, const int newRating);
    int getValue(const int rowNumber,const int columnNumber) const;
    int getNumRows() final { return numRows; }
    void paintRowBackground(Graphics& g, int rowNumber, int /*width*/, int /*height*/, bool rowIsSelected) final;
    void paintCell(Graphics& g, int rowNumber, int columnId,
                   int width, int height, bool /*rowIsSelected*/) final;
    
    Component* refreshComponentForCell(int rowNumber, int columnId, bool /*isRowSelected*/,
                                       Component* existingComponentToUpdate) final;
private:
    //==============================================================================
    MainContentComponent *mainParent;
    GrisLookAndFeel *grisFeel;
    unsigned int numRows;
    TableListBox tableListClient;
    Box * box;
    //==============================================================================
    class ListIntOutComp final 
        : public Component
        , public ComboBox::Listener
    {
    public:
        ListIntOutComp (BoxClient& td) : owner (td) {
            // Just put a combo box inside this component.
            this->addAndMakeVisible(comboBox);
            for (int i = 1; i <= 256; i++) {
                comboBox.addItem(String(i), i);
            }
            comboBox.addListener(this);
            comboBox.setWantsKeyboardFocus(false);
        }
        //==============================================================================
        void resized() final {
            comboBox.setBoundsInset(BorderSize<int> (2));
        }

        void setRowAndColumn(int newRow, int newColumn) {
            row = newRow;
            columnId = newColumn;
            comboBox.setSelectedId(owner.getValue(row,columnId), dontSendNotification);
        }
        
        void comboBoxChanged (ComboBox*) final {
            owner.setValue(row, columnId, comboBox.getSelectedId());
        }
    private:
        //==============================================================================
        BoxClient& owner;
        ComboBox comboBox;
        int row;
        int columnId;
    };
};

#endif /* UICOMPONENT_H */

