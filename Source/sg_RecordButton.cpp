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

#include "sg_RecordButton.hpp"

#include "Data/sg_Narrow.hpp"
#include "sg_GrisLookAndFeel.hpp"

namespace gris
{
namespace
{
constexpr auto LABEL_TIME_HEIGHT = 20;

constexpr auto BUTTON_SIZE = 55;
constexpr auto BLINK_PERIOD_MS = 500;
constexpr auto ACTIVE_TO_OUTER_CIRCLE_RATIO = 0.7f;
constexpr auto OUTER_TO_INNER_CIRCLE_RATIO = 0.7f;
constexpr auto LINE_THICKNESS = 3.0f;

auto const ACTIVE_COLOR{ juce::Colours::red };
auto const INACTIVE_COLOR{ juce::Colour::fromRGB(128, 0, 0) };

auto const BACKGROUND_COLOR{ juce::Colours::black.withAlpha(0.3f) };
auto const HOVER_BACKGROUND_COLOR{ juce::Colours::black.withAlpha(0.1f) };
auto const MOUSE_DOWN_BACKGROUND_COLOR{ juce::Colours::black.withAlpha(0.2f) };

} // namespace

//==============================================================================
RecordButton::RecordButton(Listener & listener, GrisLookAndFeel & glaf) : mListener(listener)
{
    JUCE_ASSERT_MESSAGE_THREAD;
    SettableTooltipClient::setTooltip("Record audio to disk");

    mRecordedTime.setJustificationType(juce::Justification::centred);
    mRecordedTime.setColour(juce::Label::ColourIds::textColourId, glaf.getFontColour());
    addChildComponent(mRecordedTime);
}

//==============================================================================
void RecordButton::setState(State const state)
{
    JUCE_ASSERT_MESSAGE_THREAD;

    mState = state;
    if (state == State::recording) {
        mTimeRecordingStarted = juce::Time::currentTimeMillis();
        updateRecordedTime();
        mRecordedTime.setVisible(true);
        startTimer(BLINK_PERIOD_MS);
    } else {
        jassert(state == State::ready);
        stopTimer();
        mRecordedTime.setVisible(false);
    }
    repaint();
}

//==============================================================================
void RecordButton::paint(juce::Graphics & g)
{
    JUCE_ASSERT_MESSAGE_THREAD;

    auto const getInnerColor = [&]() {
        if (mState == State::recording) {
            return mBlinkState ? ACTIVE_COLOR : INACTIVE_COLOR;
        }
        if (isMouseOver()) {
            return ACTIVE_COLOR;
        }
        return INACTIVE_COLOR;
    };

    auto const getBackgroundColor = [&]() {
        if (isMouseButtonDown()) {
            return MOUSE_DOWN_BACKGROUND_COLOR;
        }
        if (isMouseOver()) {
            return HOVER_BACKGROUND_COLOR;
        }
        return BACKGROUND_COLOR;
    };

    auto const activeRadius{ narrow<float>(mActiveBounds.getWidth()) / 2.0f };

    g.setColour(getBackgroundColor());
    g.fillRoundedRectangle(mActiveBounds.toFloat(), 10.0f);

    auto const outerRadius{ activeRadius * ACTIVE_TO_OUTER_CIRCLE_RATIO };
    auto const outerBounds{ mActiveBounds.toFloat().reduced(activeRadius - outerRadius) };
    g.setColour(ACTIVE_COLOR);
    g.drawEllipse(outerBounds, LINE_THICKNESS);

    auto const innerRadius{ outerRadius * OUTER_TO_INNER_CIRCLE_RATIO };
    auto const diff{ outerRadius - innerRadius };
    auto const innerBounds{ outerBounds.reduced(diff) };

    g.setColour(getInnerColor());
    g.fillEllipse(innerBounds);
}

//==============================================================================
void RecordButton::resized()
{
    JUCE_ASSERT_MESSAGE_THREAD;

    auto const height{ getHeight() };
    auto const maxY{ (height + BUTTON_SIZE) / 2 + LABEL_TIME_HEIGHT };

    auto const globalOffset{ std::min(height - maxY, 0) };

    auto const buttonX{ std::max(getWidth() - BUTTON_SIZE, 0) / 2 };
    auto const buttonY{ std::max(getHeight() - BUTTON_SIZE, 0) / 2 + globalOffset };

    auto const labelY{ buttonY + BUTTON_SIZE };

    mRecordedTime.setBounds(0, labelY, getWidth(), LABEL_TIME_HEIGHT);

    mActiveBounds = juce::Rectangle<int>{ buttonX, buttonY, BUTTON_SIZE, BUTTON_SIZE };
}

//==============================================================================
void RecordButton::mouseUp(juce::MouseEvent const & event)
{
    JUCE_ASSERT_MESSAGE_THREAD;
    if (mActiveBounds.contains(event.getMouseDownPosition())) {
        mListener.recordButtonPressed();
    }
}

//==============================================================================
void RecordButton::mouseMove(const juce::MouseEvent & /*event*/)
{
    JUCE_ASSERT_MESSAGE_THREAD;
    repaint();
}

//==============================================================================
void RecordButton::mouseExit(const juce::MouseEvent & /*event*/)
{
    JUCE_ASSERT_MESSAGE_THREAD;
    repaint();
}

//==============================================================================
int RecordButton::getMinWidth() const noexcept
{
    JUCE_ASSERT_MESSAGE_THREAD;
    return BUTTON_SIZE;
}

//==============================================================================
int RecordButton::getMinHeight() const noexcept
{
    JUCE_ASSERT_MESSAGE_THREAD;
    return BUTTON_SIZE + LABEL_TIME_HEIGHT;
}

//==============================================================================
void RecordButton::timerCallback()
{
    JUCE_ASSERT_MESSAGE_THREAD;
    mBlinkState = !mBlinkState;
    updateRecordedTime();
    repaint();
}

//==============================================================================
void RecordButton::updateRecordedTime()
{
    auto const elapsedSeconds{ (juce::Time::currentTimeMillis() - mTimeRecordingStarted) / 1000 };

    auto const elapsedMinutes{ elapsedSeconds / 60 };
    auto const remainingSeconds{ elapsedSeconds - elapsedMinutes * 60 };

    static auto const TO_PADDED_STRING = [](auto const value) {
        juce::String result{ value };
        jassert(result.length() == 1 || result.length() == 2);
        if (result.length() < 2) {
            result = "0" + result;
        }
        return result;
    };

    auto const minutes{ TO_PADDED_STRING(elapsedMinutes) };
    auto const seconds{ TO_PADDED_STRING(remainingSeconds) };

    auto const timeString{ minutes + ':' + seconds };
    mRecordedTime.setText(timeString, juce::dontSendNotification);
}

} // namespace gris
