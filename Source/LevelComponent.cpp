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
    this->muted = false;

    
    //Label================================================================
    this->indexLab = new Label();
    this->indexLab->setText(String(this->mainParent->getId()), dontSendNotification);//this->mainParent->getOutputPatch()
    this->indexLab->setSize(36, 22);
    this->indexLab->setJustificationType(Justification::centred);
    this->indexLab->setMinimumHorizontalScale(1);
    this->indexLab->setTopLeftPosition(0, 0);
    this->indexLab->setColour(Label::textColourId, this->grisFeel->getFontColour());
    this->indexLab->setLookAndFeel(this->grisFeel);
    this->addAndMakeVisible(this->indexLab);
    
    //ToggleButton=========================================================
    this->muteToggleBut = new ToggleButton();
    this->muteToggleBut->setButtonText("");
    this->muteToggleBut->setSize(36, 22);

    this->muteToggleBut->addListener(this);
    this->muteToggleBut->setToggleState(this->muted, dontSendNotification);
    this->muteToggleBut->setColour(ToggleButton::textColourId, this->grisFeel->getFontColour());
    this->muteToggleBut->setLookAndFeel(this->grisFeel);
    this->addAndMakeVisible(this->muteToggleBut);
    
    //Level BOX============================================================
    this->levelBox = new LevelBox(this, this->grisFeel);
    this->addAndMakeVisible(this->levelBox);

}


LevelComponent::~LevelComponent(){
    delete this->muteToggleBut;
    delete this->indexLab;
    delete this->levelBox;
}

void LevelComponent::buttonClicked(Button *button){
    if (button == this->muteToggleBut) {
        this->muted = this->muteToggleBut->getToggleState();
        this->levelBox->repaint();
    }
}


float LevelComponent::getLevel(){
    return level;
}

void LevelComponent::update(){
    float l = this->mainParent->getLevel();
    if(!this->muted && this->level != l){
        this->repaint();
    }
    this->level = l;
}


bool LevelComponent::isMuted(){
    return this->muted;
}

void LevelComponent::setSelected(bool value){
     if(value){
         this->indexLab->setColour(Label::textColourId, this->grisFeel->getWinBackgroundColour());
         this->indexLab->setColour(Label::backgroundColourId, this->grisFeel->getOnColour());
     }else{
         this->indexLab->setColour(Label::textColourId, this->grisFeel->getFontColour());
         this->indexLab->setColour(Label::backgroundColourId, this->grisFeel->getBackgroundColour());
     }
     this->repaint();
}

void LevelComponent::setBounds(const Rectangle<int> &newBounds){
    this->juce::Component::setBounds(newBounds);
    this->indexLab->setSize(newBounds.getWidth(), 22);
    this->muteToggleBut->setTopLeftPosition((newBounds.getWidth()/2)-10, getHeight()-22);
    
    int newWidth = newBounds.getWidth()/2;
    juce::Rectangle<int> level(newWidth/2, 18, newBounds.getWidth()-newWidth, getHeight()-40 );
    this->levelBox->setBounds(level);
}

