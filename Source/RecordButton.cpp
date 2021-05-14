#include "RecordButton.h"

#include "narrow.hpp"

static constexpr auto MIN_SIZE = 50;
static constexpr auto MAX_SIZE = 100;
static constexpr auto BLINK_PERIOD_MS = 500;
static constexpr auto ACTIVE_TO_OUTER_CIRCLE_RATIO = 0.7f;
static constexpr auto OUTER_TO_INNER_CIRCLE_RATIO = 0.7f;
static constexpr auto LINE_THICKNESS = 3.0f;

static auto const COLOR_1{ juce::Colours::red };
static auto const COLOR_2{ juce::Colour::fromRGB(128, 0, 0) };

//==============================================================================
RecordButton::RecordButton(Listener & listener) : mListener(listener)
{
    JUCE_ASSERT_MESSAGE_THREAD;
    SettableTooltipClient::setTooltip("Record audio to disk");
}

//==============================================================================
void RecordButton::setState(State const state)
{
    JUCE_ASSERT_MESSAGE_THREAD;

    mState = state;
    if (state == State::recording) {
        startTimer(BLINK_PERIOD_MS);
    } else {
        jassert(state == State::ready);
        stopTimer();
    }
    repaint();
}

//==============================================================================
void RecordButton::paint(juce::Graphics & g)
{
    JUCE_ASSERT_MESSAGE_THREAD;

    auto const getInnerColor = [&]() { return mState == State::recording && mBlinkState ? COLOR_2 : COLOR_1; };

    auto const getActiveBoundsColor = [&]() {
        static auto const NOTHING = juce::Colours::transparentBlack;
        static auto const OVER = juce::Colours::black.withAlpha(0.5f);
        static auto const MOUSE_DOWN = juce::Colours::black.withAlpha(0.1f);

        if (isMouseButtonDown()) {
            return MOUSE_DOWN;
        }
        if (isMouseOver()) {
            return OVER;
        }
        return NOTHING;
    };

    auto const activeRadius{ narrow<float>(mActiveBounds.getWidth()) / 2.0f };
    auto const outerRadius{ activeRadius * ACTIVE_TO_OUTER_CIRCLE_RATIO };

    auto const outerBounds{ mActiveBounds.toFloat().reduced(activeRadius - outerRadius) };
    g.setColour(COLOR_1);
    g.drawEllipse(outerBounds, LINE_THICKNESS);

    auto const innerRadius{ outerRadius * OUTER_TO_INNER_CIRCLE_RATIO };
    auto const diff{ outerRadius - innerRadius };
    auto const innerBounds{ outerBounds.reduced(diff) };

    g.setColour(getInnerColor());
    g.fillEllipse(innerBounds);

    g.setColour(getActiveBoundsColor());
    g.fillRect(mActiveBounds);
}

//==============================================================================
void RecordButton::resized()
{
    JUCE_ASSERT_MESSAGE_THREAD;
    updateActiveBounds();
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
int RecordButton::getMinWidth() const noexcept
{
    JUCE_ASSERT_MESSAGE_THREAD;
    return MIN_SIZE;
}

//==============================================================================
int RecordButton::getMinHeight() const noexcept
{
    JUCE_ASSERT_MESSAGE_THREAD;
    return MIN_SIZE;
}

//==============================================================================
void RecordButton::timerCallback()
{
    JUCE_ASSERT_MESSAGE_THREAD;
    mBlinkState = !mBlinkState;
    repaint();
}

//==============================================================================
void RecordButton::updateActiveBounds()
{
    JUCE_ASSERT_MESSAGE_THREAD;

    auto const smallerSide{ std::min(getWidth(), getHeight()) };

    auto const size{ std::clamp(smallerSide, MIN_SIZE, MAX_SIZE) };
    auto const x{ std::max(getWidth() - size, 0) / 2 };
    auto const y{ std::max(getHeight() - size, 0) / 2 };

    mActiveBounds = juce::Rectangle<int>{ x, y, size, size };
}
