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

#pragma once

#include "macros.h"

DISABLE_WARNINGS
#include <JuceHeader.h>
ENABLE_WARNINGS

class GrisLookAndFeel;
class AbstractVuMeterComponent;
class MainContentComponent;

//==============================================================================
class Box final : public juce::Component
{
    GrisLookAndFeel & mLookAndFeel;

    juce::Component mContent;
    juce::Viewport mViewport;
    juce::String mTitle;

public:
    //==============================================================================
    explicit Box(GrisLookAndFeel & feel,
                 juce::String const & title = "",
                 bool verticalScrollbar = false,
                 bool horizontalScrollbar = true);
    ~Box() override { this->mContent.deleteAllChildren(); }

    Box(Box const &) = delete;
    Box(Box &&) = delete;

    Box & operator=(Box const &) = delete;
    Box & operator=(Box &&) = delete;
    //==============================================================================
    [[nodiscard]] juce::Component * getContent() { return &this->mContent; }
    [[nodiscard]] juce::Component const * getContent() const { return &this->mContent; }
    [[nodiscard]] juce::Viewport * getViewport() { return &mViewport; }

    void resized() override { this->mViewport.setSize(this->getWidth(), this->getHeight()); }
    void correctSize(int width, int const height);
    void paint(juce::Graphics & g) override;

private:
    //==============================================================================
    JUCE_LEAK_DETECTOR(Box)
};
