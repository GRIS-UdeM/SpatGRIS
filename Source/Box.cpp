/*
 This file is part of SpatGRIS2.

 Developers: Olivier Belanger, Nicolas Masson

 SpatGRIS2 is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 SpatGRIS2 is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with SpatGRIS2.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "Box.h"

#include <algorithm>

#include "LevelComponent.h"
#include "MainComponent.h"
#include "ServerGrisConstants.h"
#include "Speaker.h"

//==============================================================================
static double GetFloatPrecision(double value, double precision)
{
    return (floor((value * pow(10, precision) + 0.5)) / pow(10, precision));
}

//==============================================================================
Box::Box(GrisLookAndFeel * feel, String title, bool verticalScrollbar, bool horizontalScrollbar)
{
    this->title = title;
    this->grisFeel = feel;
    this->bgColour = this->grisFeel->getBackgroundColour();

    this->content = new Component();
    this->viewport = new Viewport();
    this->viewport->setViewedComponent(this->content, false);
    this->viewport->setScrollBarsShown(verticalScrollbar, horizontalScrollbar);
    this->viewport->setScrollBarThickness(15);
    this->viewport->getVerticalScrollBar().setColour(ScrollBar::ColourIds::thumbColourId, feel->getScrollBarColour());
    this->viewport->getHorizontalScrollBar().setColour(ScrollBar::ColourIds::thumbColourId, feel->getScrollBarColour());

    this->viewport->setLookAndFeel(this->grisFeel);
    addAndMakeVisible(this->viewport);
}

//==============================================================================
Box::~Box()
{
    this->content->deleteAllChildren();
    delete this->viewport;
    delete this->content;
}

//==============================================================================
void Box::resized()
{
    if (this->viewport) {
        this->viewport->setSize(getWidth(), getHeight());
    }
}

//==============================================================================
void Box::correctSize(unsigned int width, unsigned int height)
{
    if (this->title != "") {
        this->viewport->setTopLeftPosition(0, 20);
        this->viewport->setSize(getWidth(), getHeight() - 20);
        if (width < 80) {
            width = 80;
        }
    } else {
        this->viewport->setTopLeftPosition(0, 0);
    }
    this->getContent()->setSize(width, height);
}

//==============================================================================
void Box::paint(Graphics & g)
{
    g.setColour(this->bgColour);
    g.fillRect(getLocalBounds());
    if (this->title != "") {
        g.setColour(this->grisFeel->getWinBackgroundColour());
        g.fillRect(0, 0, getWidth(), 18);
        g.setColour(this->grisFeel->getFontColour());
        g.drawText(title, 0, 0, this->content->getWidth(), 20, juce::Justification::left);
    }
}
