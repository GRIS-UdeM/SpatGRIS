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

#include "sg_AbstractSpatAlgorithm.hpp"
#include "sg_AudioStructs.hpp"
#include "sg_TaggedAudioBuffer.hpp"

#include <JuceHeader.h>

namespace gris
{
class SpeakerModel;

//==============================================================================
/** Holds the spatialization algorithm instance and does most of the audio processing. */
class AudioProcessor
{
    AudioData mAudioData{};
    juce::CriticalSection mLock{};
    std::unique_ptr<AbstractSpatAlgorithm> mSpatAlgorithm{};
    juce::Random mRandomNoise{};

public:
    //==============================================================================
    AudioProcessor();
    ~AudioProcessor();
    SG_DELETE_COPY_AND_MOVE(AudioProcessor)
    //==============================================================================
    void setAudioConfig(std::unique_ptr<AudioConfig> newAudioConfig);
    [[nodiscard]] juce::CriticalSection const & getLock() const noexcept { return mLock; }
    void processAudio(SourceAudioBuffer & sourceBuffer,
                      SpeakerAudioBuffer & speakerBuffer,
                      juce::AudioBuffer<float> & stereoBuffer) noexcept;

    auto & getAudioData() { return mAudioData; }
    auto const & getAudioData() const { return mAudioData; }

    auto const & getSpatAlgorithm() const { return mSpatAlgorithm; }
    auto & getSpatAlgorithm() { return mSpatAlgorithm; }

private:
    //==============================================================================
    void processInputPeaks(SourceAudioBuffer & inputBuffer, SourcePeaks & peaks) const noexcept;
    void processOutputModifiersAndPeaks(SpeakerAudioBuffer & speakersBuffer, SpeakerPeaks & peaks) noexcept;
    //==============================================================================
    JUCE_LEAK_DETECTOR(AudioProcessor)
};
} // namespace gris