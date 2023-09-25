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

#include "sg_TitledComponent.hpp"

#include "sg_GrisLookAndFeel.hpp"

namespace gris
{
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

} // namespace gris
