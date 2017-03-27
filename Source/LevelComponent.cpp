//
//  LevelComponent.cpp
//  spatServerGRIS
//
//  Created by GRIS on 2017-03-13.
//
//

#include "LevelComponent.h"
#include "Speaker.h"

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


//======================================================================================================================
LevelComponent::LevelComponent(ParentLevelComponent* parent, GrisLookAndFeel *feel){
    this->mainParent = parent;
    this->grisFeel = feel;
    
    //Label================================================================
    this->labId = new Label();
    this->labId->setText(String(this->mainParent->getId()), dontSendNotification);//this->mainParent->getOutputPatch()
    this->labId->setSize(36, 22);
    this->labId->setJustificationType(Justification::centred);
    this->labId->setMinimumHorizontalScale(1);
    this->labId->setTopLeftPosition(0, 0);
    this->labId->setColour(Label::textColourId, this->grisFeel->getFontColour());
    this->labId->setLookAndFeel(this->grisFeel);
    this->addAndMakeVisible(this->labId);
    
    //ToggleButton=========================================================
    this->muteToggleBut = new ToggleButton();
    this->muteToggleBut->setButtonText("M");
    this->muteToggleBut->setSize(18, 18);
    this->muteToggleBut->setTooltip ("Mute "+String(this->mainParent->getId()));
    this->muteToggleBut->addListener(this);
    this->muteToggleBut->setToggleState(false, dontSendNotification);
    this->muteToggleBut->setLookAndFeel(this->grisFeel);
    this->muteToggleBut->setColour(ToggleButton::textColourId, this->grisFeel->getFontColour());
    this->addAndMakeVisible(this->muteToggleBut);
    
    //ToggleButton=========================================================
    this->soloToggleBut = new ToggleButton();
    this->soloToggleBut->setButtonText("S");
    this->soloToggleBut->setSize(18, 18);
    this->soloToggleBut->setTooltip ("Solo "+String(this->mainParent->getId()));
    this->soloToggleBut->addListener(this);
    this->soloToggleBut->setToggleState(false, dontSendNotification);
    this->soloToggleBut->setColour(ToggleButton::textColourId, this->grisFeel->getFontColour());
    this->soloToggleBut->setLookAndFeel(this->grisFeel);
    this->addAndMakeVisible(this->soloToggleBut);
    
    //Level BOX============================================================
    this->levelBox = new LevelBox(this, this->grisFeel);
    this->addAndMakeVisible(this->levelBox);
}


LevelComponent::~LevelComponent(){
    delete this->muteToggleBut;
    delete this->soloToggleBut;
    delete this->labId;
    delete this->levelBox;
}

void LevelComponent::buttonClicked(Button *button){
    if (button == this->muteToggleBut) {
        this->mainParent->setMuted(this->muteToggleBut->getToggleState());
        this->levelBox->repaint();
    }else if (button == this->soloToggleBut) {
        this->mainParent->setSolo(this->soloToggleBut->getToggleState());
    }
}


float LevelComponent::getLevel(){
    return level;
}

void LevelComponent::update(){
    float l = this->mainParent->getLevel();
    if(isnan(l)){ return; }
    if(!this->muteToggleBut->getToggleState() && this->level != l){
        this->repaint();
    }
    this->level = l;
}


bool LevelComponent::isMuted(){
    return this->muteToggleBut->getToggleState();
}

void LevelComponent::setSelected(bool value){
     if(value){
         this->labId->setColour(Label::textColourId, this->grisFeel->getWinBackgroundColour());
         this->labId->setColour(Label::backgroundColourId, this->grisFeel->getOnColour());
     }else{
         this->labId->setColour(Label::textColourId, this->grisFeel->getFontColour());
         this->labId->setColour(Label::backgroundColourId, this->grisFeel->getBackgroundColour());
     }
     this->repaint();
}

void LevelComponent::setBounds(const Rectangle<int> &newBounds){
    this->juce::Component::setBounds(newBounds);

    juce::Rectangle<int> labRect(WidthRect/2, 0, newBounds.getWidth()-WidthRect, this->labId->getHeight() );
     this->labId->setBounds(labRect);
    //this->labId->setSize(newBounds.getWidth(), 22);
    this->muteToggleBut->setBounds((newBounds.getWidth()/2)-16, getHeight()-22, this->muteToggleBut->getWidth(), this->muteToggleBut->getHeight());
    this->soloToggleBut->setBounds((newBounds.getWidth()/2), getHeight()-22, this->muteToggleBut->getWidth(), this->muteToggleBut->getHeight());

    juce::Rectangle<int> level(WidthRect/2, 18, newBounds.getWidth()-WidthRect, getHeight()-40 );
    this->levelBox->setBounds(level);
}

