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

    setupEditor (x, vt.getPropertyAsValue ("X", nullptr));
    setupEditor (y, vt.getPropertyAsValue ("Y", nullptr));
    setupEditor (z, vt.getPropertyAsValue ("Z", nullptr));

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

void SpeakerTreeComponent::setupEditor (DraggableLabel& editor, juce::Value value)
{
    editor.getTextValue ().referTo (value);
    editor.setEditable (true);
    addAndMakeVisible (editor);
    editor.setInterceptsMouseClicks(true, false);
    editor.onMouseDragCallback = [this, &editor](int deltaY)
        {
            //there's all this logic too
            //if (spatMode == SpatMode::mbap || isDirectOutOnly) {
            //    return Position { position.getCartesian ().clampedToFarField () };
            //}

            //if (modifiedCol == Col::AZIMUTH || modifiedCol == Col::ELEVATION) {
            //    return position.normalized ();
            //}

        //static auto const clampCartesian = [](const float valueModified,
        //                                      juce::Value valueToAdjust,
        //                                      juce::Value valueToTryToKeepIntact) {
        //    float const valueModified2{ std::powf (valueModified, 2) };
        //        auto const lengthWithoutValueToAdjust{ valueModified2
        //                                               + static_cast<float>(valueToTryToKeepIntact.getValue())
        //                                                     * static_cast<float>(valueToTryToKeepIntact.getValue()) };

        //    if (lengthWithoutValueToAdjust > 1.0f) {
        //        auto const sign{ valueToTryToKeepIntact < 0.0f ? -1.0f : 1.0f };
        //        auto const length{ std::sqrt(1.0f - valueModified2) };
        //        valueToTryToKeepIntact = sign * length;
        //        valueToAdjust = 0.0f;
        //        return;
        //    }

        //    auto const sign{ valueToAdjust < 0.0f ? -1.0f : 1.0f };
        //    auto const length{ std::sqrt(1.0f - lengthWithoutValueToAdjust) };
        //    valueToAdjust = sign * length;
        //};

            auto currentValue = editor.getText ().getFloatValue ();
            auto newValue = currentValue - deltaY * 0.1f; // Adjust sensitivity as needed
            //newValue = std::clamp (newValue, -1.0f, 1.0f);

        //    if (&editor == &x)
        //        clampCartesian(newValue, y.getTextValue(), z.getTextValue ());
        //    else if (&editor == &y)
        //        clampCartesian (newValue, x.getTextValue (), z.getTextValue ());
        //    else if (&editor == &z)
        //        clampCartesian (newValue, x.getTextValue (), y.getTextValue ());
        //    else
        //        jassertfalse;
        //    //else if (editor == azim)
        //    //    clampCartesian(newValue, elev, distance);
        //    //else if (editor == elev)
        //    //    clampCartesian(newValue, azim, distance);
        //    //else if (editor == distance)


            editor.setText (juce::String (newValue, 3), juce::dontSendNotification);
            editor.getTextValue ().setValue (newValue);
        };

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
    setupEditor (id, vt.getPropertyAsValue ("ID", nullptr));
}

//==============================================================================

SpeakerComponent::SpeakerComponent (juce::TreeViewItem* owner, const juce::ValueTree& v) : SpeakerTreeComponent (owner, v)
{
    // TODO VB: this is super weird because all these components belong to the parent. We probably need some virtual
    // function to call in resized where we get the list of components that are in children
    setupEditor (id, vt.getPropertyAsValue ("ID", nullptr));
    setupEditor (gain, vt.getPropertyAsValue ("GAIN", nullptr));
    setupEditor (highpass, vt.getPropertyAsValue ("FREQ", nullptr));
    setupEditor (direct, vt.getPropertyAsValue ("DIRECT_OUT_ONLY", nullptr));
}
}