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
#include "jackClientGRIS.h"

class MainContentComponent;
class LevelComponent;

using namespace std;

//======================================= BOX ========================================
class Box : public Component
{
public:
    Box(GrisLookAndFeel *feel, String title="", bool verticalScrollbar=false, bool horizontalScrollbar=true);
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


//======================================= BoxClient ==================================
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
    String getText(const int columnNumber, const int rowNumber) const;
    void setValue(const int rowNumber,const int columnNumber, const int newRating);
    int getValue(const int rowNumber,const int columnNumber) const;
    int getNumRows() override;
    void paintRowBackground(Graphics& g, int rowNumber, int /*width*/, int /*height*/, bool rowIsSelected) override;
    void paintCell(Graphics& g, int rowNumber, int columnId,
                   int width, int height, bool /*rowIsSelected*/) override;
    
    Component* refreshComponentForCell(int rowNumber, int columnId, bool /*isRowSelected*/,
                                       Component* existingComponentToUpdate) override;
    
private:
    MainContentComponent *mainParent;
    GrisLookAndFeel *grisFeel;
    unsigned int numRows;
    TableListBox tableListClient;
    Box * box;
    
    class ListIntOutComp : public Component, 
                           public ComboBox::Listener
    {
    public:
        ListIntOutComp (BoxClient& td) : owner (td) {
            // Just put a combo box inside this component.
            addAndMakeVisible (comboBox);
            for (int i = 1; i <= 256; i++) {
                comboBox.addItem(String(i), i);
            }
            comboBox.addListener(this);
            comboBox.setWantsKeyboardFocus(false);
        }
        
        void resized() override {
            comboBox.setBoundsInset(BorderSize<int> (2));
        }

        void setRowAndColumn(int newRow, int newColumn) {
            row = newRow;
            columnId = newColumn;
            comboBox.setSelectedId(owner.getValue(row,columnId), dontSendNotification);
        }
        
        void comboBoxChanged (ComboBox*) override {
            owner.setValue(row, columnId, comboBox.getSelectedId());
        }
        
    private:
        BoxClient& owner;
        ComboBox comboBox;
        int row, columnId;
    };
};

//======================================= WindowProperties ===========================
class WindowProperties : public juce::Component,
                         public TextButton::Listener
{
public:
    WindowProperties( MainContentComponent *parent, GrisLookAndFeel *feel, Array<String> devices,
                      String currentDevice, int indR=0, int indB=0, int indFF=0, int indFC=0, int indAttDB=2, int indAttHz=3,
                      int oscPort=18032);
    ~WindowProperties();

    Label * createPropLabel(String lab, Justification::Flags just, int ypos, int width=100);
    TextEditor * createPropIntTextEditor(String tooltip, int ypos, int init);
    ComboBox * createPropComboBox(const StringArray choices, int selected, int ypos);

    void buttonClicked(Button *button);
    void closeButtonPressed();

private:
    MainContentComponent *mainParent;
    GrisLookAndFeel *grisFeel;

    Label *generalLabel;
    Label *jackSettingsLabel;
    Label *recordingLabel;
    Label *cubeDistanceLabel;

    Label *labOSCInPort;
    TextEditor *tedOSCInPort;

    Label *labDevice;
    ComboBox *cobDevice = nullptr;

    Label *labRate;
    ComboBox *cobRate;

    Label *labBuff;
    ComboBox *cobBuffer;

    Label *labRecFormat;
    ComboBox *recordFormat;

    Label *labRecFileConfig;
    ComboBox *recordFileConfig;

    Label *labDistanceDB;
    ComboBox *cobDistanceDB;

    Label *labDistanceCutoff;
    ComboBox *cobDistanceCutoff;

    TextButton *butValidSettings;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WindowProperties);
};

//======================================= About Window ===========================
class AboutWindow : public DocumentWindow,
                    public TextButton::Listener
{
public:
    AboutWindow(const String& name, Colour backgroundColour, int buttonsNeeded,
                MainContentComponent *parent, GrisLookAndFeel *feel);
    ~AboutWindow();
    void buttonClicked(Button *button);
    void closeButtonPressed();

private:
    MainContentComponent *mainParent;
    GrisLookAndFeel *grisFeel;
    ImageComponent *imageComponent;
    Label *title;
    Label *version;
    Label *label;
    HyperlinkButton *website;
    TextButton *close;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AboutWindow);
};

//======================================= OSC Log Window ===========================
class OscLogWindow : public DocumentWindow,
                     public TextButton::Listener
{
public:
    OscLogWindow(const String& name, Colour backgroundColour, int buttonsNeeded,
                 MainContentComponent *parent, GrisLookAndFeel *feel);
    ~OscLogWindow();
    void buttonClicked(Button *button);
    void closeButtonPressed();
    void addToLog(String msg);

private:
    MainContentComponent *mainParent;
    GrisLookAndFeel *grisFeel;
    int index;
    bool activated;
    CodeDocument codeDocument;
    CodeEditorComponent logger;
    TextButton stop;
    TextButton close;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OscLogWindow);
};

#endif /* UICOMPONENT_H */

