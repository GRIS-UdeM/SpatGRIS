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
#include "../../ValueTreeUtilities.hpp"
#include "../../../sg_GrisLookAndFeel.hpp"

namespace gris
{

class DraggableLabel : public juce::Label
{
public:
    std::function<void (int)> onMouseDragCallback;

private:
    int lastMouseY = 0; // Track the last Y position of the mouse

    void mouseDown (const juce::MouseEvent& event) override
    {
        lastMouseY = event.getPosition ().getY (); // Initialize the last Y position on mouse down
    }

    void mouseDrag (const juce::MouseEvent& event) override
    {
        //probably should be using this, not sure it needs a buffered position 
        //event.getDistanceFromDragStartY()

        int currentMouseY = event.getPosition ().getY ();
        int deltaY = currentMouseY - lastMouseY; // Calculate the delta
        lastMouseY = currentMouseY; // Update the last Y position

        if (onMouseDragCallback)
            onMouseDragCallback (deltaY); // Pass the delta to the callback
    }
};

//==============================================================================

class SpeakerTreeComponent : public juce::Component
{
public:
    SpeakerTreeComponent (juce::TreeViewItem* owner, const juce::ValueTree& v);
    ~SpeakerTreeComponent () { setLookAndFeel (nullptr); }

    void paint(juce::Graphics & g) override;

    void resized () override;

protected:
    void setupEditor (DraggableLabel& editor, juce::Value value);
    void setupEditor (juce::Label& editor, juce::StringRef text);

    DraggableLabel id, x, y, z, azim, elev, distance, gain, highpass, direct, del, drag;

    juce::ValueTree vt;

    GrisLookAndFeel lnf;
    juce::TreeViewItem* treeViewItem;
};

//==============================================================================

class SpeakerGroupComponent : public SpeakerTreeComponent
{
public:
    SpeakerGroupComponent (juce::TreeViewItem* owner, const juce::ValueTree& v);
};

//==============================================================================

class SpeakerComponent : public SpeakerTreeComponent
{
public:
    SpeakerComponent (juce::TreeViewItem* owner, const juce::ValueTree& v);
};
}
