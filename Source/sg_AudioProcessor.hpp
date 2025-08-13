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

#include "sg_AbstractSpatAlgorithm.hpp"
#include "Data/sg_AudioStructs.hpp"
#include "Containers/sg_TaggedAudioBuffer.hpp"

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
#if SG_USE_FORK_UNION
    #if SG_FU_METHOD == SG_FU_USE_ARRAY_OF_ATOMICS
                      ForkUnionBuffer & forkUnionBuffer,
    #elif SG_FU_METHOD == SG_FU_USE_BUFFER_PER_THREAD
                      ForkUnionBuffer & forkUnionBuffer,
    #endif
#endif
                      juce::AudioBuffer<float> & stereoBuffer) noexcept;

    auto & getAudioData() { return mAudioData; }
    auto const & getAudioData() const { return mAudioData; }

    auto const & getSpatAlgorithm() const { return mSpatAlgorithm; }
    auto & getSpatAlgorithm() { return mSpatAlgorithm; }

#if SG_USE_FORK_UNION && (SG_FU_METHOD == SG_FU_USE_ARRAY_OF_ATOMICS || SG_FU_METHOD == SG_FU_USE_BUFFER_PER_THREAD)
    void silenceForkUnionBuffer(ForkUnionBuffer & forkUnionBuffer) noexcept
    {
        if (mSpatAlgorithm)
            mSpatAlgorithm->silenceForkUnionBuffer(forkUnionBuffer);
    }
#endif

private:
    //==============================================================================
    void processInputPeaks(SourceAudioBuffer & inputBuffer, SourcePeaks & peaks) const noexcept;
    void processOutputModifiersAndPeaks(SpeakerAudioBuffer & speakersBuffer, SpeakerPeaks & peaks) noexcept;
    //==============================================================================
    JUCE_LEAK_DETECTOR(AudioProcessor)
};
} // namespace gris
