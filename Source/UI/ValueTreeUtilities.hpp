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
#include <JuceHeader.h>

namespace gris
{
/**
 * Copies all properties (and no children or children properties) from the source to the destination ValueTree.
 *
 * @param source The source ValueTree containing the properties to copy.
 * @param dest The destination ValueTree where the properties will be copied to.
 */
void copyProperties(const juce::ValueTree & source, juce::ValueTree & dest);

/**
 * Converts an old speaker setup ValueTree to the CURRENT_SPEAKER_SETUP_VERSION.
 *
 * @param oldSpeakerSetup The ValueTree representing the old speaker setup.
 * @return A new ValueTree representing the converted speaker setup.
 *
 * Here are examples of all the existing speaker setup versions:
    3.1.14 Resources/templates/Speaker setups/CUBE/Cube_default_speaker_setup.xml
    3.1.14 Resources/templates/Speaker setups/DOME/Dome_default_speaker_setup.xml

    3.2.11 Resources/templates/Speaker setups/CUBE/Cube0(0)Subs0.xml
    3.2.11 Resources/templates/Speaker setups/DOME/Dome0(0)Subs0.xml

    3.2.3 Resources/templates/Speaker setups/CUBE/Cube12(3X4)Subs2 Centres.xml

    3.2.5 Resources/templates/Speaker setups/DOME/Dome13(9-4)Subs2 Bremen.xml

    3.2.9 Resources/templates/Speaker setups/DOME/Dome8(4-4)Subs1 ZiMMT Small Studio.xml

    3.3.0 Resources/templates/Speaker setups/CUBE/Cube93(32-32-16-8-4-1)Subs5 Satosphere.xml
    3.3.0 Resources/templates/Speaker setups/DOME/Dome93(32-32-16-8-4-1)Subs5 Satosphere.xml

    3.3.5 Resources/templates/Speaker setups/CUBE/Cube26(8-8-6-2-2)Subs3 Lisbonne.xml
    3.3.5 Resources/templates/Speaker setups/DOME/Dome61(29-11-14-7)Subs0 Brahms.xml

    3.3.6 Resources/templates/Speaker setups/CUBE/Cube24(8-8-8)Subs2 Studio PANaroma.xml
    3.3.6 Resources/templates/Speaker setups/DOME/Dome20(8-6-4-2)Sub4 Lakefield Icosa.xml

    3.3.7 Resources/templates/Speaker setups/DOME/Dome32(4X8)Subs4 SubMix.xml
 */
juce::ValueTree convertSpeakerSetup(const juce::ValueTree & oldSpeakerSetup);

juce::ValueTree getTopParent(const juce::ValueTree & vt);

// Speaker setup identifiers
const auto CURRENT_SPEAKER_SETUP_VERSION = "4.0.0";
const juce::Identifier SPAT_MODE{ "SPAT_MODE" };
const juce::Identifier SPEAKER_SETUP{ "SPEAKER_SETUP" };
const juce::Identifier SPEAKER_GROUP{ "SPEAKER_GROUP" };
const juce::Identifier SPEAKER{ "SPEAKER" };
const juce::Identifier VERSION{ "VERSION" };
const juce::Identifier ID{ "ID" };
const juce::Identifier NEXT_ID{ "NEXT_ID" };
const juce::Identifier UUID{ "UUID" };
const juce::Identifier STATE{ "STATE" };
const juce::Identifier CARTESIAN_POSITION{ "CARTESIAN_POSITION" };
const juce::Identifier GAIN{ "GAIN" };
const juce::Identifier FREQ{ "FREQ" };
const juce::Identifier DIRECT_OUT_ONLY{ "DIRECT_OUT_ONLY" };
} // namespace gris
