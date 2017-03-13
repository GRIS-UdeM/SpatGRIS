//
//  LevelComponent.cpp
//  spatServerGRIS
//
//  Created by GRIS on 2017-03-13.
//
//

#include "LevelComponent.h"
#include "MainComponent.h"

LevelComponent::LevelComponent(MainContentComponent* parent, GrisLookAndFeel *feel, int id){
    this->mainParent = parent;
    this->grisFeel = feel;
    this->index = id;
    this->muted = false;
    
    //Level BOX============================================================
    this->levelBox = new LevelBox(this, this->grisFeel);
    this->addAndMakeVisible(this->levelBox);
    
    //Label================================================================
    this->indexLab = new Label();
    this->indexLab->setText(String(this->index), dontSendNotification);
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
    return linearToDb(this->mainParent->getLevel(this->index));
}

bool LevelComponent::isMuted(){
    return this->muted;
}

void LevelComponent::setBounds(const Rectangle<int> &newBounds){
    this->juce::Component::setBounds(newBounds);
    this->indexLab->setSize(newBounds.getWidth(), 22);
    this->muteToggleBut->setTopLeftPosition((newBounds.getWidth()/2)-10, getHeight()-22);
    
    int newWidth = newBounds.getWidth()/2;
    juce::Rectangle<int> level(newWidth/2, 18, newBounds.getWidth()-newWidth, getHeight()-40 );
    this->levelBox->setBounds(level);
}

