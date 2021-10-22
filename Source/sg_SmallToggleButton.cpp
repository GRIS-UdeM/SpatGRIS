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

#include "sg_SmallToggleButton.hpp"

#include "sg_GrisLookAndFeel.hpp"

//==============================================================================
SmallToggleButton::SmallToggleButton(juce::String const & text, Listener & listener, SmallGrisLookAndFeel & lookAndFeel)
    : mListener(listener)
    , mLookAndFeel(lookAndFeel)
    , mLabel("", text)
{
    auto const initColors = [&](Component & component) {
        component.setLookAndFeel(&lookAndFeel);
        component.setColour(juce::Label::textColourId, lookAndFeel.getFontColour());
        component.setColour(juce::TextButton::textColourOnId, lookAndFeel.getFontColour());
        component.setColour(juce::TextButton::textColourOffId, lookAndFeel.getFontColour());
        component.setColour(juce::TextButton::buttonColourId, lookAndFeel.getBackgroundColour());
        addAndMakeVisible(component);
    };

    mButton.setClickingTogglesState(true);
    mButton.addListener(this);
    initColors(mButton);

    mLabel.setJustificationType(juce::Justification::centred);
    mLabel.setInterceptsMouseClicks(false, false);
    initColors(mLabel);
    mLabel.setFont(juce::Font{ 0.8f });
}

//==============================================================================
void SmallToggleButton::setToggleState(bool const state)
{
    mButton.setToggleState(state, juce::dontSendNotification);
}

//==============================================================================
int SmallToggleButton::getMinWidth() const noexcept
{
    return 0;
}

//==============================================================================
int SmallToggleButton::getMinHeight() const noexcept
{
    return 15;
}

//==============================================================================
void SmallToggleButton::resized()
{
    auto const localBounds{ getLocalBounds() };
    mButton.setBounds(localBounds);
    mLabel.setBounds(localBounds.withSizeKeepingCentre(100, 100));
}

//==============================================================================
void SmallToggleButton::buttonClicked([[maybe_unused]] juce::Button * button)
{
    mListener.smallButtonClicked(this, button->getToggleState());
}
