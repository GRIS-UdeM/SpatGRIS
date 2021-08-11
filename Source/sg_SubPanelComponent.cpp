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

#include "sg_SubPanelComponent.hpp"

#include "sg_GrisLookAndFeel.hpp"

static constexpr auto CORNER_SIZE = 8;
static constexpr auto INNER_PADDING = 8;

static constexpr auto OFFSET{ INNER_PADDING / 2 };

//==============================================================================
SubPanelComponent::SubPanelComponent(LayoutComponent::Orientation const orientation, GrisLookAndFeel & lookAndFeel)
    : mLookAndFeel(lookAndFeel)
    , mLayout(orientation, false, false, lookAndFeel)
{
    addAndMakeVisible(mLayout);
}

//==============================================================================
void SubPanelComponent::resized()
{
    mLayout.setBounds(OFFSET, OFFSET, getWidth() - INNER_PADDING, getHeight() - INNER_PADDING);
}

//==============================================================================
void SubPanelComponent::paint(juce::Graphics & g)
{
    g.setColour(mLookAndFeel.getWinBackgroundColour());
    auto const layoutBounds{ mLayout.getBoundsInParent().expanded(OFFSET, OFFSET).toFloat() };
    g.fillRoundedRectangle(layoutBounds.getX(),
                           layoutBounds.getY(),
                           layoutBounds.getWidth(),
                           layoutBounds.getHeight(),
                           CORNER_SIZE);
}

//==============================================================================
int SubPanelComponent::getMinWidth() const noexcept
{
    return mLayout.getMinWidth() + INNER_PADDING;
}

//==============================================================================
int SubPanelComponent::getMinHeight() const noexcept
{
    return mLayout.getMinHeight() + INNER_PADDING;
}
