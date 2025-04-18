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

#include "SpeakerTreeComponent.hpp"
namespace gris
{
SpeakerTreeComponent::SpeakerTreeComponent (const juce::ValueTree& v)
    : vt (v)
{
    setInterceptsMouseClicks (false, true);

    setupEditor (x, juce::String::formatted ("%.2f", static_cast<float>(v["X"])));
    setupEditor (y, juce::String::formatted ("%.2f", static_cast<float>(v["Y"])));
    setupEditor (z, juce::String::formatted ("%.2f", static_cast<float>(v["Z"])));
    setupEditor (azim, "need convert");
    setupEditor (elev, "need convert");
    setupEditor (distance, "is this radius?");
    setupEditor (del, "DEL");
}

//==============================================================================

SpeakerGroupComponent::SpeakerGroupComponent (const juce::ValueTree& v) : SpeakerTreeComponent (v)
{
    setupEditor (id, v.getType ().toString ());
}

//==============================================================================

SpeakerComponent::SpeakerComponent (const juce::ValueTree& v) : SpeakerTreeComponent (v)
{
    setupEditor (id, v["ID"].toString ());
    setupEditor (gain, v["GAIN"].toString ());
    setupEditor (highpass, "wtf?");
    setupEditor (direct, v["DIRECT_OUT_ONLY"].toString ());
}
}