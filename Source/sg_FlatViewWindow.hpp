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

#include "sg_LogicStrucs.hpp"

#include <JuceHeader.h>

namespace gris
{
class MainContentComponent;
class GrisLookAndFeel;
class InputModel;

//==============================================================================
class FlatViewWindow final
    : public juce::DocumentWindow
    , juce::Timer
{
    MainContentComponent & mMainContentComponent;
    GrisLookAndFeel & mLookAndFeel;
    StrongArray<source_index_t, ViewportSourceDataUpdater, MAX_NUM_SOURCES> mSourceDataQueues{};
    StrongArray<source_index_t, ViewportSourceDataUpdater::Token *, MAX_NUM_SOURCES> mLastSourceData{};

public:
    //==============================================================================
    FlatViewWindow(MainContentComponent & parent, GrisLookAndFeel & feel);
    //==============================================================================
    FlatViewWindow() = delete;
    ~FlatViewWindow() override = default;
    SG_DELETE_COPY_AND_MOVE(FlatViewWindow)
    //==============================================================================
    auto & getSourceDataQueues() { return mSourceDataQueues; }
    //==============================================================================
    void timerCallback() override { this->repaint(); }
    void paint(juce::Graphics & g) override;
    void resized() override;
    void closeButtonPressed() override;

private:
    //==============================================================================
    void drawFieldBackground(juce::Graphics & g) const;
    void drawSource(juce::Graphics & g,
                    source_index_t sourceIndex,
                    ViewportSourceData const & sourceData,
                    SpatMode spatMode) const;
    void drawSourceVbapSpan(juce::Graphics & g, ViewportSourceData const & sourceData) const;
    void drawSourceMbapSpan(juce::Graphics & g,
                            ViewportSourceData const & sourceData,
                            juce::Point<float> const & sourcePositionAbsolute) const;
    //==============================================================================
    JUCE_LEAK_DETECTOR(FlatViewWindow)
};

} // namespace gris