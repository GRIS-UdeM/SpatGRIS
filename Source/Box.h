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

#ifndef UICOMPONENT_H
#define UICOMPONENT_H

#include <iostream>

#include "../JuceLibraryCode/JuceHeader.h"

#include "GrisLookAndFeel.h"
#include "JackClientGRIS.h"

class LevelComponent;
class MainContentComponent;

//==============================================================================
class Box final : public Component
{
public:
    Box(GrisLookAndFeel &    feel,
        juce::String const & title = "",
        bool                 verticalScrollbar = false,
        bool                 horizontalScrollbar = true);
    ~Box() final { this->content.deleteAllChildren(); }
    //==============================================================================
    Component *       getContent() { return &this->content; }
    Component const * getContent() const { return &this->content; }

    void resized() final { this->viewport.setSize(this->getWidth(), this->getHeight()); }
    void correctSize(unsigned int width, unsigned int height);
    void paint(Graphics & g) final;

private:
    //==============================================================================
    GrisLookAndFeel & grisFeel;

    juce::Component content;
    juce::Viewport  viewport;
    // juce::Colour      bgColour;
    juce::String title;
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Box)
};

#endif /* UICOMPONENT_H */
