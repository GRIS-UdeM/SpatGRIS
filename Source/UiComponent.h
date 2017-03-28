//
//  UiComponent.h
//  spatServerGRIS
//
//  Created by GRIS on 2017-03-09.
//
//

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
    void correctSize(int width, int height);
    void paint(Graphics &g) ;
    
private:
    Component *content;
    Viewport *viewport;
    GrisLookAndFeel *grisFeel;
    Colour bgColour;
    String title;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Box)
};



//======================================= BoxClient===========================

class BoxClient :   public Component,
                    public TableListBoxModel,
                    public ToggleButton::Listener

{
public:
    BoxClient(MainContentComponent * parent, GrisLookAndFeel *feel);
    ~BoxClient();
    
    void updateContentCli();
    void buttonClicked(Button *button);
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
    int numRows;
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
            for(int i = 1; i <= 64; i++){
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
                            public ToggleButton::Listener
{
public:
    WindowEditSpeaker(const String& name, Colour backgroundColour, int buttonsNeeded, MainContentComponent * parent, GrisLookAndFeel * feel);
    ~WindowEditSpeaker();
    
    void updateWinContent();
    void selectedRow(int value){ this->tableListSpeakers.selectRow(value); this->repaint();}
    
    void initComp();
    void buttonClicked(Button *button);
    void closeButtonPressed();
    
    void resized() override;
    
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


#endif /* UiComponent_h */

