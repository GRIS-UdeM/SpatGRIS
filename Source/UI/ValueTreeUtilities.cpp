/*
  ==============================================================================

    ValueTreeUtilities.cpp
    Created: 18 Apr 2025 10:55:33am
    Author:  barth

  ==============================================================================
*/

#include "ValueTreeUtilities.hpp"

namespace gris
{
//TODO VB: put somewhere central
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

    if (oldSpeakerSetup.getType ().toString () != "SPEAKER_SETUP")
    {
        jassertfalse;
        return {};
    }

    const auto version = "4.0.0";
    auto newVt = juce::ValueTree ("SPEAKER_SETUP");

    //copy and update the root node
    copyValueTreeProperties (oldSpeakerSetup, newVt);
    newVt.setProperty ("VERSION", version, nullptr);

    //then do the same with the children
    for (const auto& speaker : oldSpeakerSetup)
    {
        auto newSpeaker = juce::ValueTree { SPEAKER };
        const auto speakerId = speaker.getType ().toString ().removeCharacters ("SPEAKER_");
        newSpeaker.setProperty ("ID", speakerId, nullptr);

        //copy properties for the speaker and its position child into the newSpeaker
        copyValueTreeProperties (speaker, newSpeaker);
        copyValueTreeProperties (speaker.getChild (0), newSpeaker);

        newVt.appendChild (newSpeaker, nullptr);
    }

    //DBG (newVt.toXmlString());

    return newVt;
}
}
