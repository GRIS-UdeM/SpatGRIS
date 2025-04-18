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

namespace gris
{
class SpeakerTreeComponent : public juce::Component
{
public:
    SpeakerTreeComponent (const juce::ValueTree& v);

    void resized () override
    {
        auto bounds { getLocalBounds () };
        constexpr auto colW { 60 };

        for (auto* component : { &id, &x, &y, &z, &azim, &elev, &distance, &gain, &highpass, &direct, &del })
            component->setBounds (bounds.removeFromLeft (colW));
    }

protected:
    void setupEditor (juce::Label& editor, juce::StringRef text)
    {
        editor.setText (text, juce::dontSendNotification);
        editor.setEditable (true);
        addAndMakeVisible (editor);
    };

    juce::Label id, x, y, z, azim, elev, distance, gain, highpass, direct, del;

    juce::ValueTree vt;
};

//==============================================================================

class SpeakerGroupComponent : public SpeakerTreeComponent
{
public:
    SpeakerGroupComponent (const juce::ValueTree& v);
};

//==============================================================================

class SpeakerComponent : public SpeakerTreeComponent
{
public:
    SpeakerComponent (const juce::ValueTree& v);
};
}
