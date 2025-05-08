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
#include <Data/StrongTypes/sg_CartesianVector.hpp>
#include <Data/sg_Position.hpp>

namespace gris
{
SpeakerTreeComponent::SpeakerTreeComponent (juce::TreeViewItem* owner, const juce::ValueTree& v)
    : vt (v)
    , treeViewItem (owner)
{
    //setLookAndFeel(&lnf);
    setInterceptsMouseClicks (false, true);

    //TODO VB: serialize to string and back to position in the VT instead of constructing it like this
    auto const position = Position{ CartesianVector(v["X"], v["Y"], v["Z"]) };
    //auto const & cartesian{ position.getCartesian() };
    auto const & polar{ position.getPolar() };

    //TODO VB: use undomanager
    setupEditor (x, X);
    setupEditor (y, Y);
    setupEditor (z, Z);

    //TODO VB: all these will need some different logic from the above setupEditor
    setupEditor (azim, juce::String (polar.azimuth.get(), 3));
    setupEditor (elev, juce::String (polar.elevation.get(), 3));
    setupEditor (distance, juce::String (polar.length, 3));
    setupEditor (del, "DEL");
    setupEditor (drag, "=");

    drag.setEditable(false);
    drag.setInterceptsMouseClicks (false, false);
}

void SpeakerTreeComponent::paint (juce::Graphics& g)
{
    if (treeViewItem->isSelected ())
        g.fillAll (lnf.mHlBgcolor);
    else if (vt.getType() == SPEAKER_GROUP)
        g.fillAll (lnf.mBackGroundAndFieldColour.darker(.5f));
    else if (treeViewItem->getIndexInParent () % 2 == 0)
        g.fillAll (lnf.mGreyColour);
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
        for (auto* component : { &x, &y, &z, &azim, &elev, &distance, &gain, &highpass, &direct, &del, &drag })
            component->setBounds (bounds.removeFromLeft (otherColWidth));
    }
}

void SpeakerTreeComponent::setupEditor(DraggableLabel & label, juce::Identifier identifier)
{
    label.setEditable(true);

    // Show the initial value with 3 decimals
    label.setText(juce::String(static_cast<float> (vt[identifier]), 3), juce::dontSendNotification);

    // Create and hold the listener to sync value -> label
    auto * listener = new ValueToLabelListener(vt.getPropertyAsValue (identifier, nullptr), label);
    valueListeners.add(listener);

    label.onTextChange = [this, &label, identifier] {
        auto newValue = label.getText().getFloatValue();
        vt.setProperty(identifier, newValue, nullptr);
        label.setText(juce::String(newValue, 3), juce::dontSendNotification);
    };

    label.onMouseDragCallback = [this, &label, identifier](int deltaY) {
        auto currentValue = label.getText().getFloatValue();
        auto newValue = currentValue - deltaY * 0.01f;
        vt.setProperty (identifier, newValue, nullptr);
        label.setText(juce::String(newValue, 3), juce::dontSendNotification);
    };

    addAndMakeVisible(label);
}

void SpeakerTreeComponent::setupEditor (juce::Label& editor, juce::StringRef text)
{
    editor.setText (text, juce::dontSendNotification);
    editor.setEditable (true);
    addAndMakeVisible (editor);
}

//==============================================================================

SpeakerGroupComponent::SpeakerGroupComponent (juce::TreeViewItem* owner, const juce::ValueTree& v) : SpeakerTreeComponent (owner, v)
{
    setupEditor (id, ID);
}

//==============================================================================

SpeakerComponent::SpeakerComponent (juce::TreeViewItem* owner, const juce::ValueTree& v) : SpeakerTreeComponent (owner, v)
{
    // TODO VB: this is super weird because all these components belong to the parent. We probably need some virtual
    // function to call in resized where we get the list of components that are in children
    setupEditor (id, ID);
    setupEditor (gain, GAIN);
    setupEditor (highpass, FREQ);
    setupEditor (direct, DIRECT_OUT_ONLY);
}
}