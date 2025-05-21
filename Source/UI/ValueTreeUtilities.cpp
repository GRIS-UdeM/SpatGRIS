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

#include "ValueTreeUtilities.hpp"
#include <Data/sg_Position.hpp>

namespace gris
{

/** This simply copies properties from one valuetree to another, nothing else. */
void copyProperties (const juce::ValueTree& source, juce::ValueTree& dest)
{
    //these are the only types of sources we're expecting here
    jassert (source.getType ().toString ().contains("SPEAKER_")
             || source.getType().toString() == CartesianVector::XmlTags::POSITION
             || source.getType ().toString () == "HIGHPASS");

    if (source.getType ().toString () == CartesianVector::XmlTags::POSITION) {
        auto const x = source[CartesianVector::XmlTags::X];
        auto const y = source[CartesianVector::XmlTags::Y];
        auto const z = source[CartesianVector::XmlTags::Z];

        dest.setProperty(CARTESIAN_POSITION,
                         juce::VariantConverter<Position>::toVar(Position{ CartesianVector{ x, y, z } }),
                         nullptr);

        return;
    }

    for (int i = 0; i < source.getNumProperties (); ++i) {
        auto const propertyName = source.getPropertyName (i);
        auto const propertyValue = source.getProperty (propertyName);
        dest.setProperty (propertyName, propertyValue, nullptr);
    }
};

//TODO VB: this will need unit tests -- and can this be constexpr
juce::ValueTree convertSpeakerSetup (const juce::ValueTree& oldSpeakerSetup)
{
    // DOME EXAMPLE
    /*<SPEAKER_SETUP VERSION = "3.1.14" SPAT_MODE = "Dome" DIFFUSION = "0.0" GENERAL_MUTE = "0">
        <SPEAKER_1 STATE = "normal" GAIN = "0.0" DIRECT_OUT_ONLY = "0">
            <POSITION X = "-4.371138828673793e-8" Y = "1.0" Z = "-4.371138828673793e-8" / >
        </SPEAKER_1>
        <SPEAKER_2 STATE = "normal" GAIN = "0.0" DIRECT_OUT_ONLY = "0">
            <POSITION X = "0.0980171337723732" Y = "0.9951847195625305" Z = "-4.371138828673793e-8" / >
        </SPEAKER_2>
    */

    // CUBE EXAMPLE
    /*<SPEAKER_SETUP VERSION = "3.1.14" SPAT_MODE = "Cube" DIFFUSION = "0.0" GENERAL_MUTE = "0">
        <SPEAKER_1 STATE = "normal" GAIN = "0.0" DIRECT_OUT_ONLY = "0">
            <POSITION X = "-0.7071083784103394" Y = "0.7071067690849304" Z = "-9.478120688299896e-8" / >
            <HIGHPASS FREQ = "60.0" / >
        </SPEAKER_1>
        <SPEAKER_2 STATE = "normal" GAIN = "0.0" DIRECT_OUT_ONLY = "0">
            <POSITION X = "0.7071067094802856" Y = "0.7071068286895752" Z = "-9.478120688299896e-8" / >
            <HIGHPASS FREQ = "60.0" / >
        </SPEAKER_2>
    */

    //TODO VB: handle default speaker setup and all legacy types
    if (oldSpeakerSetup.getType () != SPEAKER_SETUP)
    {
        //this function should  probably return an optional and here it should be empty
        //jassertfalse;
        DBG (oldSpeakerSetup.toXmlString());
        //so I don't know how many forking versions of speaker setups there are but this function deals with one of them. probably better to use that forking function and create a VT from the speaker setup data?
        // or maybe/probably there's already a function that does this
        //readLegacySpeakerSetup
        return {};
    }

    //DBG (oldSpeakerSetup.toXmlString ());

    // get outta here if the version is already up to date
    if (oldSpeakerSetup.getProperty(VERSION) == CURRENT_SPEAKER_SETUP_VERSION)
        return oldSpeakerSetup; 

    //create new value tree and copy root properties into it
    auto newVt = juce::ValueTree (SPEAKER_SETUP);
    copyProperties (oldSpeakerSetup, newVt);
    newVt.setProperty (VERSION, CURRENT_SPEAKER_SETUP_VERSION, nullptr);

    //create and append the main speaker group node
    auto mainSpeakerGroup = juce::ValueTree(SPEAKER_GROUP);
    mainSpeakerGroup.setProperty(ID, "Main", nullptr);
    mainSpeakerGroup.setProperty(CARTESIAN_POSITION, juce::VariantConverter<Position>::toVar(Position{}), nullptr);
    newVt.appendChild(mainSpeakerGroup, nullptr);

    //then add all speakers to the main group
    for (const auto& speaker : oldSpeakerSetup)
    {
        if (! speaker.getType ().toString().contains ("SPEAKER_")
            || speaker.getChild(0).getType().toString() != "POSITION")
        {
            //corrupted file? Speakers must have a type that starts with SPEAKER_ and have a POSITION child
            jassertfalse;
            continue;   //should we continue or return here?
        }

        auto newSpeaker = juce::ValueTree { SPEAKER };
        const auto speakerId = speaker.getType ().toString ().removeCharacters ("SPEAKER_");
        newSpeaker.setProperty ("ID", speakerId, nullptr);

        // copy properties for the speaker and its children
        copyProperties (speaker, newSpeaker);
        for (const auto child : speaker)
            copyProperties (child, newSpeaker);

        mainSpeakerGroup.appendChild (newSpeaker, nullptr);
    }

    //DBG (newVt.toXmlString());
    return newVt;
}

juce::ValueTree getTopParent (const juce::ValueTree& vt)
{
    auto parent = vt.getParent();
    while (parent.isValid()) {
        if (parent.getParent().isValid())
            parent = parent.getParent();
        else
            break;
    }
    return parent;
}
}
