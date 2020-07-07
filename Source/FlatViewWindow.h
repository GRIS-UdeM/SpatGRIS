/*
 This file is part of SpatGRIS2.

 Developers: Nicolas Masson

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

#ifndef FLATVIEWWINDOW_H
#define FLATVIEWWINDOW_H

#include "../JuceLibraryCode/JuceHeader.h"

class MainContentComponent;
class GrisLookAndFeel;
class Input;

//==============================================================================
class FlatViewWindow final
    : public DocumentWindow
    , private Timer
{
public:
    FlatViewWindow(MainContentComponent & parent, GrisLookAndFeel & feel);
    ~FlatViewWindow() final;
    //==============================================================================
    void timerCallback() final { this->repaint(); }
    void paint(Graphics & g) final;
    void resized() final;
    void closeButtonPressed() final;

private:
    //==============================================================================
    void drawFieldBackground(Graphics & g, const int fieldWH) const;
    void drawSource(Graphics & g, Input * it, const int fieldWH) const;
    void drawSourceSpan(Graphics & g, Input * it, const int fieldWH, const int fieldCenter) const;
    //==============================================================================
    MainContentComponent & mainParent;
    GrisLookAndFeel &      grisFeel;
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FlatViewWindow);
};

#endif /* FLATVIEWWINDOW_H */
