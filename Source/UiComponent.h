/*
 This file is part of spatServerGRIS.
 
 Developers: Nicolas Masson
 
 spatServerGRIS is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.
 
 spatServerGRIS is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with spatServerGRIS.  If not, see <http://www.gnu.org/licenses/>.
 */


#ifndef UiComponent_h
#define UiComponent_h

#include <iostream>

#include "../JuceLibraryCode/JuceHeader.h"
#include "GrisLookAndFeel.h"

class MainContentComponent;
class LevelComponent;

using namespace std;


//======================================= BOX ========================================
class Box : public Component
{
public:
    Box(GrisLookAndFeel *feel, String title="");
    ~Box();
    
    Component * getContent();
    void resized();
    void correctSize(unsigned int width, unsigned int height);
    void paint(Graphics &g) ;
    
private:
    Component *content;
    Viewport *viewport;
    GrisLookAndFeel *grisFeel;
    Colour bgColour;
    String title;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Box)
};



//======================================= BoxClient ===========================

class BoxClient :   public Component,
                    public TableListBoxModel,
                    public ToggleButton::Listener

{
public:
    BoxClient(MainContentComponent * parent, GrisLookAndFeel *feel);
    ~BoxClient();
    
    void updateContentCli();
    void buttonClicked(Button *button) override;
    void setBounds(int x, int y, int width, int height);
    String getText (const int columnNumber, const int rowNumber) const;
    void setValue (const int rowNumber,const int columnNumber, const int newRating);
    int getValue (const int rowNumber,const int columnNumber) const;
    int getNumRows() override;
    void paintRowBackground (Graphics& g, int rowNumber, int /*width*/, int /*height*/, bool rowIsSelected) override;
    void paintCell (Graphics& g, int rowNumber, int columnId,
                    int width, int height, bool /*rowIsSelected*/) override;
    
    Component* refreshComponentForCell (int rowNumber, int columnId, bool /*isRowSelected*/,
                                        Component* existingComponentToUpdate) override;
    
private:
    MainContentComponent *mainParent;
    GrisLookAndFeel *grisFeel;
    unsigned int numRows;
    TableListBox tableListClient;
    Box * box;
    
    class ListIntOutComp    : public Component,
    private ComboBoxListener
    {
    public:
        ListIntOutComp (BoxClient& td)  : owner (td)
        {
            // just put a combo box inside this component
            addAndMakeVisible (comboBox);
            for(int i = 1; i <= 256; i++){
                comboBox.addItem (String(i), i);
            }
            
            
            // when the combo is changed, we'll get a callback.
            comboBox.addListener (this);
            comboBox.setWantsKeyboardFocus (false);
        }
        
        void resized() override
        {
            comboBox.setBoundsInset (BorderSize<int> (2));
        }
        
        // Our demo code will call this when we may need to update our contents
        void setRowAndColumn (int newRow, int newColumn)
        {
            row = newRow;
            columnId = newColumn;
            comboBox.setSelectedId (owner.getValue (row,columnId), dontSendNotification);
        }
        
        void comboBoxChanged (ComboBox*) override
        {
            owner.setValue (row, columnId, comboBox.getSelectedId());
        }
        
    private:
        BoxClient& owner;
        ComboBox comboBox;
        int row, columnId;
    };
};



//======================================= Window Edit Speaker===========================
class WindowEditSpeaker :   public DocumentWindow,
                            public TableListBoxModel,
                            public ToggleButton::Listener,
                            public TextEditor::Listener
{
public:
    WindowEditSpeaker(const String& name,String& nameC, Colour backgroundColour, int buttonsNeeded, MainContentComponent * parent, GrisLookAndFeel * feel);
    ~WindowEditSpeaker();
    
    void updateWinContent();
    void selectedRow(int value);
    
    void initComp();
    void buttonClicked(Button *button) override;
    void closeButtonPressed() override;
    
    void resized() override;
    void textEditorFocusLost (TextEditor &textEditor) override;
    void textEditorReturnKeyPressed (TextEditor &textEditor) override;

    String getText (const int columnNumber, const int rowNumber) const;
    void setText (const int columnNumber, const int rowNumber, const String& newText);
    int getNumRows() override;
    void paintRowBackground (Graphics& g, int rowNumber, int /*width*/, int /*height*/, bool rowIsSelected) override;
    void paintCell (Graphics& g, int rowNumber, int columnId,
                    int width, int height, bool /*rowIsSelected*/) override;
    Component* refreshComponentForCell (int rowNumber, int columnId, bool /*isRowSelected*/,
                                        Component* existingComponentToUpdate) override;
    
private:
    
    MainContentComponent *mainParent;
    GrisLookAndFeel *grisFeel;
    Box * boxListSpeaker;
    
    ToggleButton *toggleShowSphere;
    TextButton *butAddSpeaker;
    TextButton *butsaveSpeakers;
    TextEditor *texEditNameConf;
    
    TableListBox tableListSpeakers;
    Font font;
    int numRows;
    
    
    class EditableTextCustomComponent  : public Label
    {
    public:
        EditableTextCustomComponent (WindowEditSpeaker& td)  : owner (td)
        {
            setEditable (false, true, false);
            setColour (textColourId, Colours::black);
        }
        void mouseDown (const MouseEvent& event) override
        {
            owner.tableListSpeakers.selectRowsBasedOnModifierKeys (row, event.mods, false);
            Label::mouseDown (event);
        }
        void textWasEdited() override
        {
            owner.setText (columnId, row, getText());
        }
       void setRowAndColumn (const int newRow, const int newColumn)
        {
            row = newRow;
            columnId = newColumn;
            setText (owner.getText(columnId, row), dontSendNotification);
        }
    private:
        WindowEditSpeaker& owner;
        int row, columnId;
    };
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WindowEditSpeaker)
};


//======================================= WinJackSettings ===========================

class WindowJackSetting :   public DocumentWindow,
                            public TextButton::Listener

{
public:
    WindowJackSetting(const String& name, Colour backgroundColour, int buttonsNeeded,MainContentComponent * parent, GrisLookAndFeel * feel, int indR=0, int indB=0);
    ~WindowJackSetting();

    void buttonClicked(Button *button);
    void closeButtonPressed();
    
    
private:
    MainContentComponent *mainParent;
    GrisLookAndFeel *grisFeel;
    
    Label *labRate;
    Label *labBuff;
    ComboBox *cobRate;
    ComboBox *cobBuffer;
    TextButton *butValidSettings;

};
#endif /* UiComponent_h */

