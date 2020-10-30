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

#pragma once

#include "macros.h"

DISABLE_WARNINGS
#include <JuceHeader.h>
ENABLE_WARNINGS

#include "Box.h"
#include "GrisLookAndFeel.h"
#include "ParentLevelComponent.h"
#include "Speaker.h"

static const float MinLevelComp = -60.f;
static const float MaxLevelComp = 0.f;
static const int WidthRect = 1;

//==============================================================================
//============================ LevelBox ================================
class LevelBox final : public juce::Component
{
public:
    LevelBox(LevelComponent & levelComponent, SmallGrisLookAndFeel & lookAndFeel);
    ~LevelBox() final = default;
    //==============================================================================
    void setBounds(const juce::Rectangle<int> & newBounds);
    void paint(juce::Graphics & g) final;
    void mouseDown(const juce::MouseEvent & e) final;
    void resetClipping();

private:
    //==============================================================================
    LevelComponent & levelComponent;
    SmallGrisLookAndFeel & lookAndFeel;

    juce::ColourGradient colorGrad;
    juce::Image vumeterBit;
    juce::Image vumeterBackBit;
    juce::Image vumeterMutedBit;

    bool isClipping = false;
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LevelBox);
};

//==============================================================================
//======================== LevelComponent ==============================
class LevelComponent final
    : public juce::Component
    , public juce::ToggleButton::Listener
    , public juce::ChangeListener
{
public:
    LevelComponent(ParentLevelComponent & parentLevelComponent,
                   SmallGrisLookAndFeel & lookAndFeel,
                   bool colorful = true);
    ~LevelComponent() final = default;
    //==============================================================================
    void setOutputLab(juce::String value) { this->idBut.setButtonText(value); }
    void setColor(juce::Colour color)
    {
        this->idBut.setColour(juce::TextButton::buttonColourId, color);
        this->repaint();
    }
    float getLevel() const { return level; }
    void update();
    bool isMuted() const { return this->muteToggleBut.getToggleState(); }
    void setSelected(bool value);
    void buttonClicked(juce::Button * button) final;
    void mouseDown(const juce::MouseEvent & e) final;
    void setBounds(const juce::Rectangle<int> & newBounds);
    void changeListenerCallback(juce::ChangeBroadcaster * source) final;
    void updateDirectOutMenu(juce::OwnedArray<Speaker> const & spkList);
    void resetClipping() { this->levelBox.resetClipping(); }
    //==============================================================================
    std::vector<int> directOutSpeakers;
    juce::TextButton directOut;

private:
    //==============================================================================
    ParentLevelComponent & parentLevelComponent;
    SmallGrisLookAndFeel & lookAndFeel;

    LevelBox levelBox;

    juce::TextButton idBut;
    juce::ToggleButton muteToggleBut;
    juce::ToggleButton soloToggleBut;

    float level = MinLevelComp;
    int lastMouseButton = 1; // 1 means left, 0 means right
    bool isColorful;
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LevelComponent)
};
