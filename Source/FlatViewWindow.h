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

class MainContentComponent;
class GrisLookAndFeel;
class InputModel;

//==============================================================================
class FlatViewWindow final
    : public juce::DocumentWindow
    , private juce::Timer
{
    MainContentComponent & mMainContentComponent;
    GrisLookAndFeel & mLookAndFeel;

public:
    //==============================================================================
    FlatViewWindow(MainContentComponent & parent, GrisLookAndFeel & feel);
    //==============================================================================
    FlatViewWindow() = delete;
    ~FlatViewWindow() override = default;
    //==============================================================================
    FlatViewWindow(FlatViewWindow const &) = delete;
    FlatViewWindow(FlatViewWindow &&) = delete;
    FlatViewWindow & operator=(FlatViewWindow const &) = delete;
    FlatViewWindow & operator=(FlatViewWindow &&) = delete;
    //==============================================================================
    void timerCallback() override { this->repaint(); }
    void paint(juce::Graphics & g) override;
    void resized() override;
    void closeButtonPressed() override;

private:
    //==============================================================================
    void drawFieldBackground(juce::Graphics & g, int fieldSize) const;
    void drawSource(juce::Graphics & g, InputModel * input, int fieldSize) const;
    void drawSourceSpan(juce::Graphics & g, InputModel * it, int fieldWh, int fieldCenter) const;
    //==============================================================================
    JUCE_LEAK_DETECTOR(FlatViewWindow)
};
