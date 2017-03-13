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

//======================================= BOX ===========================================================================
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





//======================================= LevelBox =====================================================================

class LevelBox : public Component
{
public:
    LevelBox(MainContentComponent* parent, GrisLookAndFeel *feel, int index);
    ~LevelBox();
    
    void setMute(bool b);
    void setBounds(const Rectangle<int> &newBounds);
    void paint (Graphics& g);
    
private:
    MainContentComponent *mainParent;
    GrisLookAndFeel *grisFeel;
    ColourGradient colorGrad;
    int mIndex;
    bool muted;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LevelBox)
};

#endif /* UiComponent_h */

