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

#include "constants.hpp"

#include "StaticMap.hpp"
#include "TaggedAudioBuffer.hpp"
#include "macros.h"
DISABLE_WARNINGS
#include <JuceHeader.h>
ENABLE_WARNINGS

#include "AudioStructs.hpp"

class Speaker;

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
    ~AudioProcessor() = default;
    //==============================================================================
    AudioProcessor(AudioProcessor const &) = delete;
    AudioProcessor(AudioProcessor &&) = delete;
    AudioProcessor & operator=(AudioProcessor const &) = delete;
    AudioProcessor & operator=(AudioProcessor &&) = delete;
    //==============================================================================
    // Audio Status.

    // LBAP distance attenuation functions.
    void setAttenuationDbIndex(int index);
    void setAttenuationFrequencyIndex(int index);

    // Reinit HRTF delay lines.
    void resetHrtf();

    [[nodiscard]] juce::CriticalSection const & getCriticalSection() const noexcept { return mCriticalSection; }

    void setAudioConfig(AudioConfig const & audioConfig);

    //==============================================================================
    // Audio processing
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
    void processAudio(SourceAudioBuffer & sourceBuffer, SpeakerAudioBuffer & speakerBuffer) noexcept;

private:
    //==============================================================================
    // Connect the server's outputs to the system's inputs.
    SourcePeaks muteSoloVuMeterIn(SourceAudioBuffer & inputBuffer) const noexcept;
    void updateSourceVbap(int idS) noexcept;
    //==============================================================================
    JUCE_LEAK_DETECTOR(AudioProcessor)
};
