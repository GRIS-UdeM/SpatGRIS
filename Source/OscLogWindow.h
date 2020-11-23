/*
 This file is part of SpatGRIS2.

 Developers: Samuel Béland, Olivier Bélanger, Nicolas Masson

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

#pragma once

#include "macros.h"

DISABLE_WARNINGS
#include <JuceHeader.h>
ENABLE_WARNINGS

class GrisLookAndFeel;
class MainContentComponent;

//==============================================================================
class OscLogWindow final
    : public juce::DocumentWindow
    , public juce::TextButton::Listener
{
    MainContentComponent * mMainContentComponent;
    GrisLookAndFeel * mLookAndFeel;

    int mIndex;
    bool mActivated;

    juce::CodeDocument mCodeDocument;
    juce::CodeEditorComponent mLogger;
    juce::TextButton mStopButton;
    juce::TextButton mCloseButton;

public:
    //==============================================================================
    OscLogWindow(juce::String const & name,
                 juce::Colour backgroundColour,
                 int buttonsNeeded,
                 MainContentComponent * parent,
                 GrisLookAndFeel * feel);
    //==============================================================================
    OscLogWindow() = delete;
    ~OscLogWindow() override;

    OscLogWindow(OscLogWindow const &) = delete;
    OscLogWindow(OscLogWindow &&) = delete;

    OscLogWindow & operator=(OscLogWindow const &) = delete;
    OscLogWindow & operator=(OscLogWindow &&) = delete;
    //==============================================================================
    void addToLog(juce::String msg);
    //==============================================================================
    void buttonClicked(juce::Button * button) override;
    void closeButtonPressed() override;

private:
    //==============================================================================
    JUCE_LEAK_DETECTOR(OscLogWindow)
};
