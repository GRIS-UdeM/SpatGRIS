#include "SpatModeComponent.h"

#include "constants.hpp"

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

    auto const buttonWidth{ getWidth() / NUM_COLS + SPACING_TO_REMOVE * 2 };
    auto const buttonHeight{ getHeight() / NUM_ROWS + SPACING_TO_REMOVE * 2 };

    for (int i{}; i < NUM_ROWS; ++i) {
        auto const yOffset{ SPACING_TO_REMOVE + i * 2 * SPACING_TO_REMOVE };
        for (int j{}; j < NUM_COLS; ++j) {
            auto const xOffset{ SPACING_TO_REMOVE + j * 2 * SPACING_TO_REMOVE };
            auto const index{ i * NUM_COLS + j };
            juce::Rectangle<int> const bounds{ j * buttonWidth - xOffset,
                                               i * buttonHeight - yOffset,
                                               buttonWidth,
                                               buttonHeight };
            mButtons[index]->setBounds(bounds);
        }
    }
}
