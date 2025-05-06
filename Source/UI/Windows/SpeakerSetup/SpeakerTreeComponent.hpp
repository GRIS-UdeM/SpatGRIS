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
class SpeakerTreeComponent : public juce::Component
{
public:
    SpeakerTreeComponent (juce::TreeViewItem* owner, const juce::ValueTree& v);
    ~SpeakerTreeComponent () { setLookAndFeel (nullptr); }

    void paint(juce::Graphics & g) override;

    void resized () override;

protected:
    void setupEditor (juce::Label& editor, juce::StringRef text);;

    juce::Label id, x, y, z, azim, elev, distance, gain, highpass, direct, del, drag;

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
