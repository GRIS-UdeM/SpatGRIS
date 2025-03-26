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

#pragma once

#include "AlgoGRIS/Data/sg_Macros.hpp"

#include <JuceHeader.h>

namespace gris
{
class GrisLookAndFeel;
class AbstractSliceComponent;
class MainContentComponent;

/** @brief A custom component class for displaying content within a viewport.
 *
 *  The Box class is juce::Component that has a title and wraps another Component. It is used to display content
 *  within a scrollable viewport. The class manages a content component and a viewport, and
 *  provides methods to access these components. It also handles resizing and painting of the
 *  component. The Box class is useful for displaying large content that may not fit within the
 *  visible area of the parent component, allowing the user to scroll through the content.
 */
class Box final : public juce::Component
{
    GrisLookAndFeel & mLookAndFeel;

    Component mContent;
    juce::Viewport mViewport;
    juce::String mTitle;

public:
    //==============================================================================
    explicit Box(GrisLookAndFeel & feel,
                 juce::String title = "",
                 bool verticalScrollbar = false,
                 bool horizontalScrollbar = true);
    ~Box() override { this->mContent.deleteAllChildren(); }
    SG_DELETE_COPY_AND_MOVE(Box)
    //==============================================================================
    [[nodiscard]] Component * getContent() { return &this->mContent; }
    [[nodiscard]] Component const * getContent() const { return &this->mContent; }
    [[nodiscard]] juce::Viewport * getViewport() { return &mViewport; }

    void resized() override { this->mViewport.setSize(this->getWidth(), this->getHeight()); }
    void correctSize(int width, int height);
    void paint(juce::Graphics & g) override;

private:
    //==============================================================================
    JUCE_LEAK_DETECTOR(Box)
};

} // namespace gris
