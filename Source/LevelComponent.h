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


#ifndef LevelComponent_h
#define LevelComponent_h

#include "../JuceLibraryCode/JuceHeader.h"
#include "GrisLookAndFeel.h"
#include "UiComponent.h"
#include "ParentLevelComponent.h"

static const float MinLevelComp  = -60.f;
static const float MaxLevelComp  = 1.f;
static const float MaxMinLevComp = MaxLevelComp - MinLevelComp;
static const int   WidthRect     = 2;

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

//====================================================================================
class LevelComponent :  public Component,
                        public ToggleButton::Listener,
                        public ChangeListener
{
public:
    LevelComponent(ParentLevelComponent * parent, GrisLookAndFeel *feel, bool colorful = true);
    ~LevelComponent();
    
    void setOutputLab(String value) { this->idBut->setButtonText(value); }
    void setColor(Colour color){ this->idBut->setColour(TextButton::buttonColourId, color); this->repaint(); }
    float getLevel();
    void update();
    bool isMuted();
    void setSelected(bool value);
    void buttonClicked(Button *button) override;
    void setBounds(const Rectangle<int> &newBounds);
    void changeListenerCallback (ChangeBroadcaster* source) override;
    
    
private:
    ParentLevelComponent* mainParent;
    LevelBox * levelBox;
    //Label * labId;
    TextButton * idBut;
    ToggleButton * muteToggleBut;
    ToggleButton * soloToggleBut;
    GrisLookAndFeel * grisFeel;
    float level = MinLevelComp;
    bool isColorful;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LevelComponent)
};
#endif /* LevelComponent_h */
