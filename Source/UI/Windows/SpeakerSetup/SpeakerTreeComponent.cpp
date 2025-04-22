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
    setupEditor (azim, "convert");
    setupEditor (elev, "convert");
    setupEditor (distance, "radius?");
    setupEditor (del, "DEL");
}

void SpeakerTreeComponent::resized ()
{
    constexpr auto fixedLeftColWidth { 200 };
    constexpr auto otherColWidth { 60 } ;

    if (auto* window = findParentComponentOfClass<juce::DocumentWindow> ())
    {
        auto bounds = getLocalBounds ();

        // position the ID colum so it is a fixed width of fixedLeftColWidth fromt the left of the document window
        const auto windowOriginInThis = window->getLocalPoint (this, juce::Point<int>{ 0, 0 });
        const auto idColWidth = fixedLeftColWidth - windowOriginInThis.x;
        id.setBounds (bounds.removeFromLeft (idColWidth));

        // then position the other components with a fixed width of otherColWidth
        for (auto* component : { &x, &y, &z, &azim, &elev, &distance, &gain, &highpass, &direct, &del })
            component->setBounds (bounds.removeFromLeft (otherColWidth));
    }
}

void SpeakerTreeComponent::setupEditor (juce::Label& editor, juce::StringRef text)
{
    editor.setText (text, juce::dontSendNotification);
    editor.setEditable (true);
    addAndMakeVisible (editor);
}

//==============================================================================

SpeakerGroupComponent::SpeakerGroupComponent (const juce::ValueTree& v) : SpeakerTreeComponent (v)
{
    setupEditor (id, v[ID].toString ());
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