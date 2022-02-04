/*
 This file is part of SpatGRIS.

 Developers: Gaël Lane Lépine, Samuel Béland, Olivier Bélanger, Nicolas Masson

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

#include "sg_AudioManager.hpp"

#include <JuceHeader.h>

namespace gris
{
//// audio format manager
// juce::AudioFormatManager manager{};
// manager.registerBasicFormats();
//// audio file to map in memory
// juce::File const file{ "C:/musik.wav" };
// jassert(file.existsAsFile());
//// audio format to use
// juce::AudioFormat * wavFormat{ manager.findFormatForFileExtension(file.getFileExtension()) };
//// make sure the format is valid and registered
// jassert(wavFormat);
//// create a file reader
// juce::MemoryMappedAudioFormatReader * reader = wavFormat->createMemoryMappedReader(file);
//// make sure the reader is created
// jassert(reader);
//// make sure the file is mono
// jassert(reader->getChannelLayout().size() == 1);
//// create an audio source that takes ownership of the reader
//// this audio source only knows how to get the next audio block
// juce::AudioFormatReaderSource source{ reader, true };
//// create an other audio source that can do start, stop, but more importantly, resampling
// juce::AudioTransportSource resampled_source{};
// resampled_source.setSource(&source);
//// inform the source about sample rate
// resampled_source.prepareToPlay(/*buffer size*/, /*sample rate*/);

class Player
{
    juce::AudioFormatManager manager{};
    juce::File file{};
    juce::AudioFormat * wavFormat{};
    juce::MemoryMappedAudioFormatReader * reader{};

public:
    //==============================================================================
    Player();
    ~Player();
    //==============================================================================
    void loadWavFilesAndSpeakerSetupFolder();

private:
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Player)
};

} // namespace gris