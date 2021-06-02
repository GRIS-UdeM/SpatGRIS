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

#include "AudioStructs.hpp"
#include "StrongArray.hpp"
#include "TaggedAudioBuffer.hpp"

#include <JuceHeader.h>

class SpeakerModel;

//==============================================================================
/**
 * Does most of the spatialization heavy-lifting.
 */
class AudioProcessor
{
    AudioData mAudioData{};
    juce::CriticalSection mLock{};

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
    void setAudioConfig(std::unique_ptr<AudioConfig> newAudioConfig);
    [[nodiscard]] juce::CriticalSection const & getLock() const noexcept { return mLock; }
    void processAudio(SourceAudioBuffer & sourceBuffer, SpeakerAudioBuffer & speakerBuffer) noexcept;

    auto & getAudioData() { return mAudioData; }
    auto const & getAudioData() const { return mAudioData; }

private:
    //==============================================================================
    void muteSoloVuMeterIn(SourceAudioBuffer & inputBuffer, SourcePeaks & peaks) const noexcept;
    void muteSoloVuMeterGainOut(SpeakerAudioBuffer & speakersBuffer, SpeakerPeaks & peaks) noexcept;
    void processVbap(SourceAudioBuffer const & inputBuffer,
                     SpeakerAudioBuffer & outputBuffer,
                     SpeakersAudioConfig const & speakersAudioConfig,
                     SourcePeaks const & sourcePeaks) noexcept;
    void processLbap(SourceAudioBuffer & sourcesBuffer,
                     SpeakerAudioBuffer & speakersBuffer,
                     SpeakersAudioConfig const & speakersAudioConfig,
                     SourcePeaks const & sourcePeaks) noexcept;
    void processHrtf(SourceAudioBuffer & inputBuffer,
                     SpeakerAudioBuffer & outputBuffer,
                     SourcePeaks const & sourcePeaks) noexcept;
    void processStereo(SourceAudioBuffer const & inputBuffer,
                       SpeakerAudioBuffer & outputBuffer,
                       SourcePeaks const &) noexcept;
    //==============================================================================
    JUCE_LEAK_DETECTOR(AudioProcessor)
};
