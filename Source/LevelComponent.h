/*
 This file is part of SpatGRIS2.

 Developers: Samuel Béland, Olivier Bélanger, Nicolas Masson

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

#include "Speaker.h"

static float constexpr MIN_LEVEL_COMP = -60.0f;
static float constexpr MAX_LEVEL_COMP = 0.0f;
static int constexpr WIDTH_RECT = 1;

class GrisLookAndFeel;
class LevelComponent;
class ParentLevelComponent;

//============================ LevelBox ================================
class LevelBox final : public juce::Component
{
    LevelComponent & mLevelComponent;
    SmallGrisLookAndFeel & mLookAndFeel;

    juce::ColourGradient mColorGrad;
    juce::Image mVuMeterBit;
    juce::Image mVuMeterBackBit;
    juce::Image mVuMeterMutedBit;

    bool mIsClipping = false;

public:
    //==============================================================================
    LevelBox(LevelComponent & levelComponent, SmallGrisLookAndFeel & lookAndFeel);
    //==============================================================================
    LevelBox() = delete;
    ~LevelBox() override = default;

    LevelBox(LevelBox const &) = delete;
    LevelBox(LevelBox &&) = delete;

    LevelBox & operator=(LevelBox const &) = delete;
    LevelBox & operator=(LevelBox &&) = delete;
    //==============================================================================
    void setBounds(const juce::Rectangle<int> & newBounds);
    void paint(juce::Graphics & g) override;
    void mouseDown(const juce::MouseEvent & e) override;
    void resetClipping();

private:
    //==============================================================================
    JUCE_LEAK_DETECTOR(LevelBox)
}; // class LevelBox

//======================== LevelComponent ==============================
class LevelComponent final
    : public juce::Component
    , public juce::ToggleButton::Listener
    , public juce::ChangeListener
{
    ParentLevelComponent & mParentLevelComponent;
    SmallGrisLookAndFeel & mLookAndFeel;

    LevelBox mLevelBox;

    juce::TextButton mIdButton;
    juce::ToggleButton mMuteToggleButton;
    juce::ToggleButton mSoloToggleButton;

    float mLevel = MIN_LEVEL_COMP;
    int mLastMouseButton = 1; // 1 means left, 0 means right
    bool mIsColorful;

public:
    //==============================================================================
    LevelComponent(ParentLevelComponent & parentLevelComponent,
                   SmallGrisLookAndFeel & lookAndFeel,
                   bool colorful = true);
    //==============================================================================
    LevelComponent() = delete;
    ~LevelComponent() override = default;

    LevelComponent(LevelComponent const &) = delete;
    LevelComponent(LevelComponent &&) = delete;

    LevelComponent & operator=(LevelComponent const &) = delete;
    LevelComponent & operator=(LevelComponent &&) = delete;
    //==============================================================================
    void setOutputLab(juce::String const & value) { this->mIdButton.setButtonText(value); }
    void setColor(juce::Colour color);
    float getLevel() const { return mLevel; }
    void update();
    bool isMuted() const { return this->mMuteToggleButton.getToggleState(); }
    void setSelected(bool value);
    void buttonClicked(juce::Button * button) override;
    void mouseDown(const juce::MouseEvent & e) override;
    void setBounds(const juce::Rectangle<int> & newBounds);
    void changeListenerCallback(juce::ChangeBroadcaster * source) override;
    void updateDirectOutMenu(juce::OwnedArray<Speaker> const & spkList);
    void resetClipping() { this->mLevelBox.resetClipping(); }
    //==============================================================================
    std::vector<int> directOutSpeakers;
    juce::TextButton directOut;

private:
    //==============================================================================
    JUCE_LEAK_DETECTOR(LevelComponent)
}; // class LevelComponent
