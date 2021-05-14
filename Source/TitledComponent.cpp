/*
  ==============================================================================

    TitledComponent.cpp
    Created: 13 May 2021 9:37:58pm
    Author:  samue

  ==============================================================================
*/

#include "TitledComponent.h"

#include "GrisLookAndFeel.h"

//==============================================================================
TitledComponent::TitledComponent(juce::String title,
                                 MinSizedComponent * contentComponent,
                                 GrisLookAndFeel & lookAndFeel)
    : mTitle(std::move(title))
    , mContentComponent(contentComponent)
    , mLookAndFeel(lookAndFeel)
{
    addAndMakeVisible(mContentComponent);
}

//==============================================================================
void TitledComponent::resized()
{
    auto const availableHeight{ std::max(getHeight() - TITLE_HEIGHT, 0) };
    juce::Rectangle<int> const contentBounds{ 0, TITLE_HEIGHT, getWidth(), availableHeight };
    mContentComponent->setBounds(contentBounds);
}

//==============================================================================
void TitledComponent::paint(juce::Graphics & g)
{
    g.setColour(mLookAndFeel.getBackgroundColour());
    g.fillRect(getLocalBounds());
    if (mTitle != "") {
        g.setColour(mLookAndFeel.getWinBackgroundColour());
        g.fillRect(0, 0, getWidth(), TITLE_HEIGHT);
        g.setColour(mLookAndFeel.getFontColour());
        g.drawText(mTitle, 0, 0, getWidth(), TITLE_HEIGHT + 2, juce::Justification::left);
    }
}