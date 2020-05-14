/*
 This file is part of SpatGRIS2.
 
 Developers: Olivier Belanger, Nicolas Masson
 
 SpatGRIS2 is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.
 
 SpatGRIS2 is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with SpatGRIS2.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef LEVELCOMPONENT_H
#define LEVELCOMPONENT_H

#include "../JuceLibraryCode/JuceHeader.h"

#include "GrisLookAndFeel.h"
#include "ParentLevelComponent.h"
#include "Speaker.h"
#include "UiComponent.h"

static const float MinLevelComp  = -60.f;
static const float MaxLevelComp  = 0.f;
static const int   WidthRect     = 1;

//==============================================================================
//============================ LevelBox ================================
class LevelBox : public Component
{
public:
    LevelBox(LevelComponent* parent, SmallGrisLookAndFeel *feel);
    ~LevelBox() override = default;
    //==============================================================================
    void setBounds(const juce::Rectangle<int> &newBounds);
    void paint(Graphics& g) override;
    void mouseDown(const MouseEvent& e) override;
    void resetClipping();
private:
    //==============================================================================
    LevelComponent *mainParent;
    SmallGrisLookAndFeel *grisFeel;
    ColourGradient colorGrad;
    Image vumeterBit;
    Image vumeterBackBit;
    Image vumeterMutedBit;
    bool isClipping = false;
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LevelBox)
};

//==============================================================================
//======================== LevelComponent ==============================
class LevelComponent : public Component,
                       public ToggleButton::Listener,
                       public ChangeListener
{
public:
    LevelComponent(ParentLevelComponent * parent,
                   SmallGrisLookAndFeel *feel,
                   bool colorful = true);
    ~LevelComponent() = default;
    //==============================================================================
    void setOutputLab(String value) { this->idBut->setButtonText(value); }
    void setColor(Colour color) {
        this->idBut->setColour(TextButton::buttonColourId, color);
        this->repaint();
    }
    float getLevel();
    void update();
    bool isMuted();
    void setSelected(bool value);
    void buttonClicked(Button *button) override;
    void mouseDown(const MouseEvent& e) override;
    void setBounds(const juce::Rectangle<int> &newBounds);
    void changeListenerCallback (ChangeBroadcaster* source) override;
    void updateDirectOutMenu(juce::OwnedArray<Speaker> & spkList);
    void resetClipping();
    //==============================================================================
    std::vector<int> directOutSpeakers;
    std::unique_ptr<TextButton> directOut;
private:
    //==============================================================================
    ParentLevelComponent* mainParent;
    std::unique_ptr<LevelBox> levelBox;
    std::unique_ptr<TextButton> idBut;
    std::unique_ptr<ToggleButton> muteToggleBut;
    std::unique_ptr<ToggleButton> soloToggleBut;
    SmallGrisLookAndFeel * grisFeel;
    float level = MinLevelComp;
    int lastMouseButton = 1; // 1 means left, 0 means right
    bool isColorful;
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LevelComponent)
};

#endif // LEVELCOMPONENT_H
