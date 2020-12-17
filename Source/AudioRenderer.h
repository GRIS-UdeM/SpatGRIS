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
// Audio recorder class used to write an interleaved multi-channel soundfile on disk.
class AudioRenderer final : public juce::ThreadWithProgressWindow
{
    juce::AudioFormatManager mFormatManager;
    juce::File mFileToRecord;
    juce::Array<juce::File> mFiles;
    unsigned int mSampleRate;

public:
    //==============================================================================
    AudioRenderer();
    ~AudioRenderer() override = default;

    AudioRenderer(AudioRenderer const &) = delete;
    AudioRenderer(AudioRenderer &&) = delete;

    AudioRenderer & operator=(AudioRenderer const &) = delete;
    AudioRenderer & operator=(AudioRenderer &&) = delete;
    //==============================================================================
    void prepareRecording(juce::File const & file, juce::Array<juce::File> const & fileNames, unsigned int sampleRate);
    void run() override;
    void threadComplete(bool userPressedCancel) override;

private:
    //==============================================================================
    JUCE_LEAK_DETECTOR(AudioRenderer)
};