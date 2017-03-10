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

//======================================= BOX ===========================================================================
class Box : public Component
{
public:
    Box(bool useViewport, GrisLookAndFeel *feel, String title="") {
        if (useViewport) {
            this->content = new Component();
            this->viewport = new Viewport();
            this->viewport->setViewedComponent(this->content, false);
            this->viewport->setScrollBarsShown(true, true);
            this->viewport->setScrollBarThickness(6);
            this->viewport->getVerticalScrollBar()->setColour(ScrollBar::ColourIds::thumbColourId, feel->getScrollBarColour());
            this->viewport->getHorizontalScrollBar()->setColour(ScrollBar::ColourIds::thumbColourId, feel->getScrollBarColour());
            
            this->viewport->setLookAndFeel(feel);
            addAndMakeVisible(this->viewport);
        }
        this->title = title;
        this->grisFeel = feel;
        this->bgColour = this->grisFeel->getBackgroundColour();
    }
    
    ~Box()
    { }
    
    void correctSize(int width, int height){
        if(this->title!=""){
            this->viewport->setTopLeftPosition(0, 20);
            this->content->setSize(width, height+20);

        }else{
            this->viewport->setTopLeftPosition(0, 0);
            this->content->setSize(width, height);

        }
        
    }
    
    Component * getContent() {
        return this->content.get() ? this->content.get() : this;
    }
    
    Viewport * getViewport() {
        return this->viewport.get() ;
    }
    
    void paint(Graphics &g) {
        g.setColour(this->bgColour);
        g.fillRect(getLocalBounds());
        if(this->title!=""){
            g.setColour (this->grisFeel->getWinBackgroundColour());
            g.fillRect(0,0,getWidth(),18);
            
            g.setColour (this->grisFeel->getFontColour());
            g.drawText(title, 0, 0, this->content->getWidth(), 20, juce::Justification::left);
        }
    }
    
    void resized() {
        if (this->viewport){
            this->viewport->setSize(getWidth(), getHeight());
        }
    }
    
private:
    ScopedPointer<Component> content;
    ScopedPointer<Viewport> viewport;
    GrisLookAndFeel *grisFeel;
    Colour bgColour;
    String title = "";
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Box)
};
#endif /* UiComponent_h */


