//
//  UiComponent.cpp
//  spatServerGRIS
//
//  Created by GRIS on 2017-03-13.
//
//

#include "UiComponent.h"
#include "LevelComponent.h"
#include "MainComponent.h"

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
        if(width<20){
            width = 20;
        }
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


//======================================= Window Edit Speaker============================================================

WindowEditSpeaker::WindowEditSpeaker(const String& name, Colour backgroundColour, int buttonsNeeded,
                                     MainContentComponent * parent, GrisLookAndFeel * feel):
    DocumentWindow (name, backgroundColour, buttonsNeeded)
{
    this->mainParent = parent;
    this->grisFeel = feel;
    
}
WindowEditSpeaker::~WindowEditSpeaker(){
    delete this->labColumn;
    delete this->boxListSpeaker;
    this->mainParent->destroyWinSpeakConf();
}
void WindowEditSpeaker::initComp(){
    this->boxListSpeaker = new Box(this->grisFeel, "Configuration Speakers");
    this->setContentComponent(this->boxListSpeaker);
    
    this->labColumn = new Label();
    this->labColumn->setText("X                   Y                   Z                       Azimuth         Zenith          Radius          #",
                             NotificationType::dontSendNotification);
    this->labColumn->setJustificationType(Justification::left);
    this->labColumn->setFont(this->grisFeel->getFont());
    this->labColumn->setLookAndFeel(this->grisFeel);
    this->labColumn->setColour(Label::textColourId, this->grisFeel->getFontColour());
    this->labColumn->setBounds(25, 0, getWidth(), 22);
    this->boxListSpeaker->getContent()->addAndMakeVisible(this->labColumn);
    
    int y = 18;
    for (auto&& it : this->mainParent->getListSpeaker())
    {
        juce::Rectangle<int> boundsSpeak(0, y,550, 26);
        it->setBounds(boundsSpeak);
        this->boxListSpeaker->getContent()->addAndMakeVisible(it);
        y+=28;
    }
    
    
    this->boxListSpeaker->setBounds(0, 0, getWidth(),getHeight());
    this->boxListSpeaker->correctSize(getWidth()-8, (this->mainParent->getListSpeaker().size()*28)+50);
}

void WindowEditSpeaker::closeButtonPressed()
{

    delete this;
}

