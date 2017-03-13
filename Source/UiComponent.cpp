//
//  UiComponent.cpp
//  spatServerGRIS
//
//  Created by GRIS on 2017-03-13.
//
//

#include "UiComponent.h"
#include "LevelComponent.h"


//======================================= BOX ===========================================================================
Box::Box(GrisLookAndFeel *feel, String title) {
    
    this->title = title;
    this->grisFeel = feel;
    this->bgColour = this->grisFeel->getBackgroundColour();
    
    this->content = new Component();
    this->viewport = new Viewport();
    this->viewport->setViewedComponent(this->content, false);
    this->viewport->setScrollBarsShown(true, true);
    this->viewport->setScrollBarThickness(6);
    this->viewport->getVerticalScrollBar()->setColour(ScrollBar::ColourIds::thumbColourId, feel->getScrollBarColour());
    this->viewport->getHorizontalScrollBar()->setColour(ScrollBar::ColourIds::thumbColourId, feel->getScrollBarColour());
    
    this->viewport->setLookAndFeel(this->grisFeel);
    addAndMakeVisible(this->viewport);
}

Box::~Box(){
    delete this->viewport;
    delete this->content;
}

Component * Box::getContent() {
    return this->content ? this->content : this;
}


void Box::resized() {
    if (this->viewport){
        this->viewport->setSize(getWidth(), getHeight());
    }
}

void Box::correctSize(int width, int height){
    if(this->title!=""){
        this->viewport->setTopLeftPosition(0, 20);
        this->viewport->setSize(getWidth(), getHeight()-20);
    }else{
        this->viewport->setTopLeftPosition(0, 0);
    }
    this->getContent()->setSize(width, height);
}


void Box::paint(Graphics &g) {
    g.setColour(this->bgColour);
    g.fillRect(getLocalBounds());
    if(this->title!=""){
        g.setColour (this->grisFeel->getWinBackgroundColour());
        g.fillRect(0,0,getWidth(),18);
        
        g.setColour (this->grisFeel->getFontColour());
        g.drawText(title, 0, 0, this->content->getWidth(), 20, juce::Justification::left);
    }
}




//======================================= LevelBox =====================================================================
LevelBox::LevelBox(LevelComponent * parent, GrisLookAndFeel *feel):
    mainParent(parent),
    grisFeel(feel)
{

}

LevelBox::~LevelBox(){
    
}


void LevelBox::setBounds(const Rectangle<int> &newBounds){
    this->juce::Component::setBounds(newBounds);
    colorGrad = ColourGradient(Colours::red, 0.f, 0.f, Colour::fromRGB(17, 255, 159), 0.f, getHeight(), false);
    colorGrad.addColour(0.1, Colours::yellow);
}

void LevelBox::paint (Graphics& g){
    if(this->mainParent->isMuted()){
        g.fillAll (grisFeel->getWinBackgroundColour());
    }
    else{
        float level = this->mainParent->getLevel();
        g.setGradientFill(colorGrad);
        g.fillRect(0, 0, getWidth() ,getHeight());
        
        if (level < MinLevelComp){
            level = MinLevelComp;
        }
        if (level < 0.9f){
            level = -abs(level);
            g.setColour(grisFeel->getDarkColour());
            g.fillRect(0, 0, getWidth() ,(int)(getHeight()*(level/MinLevelComp)));
        }
    }
}

