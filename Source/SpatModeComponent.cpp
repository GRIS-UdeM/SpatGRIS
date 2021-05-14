#include "SpatModeComponent.h"

#include "constants.hpp"

static constexpr auto MIN_WIDTH = 150;
static constexpr auto MAX_WIDTH = 200;
static constexpr auto MIN_HEIGHT = 75;
static constexpr auto MAX_HEIGHT = 100;

//==============================================================================
SpatModeComponent::SpatModeComponent(Listener & listener) : mListener(listener)
{
    using flag = juce::Button::ConnectedEdgeFlags;

    static auto const getFlags = [](int const index) {
        static_assert(NUM_ROWS == 2 && NUM_COLS == 2);

        int result{};

        switch (result / NUM_COLS) {
        case 0:
            result |= flag::ConnectedOnBottom;
            break;
        case 1:
            result |= flag::ConnectedOnTop;
            break;
        default:
            jassertfalse;
        }

        switch (result % NUM_ROWS) {
        case 0:
            result |= flag::ConnectedOnRight;
            break;
        case 1:
            result |= flag::ConnectedOnLeft;
            break;
        default:
            jassertfalse;
        }

        return result;
    };

    mButtons.ensureStorageAllocated(SPAT_MODE_STRINGS.size());

    for (int i{}; i < SPAT_MODE_STRINGS.size(); ++i) {
        auto * newButton{ mButtons.add(std::make_unique<juce::TextButton>(SPAT_MODE_STRINGS[i])) };
        newButton->setTooltip(SPAT_MODE_TOOLTIPS[i]);
        newButton->setClickingTogglesState(true);
        newButton->setRadioGroupId(SPAT_MODE_BUTTONS_RADIO_GROUP_ID, juce::dontSendNotification);
        newButton->setConnectedEdges(getFlags(i));
        newButton->addListener(this);
        addAndMakeVisible(newButton);
    }

    mButtons.getFirst()->setToggleState(true, juce::dontSendNotification);
}

//==============================================================================
void SpatModeComponent::setSpatMode(SpatMode const spatMode)
{
    if (spatMode == mSpatMode) {
        return;
    }

    mSpatMode = spatMode;
    auto const index{ narrow<int>(spatMode) };
    jassert(index >= 0 && index <= SPAT_MODE_STRINGS.size());
    mButtons[index]->setToggleState(true, juce::dontSendNotification);
}

//==============================================================================
void SpatModeComponent::buttonClicked(juce::Button * button)
{
    auto const index{ mButtons.indexOf(button) };
    jassert(index >= 0);
    mSpatMode = narrow<SpatMode>(index);
    mListener.handleSpatModeChanged(mSpatMode);
}

//==============================================================================
void SpatModeComponent::resized()
{
    jassert(NUM_COLS * NUM_ROWS == mButtons.size());

    auto const width{ std::clamp(getWidth(), MIN_WIDTH, MAX_WIDTH) };
    auto const height{ std::clamp(getHeight(), MIN_HEIGHT, MAX_HEIGHT) };

    auto const baseOffsetX{ std::max(getWidth() - width, 0) / 2 };
    auto const baseOffsetY{ std::max(getHeight() - height, 0) / 2 };

    auto const totalXPadding{ INNER_PADDING * (NUM_COLS - 1) };
    auto const totalYPadding{ INNER_PADDING * (NUM_ROWS - 1) };

    auto const buttonWidth{ (width - totalXPadding) / NUM_COLS };
    auto const buttonHeight{ (height - totalYPadding) / NUM_ROWS };

    for (int i{}; i < NUM_ROWS; ++i) {
        auto const yOffset{ baseOffsetY + i * (buttonHeight + INNER_PADDING) };
        for (int j{}; j < NUM_COLS; ++j) {
            auto const xOffset{ baseOffsetX + j * (buttonWidth + INNER_PADDING) };
            auto const index{ i * NUM_COLS + j };
            juce::Rectangle<int> const bounds{ xOffset, yOffset, buttonWidth, buttonHeight };
            mButtons[index]->setBounds(bounds);
        }
    }
}

//==============================================================================
int SpatModeComponent::getMinWidth() const noexcept
{
    return MIN_WIDTH;
}

//==============================================================================
int SpatModeComponent::getMinHeight() const noexcept
{
    return MIN_HEIGHT;
}
