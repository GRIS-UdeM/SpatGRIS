/*
 This file is part of SpatGRIS.

 Developers: Samuel Béland, Olivier Bélanger, Nicolas Masson

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

#include "AudioStructs.hpp"
#include "StaticMap.hpp"
#include "TaggedAudioBuffer.hpp"

class SpeakerModel;

//==============================================================================
/**
 * Does most of the spatialization heavy-lifting.
 */
class AudioProcessor
{
    AudioData mAudioData{};
    juce::CriticalSection mCriticalSection{};

public:
    //==============================================================================
    AudioProcessor();
    ~AudioProcessor();
    //==============================================================================
    AudioProcessor(AudioProcessor const &) = delete;
    AudioProcessor(AudioProcessor &&) = delete;
    AudioProcessor & operator=(AudioProcessor const &) = delete;
    AudioProcessor & operator=(AudioProcessor &&) = delete;
    //==============================================================================
    // Reinit HRTF delay lines.
    void resetHrtf();
    void setAudioConfig(AudioConfig const & newAudioConfig);
    [[nodiscard]] juce::CriticalSection const & getLock() const noexcept { return mCriticalSection; }
    void processAudio(SourceAudioBuffer & sourceBuffer, SpeakerAudioBuffer & speakerBuffer) noexcept;

    auto & getAudioData() { return mAudioData; }
    auto const & getAudioData() const { return mAudioData; }

private:
    //==============================================================================
    SourcePeaks muteSoloVuMeterIn(SourceAudioBuffer & inputBuffer) const noexcept;
    SpeakerPeaks muteSoloVuMeterGainOut(SpeakerAudioBuffer & speakerBuffer) noexcept;
    void processVbap(SourceAudioBuffer const & inputBuffer,
                     SpeakerAudioBuffer & outputBuffer,
                     SourcePeaks const & sourcePeaks) noexcept;
    void
        processLbap(SourceAudioBuffer & sourceBuffer, SpeakerAudioBuffer & speakerBuffer, SourcePeaks const &) noexcept;
    void processVBapHrtf(SourceAudioBuffer const & inputBuffer,
                         SpeakerAudioBuffer & outputBuffer,
                         SourcePeaks const & sourcePeaks) noexcept;
    void processStereo(SourceAudioBuffer const & sourceBuffer,
                       SpeakerAudioBuffer & speakerBuffer,
                       SourcePeaks const &) noexcept;
    //==============================================================================
    JUCE_LEAK_DETECTOR(AudioProcessor)
};
