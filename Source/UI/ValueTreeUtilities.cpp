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
    for (int i = 0; i < source.getNumProperties (); ++i) {
        const auto propertyName = source.getPropertyName (i);
        const auto propertyValue = source.getProperty (propertyName);
        dest.setProperty (propertyName, propertyValue, nullptr);
    }
};

//TODO VB: this will need unit tests -- and can this be constexpr
juce::ValueTree convertSpeakerSetup (const juce::ValueTree& oldSpeakerSetup)
{
    //<SPEAKER_SETUP VERSION = "3.1.14" SPAT_MODE = "Dome" DIFFUSION = "0.0" GENERAL_MUTE = "0">
    // 
    //  <SPEAKER_1 STATE = "normal" GAIN = "0.0" DIRECT_OUT_ONLY = "0">
    //      <POSITION X = "-4.371138828673793e-8" Y = "1.0" Z = "-4.371138828673793e-8" / >
    //  </SPEAKER_1>
    //  <SPEAKER_2 STATE = "normal" GAIN = "0.0" DIRECT_OUT_ONLY = "0">
    //      <POSITION X = "0.0980171337723732" Y = "0.9951847195625305" Z = "-4.371138828673793e-8" / >

    if (oldSpeakerSetup.getType ()!= SPEAKER_SETUP)
    {
        //this function should  probably return an optional and here it should be empty
        jassertfalse;
        return {};
    }

    //create new value tree
    auto newVt = juce::ValueTree (SPEAKER_SETUP);

    //copy and update the root node
    copyValueTreeProperties (oldSpeakerSetup, newVt);
    newVt.setProperty (VERSION, CURRENT_SPEAKERSETUP_VERSION, nullptr);

    //TODO VB: we need to init this speaker group with a position and everything
    //create and append the main speaker group node
    auto mainSpeakerGroup = juce::ValueTree (SPEAKER_GROUP);
    mainSpeakerGroup.setProperty(NAME, "Main", nullptr);
    newVt.appendChild (mainSpeakerGroup, nullptr);

    //adding a couple of random group now to test this feature
#if 1
    auto randomSpeakerGroup1 = juce::ValueTree (SPEAKER_GROUP);
    randomSpeakerGroup1.setProperty (NAME, "Audiodice 2", nullptr);
    mainSpeakerGroup.appendChild (randomSpeakerGroup1, nullptr);

    auto randomSpeakerGroup = juce::ValueTree (SPEAKER_GROUP);
    randomSpeakerGroup.setProperty (NAME, "Audiodice 1", nullptr);
    mainSpeakerGroup.appendChild (randomSpeakerGroup, nullptr);
#endif

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

        //copy properties for the speaker and its position child into the newSpeaker
        copyValueTreeProperties (speaker, newSpeaker);
        copyValueTreeProperties (speaker.getChild (0), newSpeaker);

        mainSpeakerGroup.appendChild (newSpeaker, nullptr);
    }

    //DBG (newVt.toXmlString());

    return newVt;
}
}
