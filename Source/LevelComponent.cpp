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
LevelComponent::LevelComponent(ParentLevelComponent* parent, GrisLookAndFeel *feel, bool colorful){
    this->mainParent = parent;
    this->grisFeel = feel;
    this->isColorful = colorful;
    
    //Label================================================================
    this->idBut = new TextButton();
    this->idBut->setButtonText(String(this->mainParent->getId()));//this->mainParent->getOutputPatch()
    this->idBut->setTooltip(String(this->mainParent->getId()));
    this->idBut->setSize(36, 22);
    //this->idBut->setJustificationType(Justification::centred);
    //this->idBut->setMinimumHorizontalScale(1);
    this->idBut->setTopLeftPosition(0, 0);
    this->idBut->setColour(Label::textColourId, this->grisFeel->getFontColour());
    this->idBut->setLookAndFeel(this->grisFeel);
    this->idBut->setColour(TextButton::textColourOnId, this->grisFeel->getFontColour());
    this->idBut->setColour(TextButton::textColourOffId, this->grisFeel->getFontColour());
    this->idBut->setColour(TextButton::buttonColourId, this->grisFeel->getBackgroundColour());
    this->idBut->addListener(this);
    this->addAndMakeVisible(this->idBut);
    
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
    

    /*if(!this->isColorful){
        this->setSelected(false);
    }*/
}


LevelComponent::~LevelComponent(){
    delete this->muteToggleBut;
    delete this->soloToggleBut;
    delete this->idBut;
    delete this->levelBox;
}

void LevelComponent::buttonClicked(Button *button){
    if (button == this->muteToggleBut) {
        this->mainParent->setMuted(this->muteToggleBut->getToggleState());
        this->levelBox->repaint();
        
    }else if (button == this->soloToggleBut) {
        this->mainParent->setSolo(this->soloToggleBut->getToggleState());
        
    }else if (button == this->idBut) {
        if( this->isColorful){  //Input
            ColourSelector* colourSelector = new ColourSelector();
            colourSelector->setName ("background");
            colourSelector->setCurrentColour (this->idBut->findColour (TextButton::buttonColourId));
            colourSelector->addChangeListener (this);
            colourSelector->setColour (ColourSelector::backgroundColourId, Colours::transparentBlack);
            colourSelector->setSize (300, 400);
            CallOutBox::launchAsynchronously (colourSelector, getScreenBounds(), nullptr);
        }else{      //Output
            this->mainParent->selectClick();
        }
    }
}

void LevelComponent::changeListenerCallback (ChangeBroadcaster* source)
{
    if (ColourSelector* cs = dynamic_cast<ColourSelector*> (source)){
        this->idBut->setColour(TextButton::buttonColourId, cs->getCurrentColour());
        this->mainParent->setColor(cs->getCurrentColour());
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
    const MessageManagerLock mmLock;
     if(value){
         this->idBut->setColour(TextButton::textColourOnId, this->grisFeel->getWinBackgroundColour());
         this->idBut->setColour(TextButton::textColourOffId, this->grisFeel->getWinBackgroundColour());
         this->idBut->setColour(TextButton::buttonColourId, this->grisFeel->getOnColour());
     }else{
         this->idBut->setColour(TextButton::textColourOnId, this->grisFeel->getFontColour());
         this->idBut->setColour(TextButton::textColourOffId, this->grisFeel->getFontColour());
         this->idBut->setColour(TextButton::buttonColourId, this->grisFeel->getBackgroundColour());
     }
}

void LevelComponent::setBounds(const Rectangle<int> &newBounds){
    this->juce::Component::setBounds(newBounds);

    juce::Rectangle<int> labRect(WidthRect/2, 0, newBounds.getWidth()-WidthRect, this->idBut->getHeight() );
    this->idBut->setBounds(labRect);
    //this->labId->setSize(newBounds.getWidth(), 22);
    this->muteToggleBut->setBounds((newBounds.getWidth()/2)-16, getHeight()-22, this->muteToggleBut->getWidth(), this->muteToggleBut->getHeight());
    this->soloToggleBut->setBounds((newBounds.getWidth()/2), getHeight()-22, this->muteToggleBut->getWidth(), this->muteToggleBut->getHeight());

    juce::Rectangle<int> level(WidthRect/2, 18, newBounds.getWidth()-WidthRect, getHeight()-40 );
    this->levelBox->setBounds(level);
}

