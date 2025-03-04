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

#include "../Containers/sg_AtomicUpdater.hpp"
#include "../Containers/sg_StaticMap.hpp"
#include "../Containers/sg_StrongArray.hpp"
#include "StrongTypes/sg_Radians.hpp"
#include "sg_SpatMode.hpp"
#include "sg_constants.hpp"

/** This file contains most of the structures used in an audio context. */

namespace gris
{
enum class VbapType { twoD, threeD };

float constexpr SMALL_GAIN = 0.0000000000001f;
double constexpr DENORM_GAIN = 1.0e-60;

//==============================================================================
struct ColdSpeakerHighpass {
    double x1{};
    double x2{};
    double x3{};
    double x4{};
    double y1{};
    double y2{};
    double y3{};
    double y4{};
    //==============================================================================
    void resetValues();
};

//==============================================================================
struct SpeakerHighpassConfig {
    double b1{};
    double b2{};
    double b3{};
    double b4{};
    double ha0{};
    double ha1{};
    double ha2{};
    mutable bool isNewConfig{ true };
    //==============================================================================
    void process(float * data, int numSamples, ColdSpeakerHighpass & state, juce::Random & randNoise) const;
};

//==============================================================================
struct SpeakerAudioConfig {
    bool isDirectOutOnly{};
    float gain{};
    bool isMuted{};
    tl::optional<SpeakerHighpassConfig> highpassConfig{};
};

//==============================================================================
struct SpeakerAudioState {
    ColdSpeakerHighpass highpassState{};
};

//==============================================================================
struct MbapSourceAttenuationState {
    float currentGain{};
    float currentCoefficient{};
    float lowpassY{};
    float lowpassZ{};
};

//==============================================================================
struct MbapAttenuationConfig {
    float linearGain{};
    float lowpassCoefficient{};
    bool shouldProcess{};
    //==============================================================================
    void process(float * data, int numSamples, float distance, MbapSourceAttenuationState & state) const;
};

//==============================================================================
using SpeakersSpatGains = StrongArray<output_patch_t, float, MAX_NUM_SPEAKERS>;

//==============================================================================
struct SourceAudioState {
    SpeakersSpatGains lastSpatGains{};

    // MBAP-specific
    MbapSourceAttenuationState mbapAttenuationState{};
    // STEREO-specific
    radians_t stereoLastAzimuth{};
};

//==============================================================================
struct SourceAudioConfig {
    bool isMuted{};
    tl::optional<output_patch_t> directOut{};
};

//==============================================================================
using SourcesAudioConfig = StaticMap<source_index_t, SourceAudioConfig, MAX_NUM_SOURCES>;
using SpeakersAudioConfig = StaticMap<output_patch_t, SpeakerAudioConfig, MAX_NUM_SPEAKERS>;

//==============================================================================
struct AudioConfig {
    SpatMode spatMode{};
    bool isStereo{};
    bool isStereoMuted{};
    float masterGain{};
    float spatGainsInterpolation{};

    juce::Array<std::pair<source_index_t, output_patch_t>> directOutPairs{};

    SourcesAudioConfig sourcesAudioConfig{};
    SpeakersAudioConfig speakersAudioConfig{};

    tl::optional<float> pinkNoiseGain{};

    // MBAP-specific
    MbapAttenuationConfig mbapAttenuationConfig{};
};

//==============================================================================
struct AudioState {
    StrongArray<source_index_t, SourceAudioState, MAX_NUM_SOURCES> sourcesAudioState{};
    StrongArray<output_patch_t, SpeakerAudioState, MAX_NUM_SPEAKERS> speakersAudioState{};
};

//==============================================================================
using SourcePeaks = StrongArray<source_index_t, float, MAX_NUM_SOURCES>;
using SpeakerPeaks = StrongArray<output_patch_t, float, MAX_NUM_SPEAKERS>;
using StereoPeaks = std::array<float, 2>;

//==============================================================================
struct AudioData {
    // message thread -> audio thread (cold)
    std::unique_ptr<AudioConfig> config{};

    // audio thread -> audio thread (hot)
    AudioState state{};

    // audio thread -> message thread (hot)
    AtomicUpdater<SourcePeaks> sourcePeaksUpdater{};
    AtomicUpdater<SpeakerPeaks> speakerPeaksUpdater{};
    AtomicUpdater<StereoPeaks> stereoPeaksUpdater{};
};

} // namespace gris
