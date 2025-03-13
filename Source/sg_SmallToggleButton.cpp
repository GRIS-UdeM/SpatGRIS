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

#include "sg_SmallToggleButton.hpp"

#include "sg_GrisLookAndFeel.hpp"

namespace gris
{
//==============================================================================
SmallToggleButton::SmallToggleButton(bool const isToggle,
                                     juce::String const & text,
                                     juce::String const & toolTip,
                                     Listener & listener,
                                     SmallGrisLookAndFeel & lookAndFeel)
    : mListener(listener)

    , mLabel("", text)
    , mButton("", toolTip)
{
    JUCE_ASSERT_MESSAGE_THREAD;

    auto const initColors = [&](Component & component) {
        component.setLookAndFeel(&lookAndFeel);
        component.setColour(juce::Label::textColourId, lookAndFeel.getFontColour());
        component.setColour(juce::TextButton::textColourOnId, lookAndFeel.getFontColour());
        component.setColour(juce::TextButton::textColourOffId, lookAndFeel.getFontColour());
        component.setColour(juce::TextButton::buttonColourId, lookAndFeel.getBackgroundColour());
        addAndMakeVisible(component);
    };

    mButton.setClickingTogglesState(isToggle);
    mButton.addMouseListener(this, false);
    initColors(mButton);

    mLabel.setJustificationType(juce::Justification::centred);
    mLabel.setInterceptsMouseClicks(false, false);
    initColors(mLabel);

    mLabel.setFont(juce::FontOptions().withHeight(.8f));
}

//==============================================================================
juce::Colour SmallToggleButton::getButtonColor() const
{
    return mButton.findColour(juce::TextButton::buttonColourId);
}

//==============================================================================
void SmallToggleButton::setToggleState(bool const state)
{
    JUCE_ASSERT_MESSAGE_THREAD;

    mButton.setToggleState(state, juce::dontSendNotification);
}

//==============================================================================
void SmallToggleButton::setButtonColor(int const colorId, juce::Colour const colour)
{
    JUCE_ASSERT_MESSAGE_THREAD;

    mButton.setColour(colorId, colour);
}

//==============================================================================
void SmallToggleButton::setLabelColour(int const colorId, juce::Colour const colour)
{
    JUCE_ASSERT_MESSAGE_THREAD;

    mLabel.setColour(colorId, colour);
}

//==============================================================================
int SmallToggleButton::getMinWidth() const noexcept
{
    JUCE_ASSERT_MESSAGE_THREAD;

    return 0;
}

//==============================================================================
int SmallToggleButton::getMinHeight() const noexcept
{
    JUCE_ASSERT_MESSAGE_THREAD;

    return 15;
}

//==============================================================================
void SmallToggleButton::resized()
{
    JUCE_ASSERT_MESSAGE_THREAD;

    auto const localBounds{ getLocalBounds() };
    mButton.setBounds(localBounds);
    mLabel.setBounds(localBounds.withSizeKeepingCentre(100, 100));
}

//==============================================================================
void SmallToggleButton::mouseUp(const juce::MouseEvent & event)
{
    JUCE_ASSERT_MESSAGE_THREAD;

    if (!mButton.getBounds().contains(event.getPosition())) {
        return;
    }

    auto const isLeftButton{ event.mods.isLeftButtonDown() };
    auto const isRightButton{ event.mods.isRightButtonDown() };

    if (!isLeftButton && !isRightButton) {
        return;
    }

    mListener.smallButtonClicked(this, mButton.getToggleState(), isLeftButton);
}

} // namespace gris
