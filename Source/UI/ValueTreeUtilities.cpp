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

namespace gris
{

void copyValueTreeProperties (const juce::ValueTree& source, juce::ValueTree& dest)
{
    //these are the only types of sources we're expecting here
    jassert (source.getType ().toString ().contains("SPEAKER_")
             || source.getType().toString() == "POSITION"
             || source.getType ().toString () == "HIGHPASS");

    for (int i = 0; i < source.getNumProperties (); ++i) {
        const auto propertyName = source.getPropertyName (i);
        const auto propertyValue = source.getProperty (propertyName);
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

    // SPEAKER EXAMPLE
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

    // check if the version is already up to date
    if (oldSpeakerSetup.getProperty(VERSION) == CURRENT_SPEAKERSETUP_VERSION)
        return oldSpeakerSetup; 

    //create new value tree
    auto newVt = juce::ValueTree (SPEAKER_SETUP);

    //copy and update the root node
    copyValueTreeProperties (oldSpeakerSetup, newVt);
    newVt.setProperty (VERSION, CURRENT_SPEAKERSETUP_VERSION, nullptr);

    //TODO VB: we need to init this speaker group with a position and everything
    //create and append the main speaker group node
    auto mainSpeakerGroup = juce::ValueTree (SPEAKER_GROUP);
    mainSpeakerGroup.setProperty(ID, "Main", nullptr);
    newVt.appendChild (mainSpeakerGroup, nullptr);

    //then add all speakers to the main group
    for (const auto& speaker : oldSpeakerSetup)
    {
        if (! speaker.getType ().toString().contains ("SPEAKER_")
            || speaker.getChild(0).getType().toString() != "POSITION")
        {
            //corrupted file??
            jassertfalse;
            continue;   //should we continue or return here?
        }

        auto newSpeaker = juce::ValueTree { SPEAKER };
        const auto speakerId = speaker.getType ().toString ().removeCharacters ("SPEAKER_");
        newSpeaker.setProperty ("ID", speakerId, nullptr);

        // copy properties for the speaker and its children
        copyValueTreeProperties (speaker, newSpeaker);
        for (const auto child : speaker)
            copyValueTreeProperties (child, newSpeaker);

        mainSpeakerGroup.appendChild (newSpeaker, nullptr);
    }

    //DBG (newVt.toXmlString());

    return newVt;
}
}
