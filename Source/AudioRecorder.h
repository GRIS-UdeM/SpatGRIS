/*
 This file is part of SpatGRIS2.

 Developers: Samuel Béland, Olivier Bélanger, Nicolas Masson

 SpatGRIS2 is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 SpatGRIS2 is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with SpatGRIS2.  If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

#include "macros.h"

DISABLE_WARNINGS
#include <JuceHeader.h>
ENABLE_WARNINGS

//==============================================================================
// Audio recorder class used to write a monophonic sound file on disk.
class AudioRecorder
{
    std::unique_ptr<juce::AudioFormatWriter::ThreadedWriter>
        mThreadedWriter; // the FIFO used to buffer the incoming data
    juce::CriticalSection mWriterLock;
    std::atomic<juce::AudioFormatWriter::ThreadedWriter *> mActiveWriter{ nullptr };

public:
    //==============================================================================
    AudioRecorder();
    ~AudioRecorder() { stop(); }

    AudioRecorder(AudioRecorder const &) = delete;
    AudioRecorder(AudioRecorder &&) = delete;

    AudioRecorder & operator=(AudioRecorder const &) = delete;
    AudioRecorder & operator=(AudioRecorder &&) = delete;
    //==============================================================================
    void startRecording(juce::File const & file, unsigned int sampleRate, juce::String const & extF);
    //==============================================================================
    void stop();
    //==============================================================================
    void recordSamples(float * const * samples, int numSamples) const;
    //==============================================================================
    juce::TimeSliceThread backgroundThread; // the thread that will write our audio data to disk

private:
    //==============================================================================
    JUCE_LEAK_DETECTOR(AudioRecorder)
};