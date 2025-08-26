/*
 This file is part of SpatGRIS.

 Developers: Gaël Lane Lépine, Samuel Béland, Olivier Bélanger, Nicolas Masson

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

#include "sg_GeneralMuteButton.h"

#include "sg_GrisLookAndFeel.hpp"

namespace gris
{
namespace
{
constexpr auto LABEL_MUTE_HEIGHT = 20;

constexpr auto BUTTON_SIZE = 55;
constexpr auto BLINK_PERIOD_MS = 500;

auto const ACTIVE_COLOR{ juce::Colour::fromRGB(255, 165, 25) };
auto const INACTIVE_COLOR{ juce::Colour::fromRGB(46, 46, 46) };

auto const BACKGROUND_COLOR{ juce::Colours::black.withAlpha(0.3f) };
auto const HOVER_BACKGROUND_COLOR{ juce::Colours::black.withAlpha(0.1f) };
auto const MOUSE_DOWN_BACKGROUND_COLOR{ juce::Colours::black.withAlpha(0.2f) };

} // namespace

//==============================================================================
GeneralMuteButton::GeneralMuteButton(Listener & listener, GrisLookAndFeel & lookAndFeel)
    : mLookAndFeel(lookAndFeel)
    , mListener(listener)
{
    JUCE_ASSERT_MESSAGE_THREAD;
    SettableTooltipClient::setTooltip("Mute or Unmute all speakers");
}

//==============================================================================
void GeneralMuteButton::setState(State state)
{
    JUCE_ASSERT_MESSAGE_THREAD;

    mState = state;
    if (state == State::allMuted) {
        startTimer(BLINK_PERIOD_MS);
    } else {
        jassert(state == State::allUnmuted);
        stopTimer();
        mBlinkState = false;
    }
    repaint();
}

//==============================================================================
void GeneralMuteButton::paint(juce::Graphics & g)
{
    JUCE_ASSERT_MESSAGE_THREAD;

    auto const getInnerColor = [&]() {
        if (mState == State::allMuted) {
            return mBlinkState ? ACTIVE_COLOR : INACTIVE_COLOR;
        }
        if (mActiveBounds.contains(getMouseXYRelative())) {
            return ACTIVE_COLOR;
        }
        return INACTIVE_COLOR;
    };

    auto const getBackgroundColor = [&]() {
        if (isMouseButtonDown()) {
            return MOUSE_DOWN_BACKGROUND_COLOR;
        }
        if (mActiveBounds.contains(getMouseXYRelative())) {
            return HOVER_BACKGROUND_COLOR;
        }
        return BACKGROUND_COLOR;
    };

    g.setColour(getBackgroundColor());
    g.fillRoundedRectangle(mActiveBounds.toFloat(), 10.0f);

    g.setColour(ACTIVE_COLOR.withAlpha(0.5f));
    g.drawRoundedRectangle(mActiveBounds.toFloat().reduced(7.0f), 8.0f, 4.0f);

    g.setColour(getInnerColor());
    auto const outerRingBounds{ mActiveBounds.toFloat().reduced(7.0f) };
    g.fillRoundedRectangle(outerRingBounds.toFloat(), 8.0f);

    auto const fontColor{ getInnerColor() == ACTIVE_COLOR ? mLookAndFeel.getDarkColour()
                                                          : mLookAndFeel.getFontColour() };
    g.setColour(fontColor);
    g.drawFittedText("MUTE", mActiveBounds, juce::Justification::centred, 1);
}

//==============================================================================
void GeneralMuteButton::resized()
{
    JUCE_ASSERT_MESSAGE_THREAD;

    auto const height{ getHeight() };
    auto const maxY{ (height + BUTTON_SIZE) / 2 + LABEL_MUTE_HEIGHT };

    auto const globalOffset{ std::min(height - maxY, 0) };

    auto const buttonX{ std::max(getWidth() - BUTTON_SIZE, 0) / 2 };
    auto const buttonY{ std::max(getHeight() - BUTTON_SIZE, 0) / 2 + globalOffset };

    mActiveBounds = juce::Rectangle<int>{ buttonX, buttonY, BUTTON_SIZE, BUTTON_SIZE };
}

//==============================================================================
void GeneralMuteButton::mouseUp(juce::MouseEvent const & event)
{
    JUCE_ASSERT_MESSAGE_THREAD;

    if (mActiveBounds.contains(event.getMouseDownPosition())) {
        mListener.generalMuteButtonPressed();
    }
}

//==============================================================================
void GeneralMuteButton::mouseMove(const juce::MouseEvent & /*event*/)
{
    JUCE_ASSERT_MESSAGE_THREAD;
    repaint();
}

//==============================================================================
void GeneralMuteButton::mouseExit(const juce::MouseEvent & /*event*/)
{
    JUCE_ASSERT_MESSAGE_THREAD;
    repaint();
}

//==============================================================================
int GeneralMuteButton::getMinWidth() const noexcept
{
    JUCE_ASSERT_MESSAGE_THREAD;
    return BUTTON_SIZE;
}

//==============================================================================
int GeneralMuteButton::getMinHeight() const noexcept
{
    JUCE_ASSERT_MESSAGE_THREAD;
    return BUTTON_SIZE + LABEL_MUTE_HEIGHT;
}

//==============================================================================
GeneralMuteButton::State GeneralMuteButton::getGeneralMuteButtonState()
{
    return mState;
}

//==============================================================================
void GeneralMuteButton::timerCallback()
{
    JUCE_ASSERT_MESSAGE_THREAD;

    mBlinkState = !mBlinkState;
    repaint();
}

} // namespace gris
