//
//  UiComponent.h
//  spatServerGRIS
//
//  Created by GRIS on 2017-03-09.
//
//

#ifndef UiComponent_h
#define UiComponent_h

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




//======================================= LevelBox ===================================
class LevelBox : public Component
{
public:
    LevelBox(LevelComponent* parent, GrisLookAndFeel *feel);
    ~LevelBox();
    
    void setBounds(const Rectangle<int> &newBounds);
    void paint (Graphics& g);
    
private:
    LevelComponent *mainParent;
    GrisLookAndFeel *grisFeel;
    ColourGradient colorGrad;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LevelBox)
};

//======================================= Window Edit Speaker===========================
class WindowEditSpeaker : public DocumentWindow
{
public:
    WindowEditSpeaker(const String& name, Colour backgroundColour, int buttonsNeeded, MainContentComponent * parent, GrisLookAndFeel * feel);
    ~WindowEditSpeaker();
    
    void initComp();
    void closeButtonPressed();
    
private:
    MainContentComponent *mainParent;
    GrisLookAndFeel *grisFeel;
    Box * boxListSpeaker;
    Label *labColumn;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WindowEditSpeaker)
};


#endif /* UiComponent_h */

