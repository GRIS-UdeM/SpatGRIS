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

#include "SpatModeComponent.hpp"

#include "Constants.hpp"
#include "GrisLookAndFeel.hpp"

static constexpr auto PREFERRED_WIDTH = 135;
static constexpr auto PREFERRED_HEIGHT = 75;
static constexpr auto LABEL_HEIGHT = 20;
static constexpr auto NUM_COLS = 2;
static constexpr auto NUM_ROWS = 2;
static constexpr auto INNER_PADDING = 1;

//==============================================================================
SpatModeComponent::SpatModeComponent(Listener & listener, GrisLookAndFeel & lookAndFeel)
    : mListener(listener)
    , mLabel("", "Algorithm Selection")
{
    using flag = juce::Button::ConnectedEdgeFlags;

    mLabel.setJustificationType(juce::Justification::centred);
    mLabel.setFont(lookAndFeel.getFont());
    mLabel.setColour(juce::Label::ColourIds::textColourId, lookAndFeel.getFontColour());
    addAndMakeVisible(mLabel);

    mButtons.ensureStorageAllocated(SPAT_MODE_STRINGS.size());

    for (int i{}; i < SPAT_MODE_STRINGS.size(); ++i) {
        auto * newButton{ mButtons.add(std::make_unique<juce::TextButton>(SPAT_MODE_STRINGS[i])) };
        newButton->setTooltip(SPAT_MODE_TOOLTIPS[i]);
        newButton->setClickingTogglesState(true);
        newButton->setRadioGroupId(SPAT_MODE_BUTTONS_RADIO_GROUP_ID, juce::dontSendNotification);
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
    if (!button->getToggleState()) {
        return;
    }
    auto const index{ mButtons.indexOf(button) };
    jassert(index >= 0);
    mSpatMode = narrow<SpatMode>(index);
    mListener.handleSpatModeChanged(mSpatMode);
}

//==============================================================================
void SpatModeComponent::resized()
{
    jassert(NUM_COLS * NUM_ROWS == mButtons.size());

    auto const width{ getWidth() };
    auto const height{ getHeight() };

    auto const yLabelOffset{ std::max(height - PREFERRED_HEIGHT, 0) / 2 };

    mLabel.setBounds(0, yLabelOffset, width, LABEL_HEIGHT);

    auto const totalButtonsWidth{ std::min(width, PREFERRED_WIDTH) };
    auto const totalButtonsHeight{ std::clamp(PREFERRED_HEIGHT - LABEL_HEIGHT, 0, height) };

    auto const xButtonsOffset{ std::max(width - totalButtonsWidth, 0) / 2 };
    auto const yButtonsOffset{ yLabelOffset + LABEL_HEIGHT };

    static constexpr auto TOTAL_X_PADDING{ INNER_PADDING * (NUM_COLS - 1) };
    static constexpr auto TOTAL_Y_PADDING{ INNER_PADDING * (NUM_ROWS - 1) };
    auto const buttonsWidth{ (totalButtonsWidth - TOTAL_X_PADDING) / NUM_COLS };
    auto const buttonsHeight{ (totalButtonsHeight - TOTAL_Y_PADDING) / NUM_ROWS };

    for (int i{}; i < NUM_ROWS; ++i) {
        auto const yOffset{ yButtonsOffset + i * (buttonsHeight + INNER_PADDING) };
        for (int j{}; j < NUM_COLS; ++j) {
            auto const xOffset{ xButtonsOffset + j * (buttonsWidth + INNER_PADDING) };
            auto const index{ i * NUM_COLS + j };
            juce::Rectangle<int> const bounds{ xOffset, yOffset, buttonsWidth, buttonsHeight };
            mButtons[index]->setBounds(bounds);
        }
    }
}

//==============================================================================
int SpatModeComponent::getMinWidth() const noexcept
{
    return PREFERRED_WIDTH;
}

//==============================================================================
int SpatModeComponent::getMinHeight() const noexcept
{
    return PREFERRED_HEIGHT;
}
