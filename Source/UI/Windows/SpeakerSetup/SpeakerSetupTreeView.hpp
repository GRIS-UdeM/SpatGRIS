/*
 This file is part of SpatGRIS.

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

#include "JuceHeader.h"


namespace gris
{

class SpeakerSetupTreeView final
        : public juce::TreeView
{
  public:
    SpeakerSetupTreeView(const juce::String &componentName={});

    /**
     * This requests the component to set its viewport's scrollbar to a certain position
     * at the end of its paint method. I did this because I was running into timing issues
     * where the resetOpennessState method tried to reset the scrollbar before the viewport
     * was actually scrollable.
     */
    void requestScrollReset(double scrollPosition);
    /**
     * This generally just calls the treeview's paint but if requestScrollReset was called
     * it will also try to move the scrollbar.
     *
     * It will try 10 (or 11 ?) times to change the position to the requested position. It
     * will consider the request honoured if the scrollbar moved in anyway, if the requested position is 0.0 or if it tries too many times.
     */
    void paint (juce::Graphics & g);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SpeakerSetupTreeView)

  private:
    bool scrollResetRequested = false;
    double requestedScrollPosition = 0.0;
    unsigned int tries = 0;

    /**
     * sets all the scroll reset properties to their defalt values.
     */
    void resetScrollRequest();
};

}
