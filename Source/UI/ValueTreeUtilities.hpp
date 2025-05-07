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
void copyValueTreeProperties(const juce::ValueTree & source, juce::ValueTree & dest);

/**
 * Converts an old speaker setup ValueTree to the CURRENT_SPEAKERSETUP_VERSION.
 *
 * @param oldSpeakerSetup The ValueTree representing the old speaker setup.
 * @return A new ValueTree representing the converted speaker setup.
 */
juce::ValueTree convertSpeakerSetup(const juce::ValueTree & oldSpeakerSetup);

// Speaker setup identifiers
const auto CURRENT_SPEAKERSETUP_VERSION = "4.0.0";
const juce::Identifier SPEAKER_SETUP("SPEAKER_SETUP");
const juce::Identifier SPEAKER_GROUP("SPEAKER_GROUP");
const juce::Identifier SPEAKER("SPEAKER");
const juce::Identifier VERSION("VERSION");
const juce::Identifier ID ("ID");
const juce::Identifier X ("X");
const juce::Identifier Y ("Y");
const juce::Identifier Z ("Z");
const juce::Identifier GAIN ("GAIN");
const juce::Identifier FREQ ("FREQ");
const juce::Identifier DIRECT_OUT_ONLY ("DIRECT_OUT_ONLY");
} // namespace gris
