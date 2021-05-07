#include "SpatModeComponent.h"

#include "constants.hpp"

//==============================================================================
SpatModeComponent::SpatModeComponent(Listener & listener) : mListener(listener)
{
    mButtons.ensureStorageAllocated(SPAT_MODE_STRINGS.size());
    for (auto const & spatModeString : SPAT_MODE_STRINGS) {
        auto * newButton{ mButtons.add(std::make_unique<juce::TextButton>(spatModeString)) };
        newButton->setClickingTogglesState(true);
        newButton->setRadioGroupId(SPAT_MODE_BUTTONS_RADIO_GROUP_ID, juce::dontSendNotification);
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
    static constexpr auto NUM_COLS = 2;
    static constexpr auto NUM_ROWS = 2;
    jassert(NUM_COLS * NUM_ROWS == mButtons.size());

    auto const buttonWidth{ getWidth() / NUM_COLS };
    auto const buttonHeight{ getHeight() / NUM_ROWS };

    for (int i{}; i < NUM_ROWS; ++i) {
        for (int j{}; j < NUM_COLS; ++j) {
            auto const index{ i * NUM_COLS + j };
            juce::Rectangle<int> const bounds{ j * buttonWidth, i * buttonHeight, buttonWidth, buttonHeight };
            mButtons[index]->setBounds(bounds);
        }
    }
}
