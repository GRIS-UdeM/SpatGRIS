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

#include "Box.h"

#include "GrisLookAndFeel.h"
#include "VuMeterComponent.h"

//==============================================================================
Box::Box(GrisLookAndFeel & feel,
         juce::String const & title,
         bool const verticalScrollbar,
         bool const horizontalScrollbar)
    : mLookAndFeel(feel)
    , mTitle(title)
{
    this->mViewport.setViewedComponent(&this->mContent, false);
    this->mViewport.setScrollBarsShown(verticalScrollbar, horizontalScrollbar);
    this->mViewport.setScrollBarThickness(15);
    this->mViewport.getVerticalScrollBar().setColour(juce::ScrollBar::ColourIds::thumbColourId,
                                                     feel.getScrollBarColour());
    this->mViewport.getHorizontalScrollBar().setColour(juce::ScrollBar::ColourIds::thumbColourId,
                                                       feel.getScrollBarColour());

    this->mViewport.setLookAndFeel(&feel);
    addAndMakeVisible(this->mViewport);
}

//==============================================================================
void Box::correctSize(int width, int const height)
{
    if (this->mTitle != "") {
        this->mViewport.setTopLeftPosition(0, 20);
        this->mViewport.setSize(getWidth(), getHeight() - 20);
        if (width < 80) {
            width = 80;
        }
    } else {
        this->mViewport.setTopLeftPosition(0, 0);
    }
    this->getContent()->setSize(width, height);
}

//==============================================================================
void Box::paint(juce::Graphics & g)
{
    g.setColour(this->mLookAndFeel.getBackgroundColour());
    g.fillRect(getLocalBounds());
    if (this->mTitle != "") {
        g.setColour(this->mLookAndFeel.getWinBackgroundColour());
        g.fillRect(0, 0, getWidth(), 18);
        g.setColour(this->mLookAndFeel.getFontColour());
        g.drawText(mTitle, 0, 0, this->mContent.getWidth(), 20, juce::Justification::left);
    }
}
