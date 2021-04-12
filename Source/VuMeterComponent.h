/*
 This file is part of SpatGRIS.

 Developers: Samuel Béland, Olivier Bélanger, Nicolas Masson

 SpatGRIS is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 SpatGRIS is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with SpatGRIS.  If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

#include <JuceHeader.h>

#include "GrisLookAndFeel.h"
#include "LogicStrucs.hpp"
#include "StrongTypes.hpp"

static dbfs_t constexpr MIN_LEVEL_COMP{ -60.0f };
static dbfs_t constexpr MAX_LEVEL_COMP{ 0.0f };
static int constexpr WIDTH_RECT = 1;

class GrisLookAndFeel;
class VuMeterComponent;
class VuMeterModel;

enum class MouseButton { right, left };

//============================ LevelBox ================================
class VuMeterBox final : public juce::Component
{
    static constexpr auto WIDTH = 22;
    static constexpr auto HEIGHT = 140;

    VuMeterComponent & mLevelComponent;
    SmallGrisLookAndFeel & mLookAndFeel;

    juce::ColourGradient mColorGrad;
    juce::Image mVuMeterBit;
    juce::Image mVuMeterBackBit;
    juce::Image mVuMeterMutedBit;

    bool mIsClipping = false;

public:
    //==============================================================================
    VuMeterBox(VuMeterComponent & levelComponent, SmallGrisLookAndFeel & lookAndFeel);
    //==============================================================================
    VuMeterBox() = delete;
    ~VuMeterBox() override = default;

    VuMeterBox(VuMeterBox const &) = delete;
    VuMeterBox(VuMeterBox &&) = delete;

    VuMeterBox & operator=(VuMeterBox const &) = delete;
    VuMeterBox & operator=(VuMeterBox &&) = delete;
    //==============================================================================
    void setBounds(const juce::Rectangle<int> & newBounds);
    void paint(juce::Graphics & g) override;
    void mouseDown(const juce::MouseEvent & e) override;
    void resetClipping();

private:
    //==============================================================================
    JUCE_LEAK_DETECTOR(VuMeterBox)
}; // class LevelBox

//======================== LevelComponent ==============================
class VuMeterComponent final
    : public juce::Component
    , public juce::ToggleButton::Listener
    , public juce::ChangeListener
{
    VuMeterModel & mModel;
    SmallGrisLookAndFeel & mLookAndFeel;

    VuMeterBox mContainerBox;

    juce::TextButton mIdButton;
    juce::ToggleButton mMuteToggleButton;
    juce::ToggleButton mSoloToggleButton;

    dbfs_t mLevel{ MIN_LEVEL_COMP };
    MouseButton mLastMouseButton{ MouseButton::right };
    bool mIsColorful;

    juce::TextButton mDirectOutButton;

    SpeakersData const & mSpeakersData;

public:
    //==============================================================================
    VuMeterComponent(VuMeterModel & model,
                     SmallGrisLookAndFeel & lookAndFeel,
                     SpeakersData const & speakersData,
                     bool colorful);
    //==============================================================================
    VuMeterComponent() = delete;
    ~VuMeterComponent() override = default;

    VuMeterComponent(VuMeterComponent const &) = delete;
    VuMeterComponent(VuMeterComponent &&) = delete;

    VuMeterComponent & operator=(VuMeterComponent const &) = delete;
    VuMeterComponent & operator=(VuMeterComponent &&) = delete;
    //==============================================================================
    void setOutputLab(juce::String const & value) { this->mIdButton.setButtonText(value); }
    void setColor(juce::Colour color);
    [[nodiscard]] dbfs_t getLevel() const { return mLevel; }
    void update();
    [[nodiscard]] bool isMuted() const { return this->mMuteToggleButton.getToggleState(); }
    void setSelected(bool value);
    void resetClipping() { this->mContainerBox.resetClipping(); }
    void setLevel(dbfs_t level);

    [[nodiscard]] juce::TextButton & getDirectOutButton() { return mDirectOutButton; }
    [[nodiscard]] juce::TextButton const & getDirectOutButton() const { return mDirectOutButton; }
    //==============================================================================
    // overrides
    void buttonClicked(juce::Button * button) override;
    void mouseDown(const juce::MouseEvent & e) override;
    void setBounds(const juce::Rectangle<int> & newBounds);
    void changeListenerCallback(juce::ChangeBroadcaster * source) override;

private:
    //==============================================================================
    JUCE_LEAK_DETECTOR(VuMeterComponent)
}; // class LevelComponent
