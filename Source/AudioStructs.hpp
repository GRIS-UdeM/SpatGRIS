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

#include "AtomicExchanger.hpp"
#include "Constants.hpp"
#include "PolarVector.hpp"
#include "SpatMode.hpp"
#include "StaticMap.hpp"
#include "StrongArray.hpp"

#include <JuceHeader.h>

#include "lib/tl/optional.hpp"

enum class VbapType { twoD, threeD };

//==============================================================================
struct SpeakerHighpassState {
    double x1{};
    double x2{};
    double x3{};
    double x4{};
    double y1{};
    double y2{};
    double y3{};
    double y4{};
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
    //==============================================================================
    void process(float * data, int numSamples, SpeakerHighpassState & state) const;
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
    SpeakerHighpassState highpassState{};
};

//==============================================================================
struct LbapSourceAttenuationState {
    float currentGain{};
    float currentCoefficient{};
    float lowpassY{};
    float lowpassZ{};
};

//==============================================================================
struct LbapAttenuationConfig {
    float linearGain{};
    float lowpassCoefficient{};
    //==============================================================================
    void process(float * data, int numSamples, float distance, LbapSourceAttenuationState & state) const;
};

//==============================================================================
using SpeakersSpatGains = StrongArray<output_patch_t, float, MAX_NUM_SPEAKERS>;

//==============================================================================
struct SourceAudioState {
    SpeakersSpatGains lastSpatGains{};

    // LBAP-specific
    LbapSourceAttenuationState lbapAttenuationState{};
    // STEREO-specific
    radians_t stereoLastAzimuth{};
};

//==============================================================================
struct SourceAudioConfig {
    bool isMuted{};
    tl::optional<output_patch_t> directOut{};
};

//==============================================================================
struct AudioConfig {
    SpatMode spatMode{};
    float masterGain{};
    float spatGainsInterpolation{};

    juce::Array<std::pair<source_index_t, output_patch_t>> directOutPairs{};

    StaticMap<source_index_t, SourceAudioConfig, MAX_NUM_SOURCES> sourcesAudioConfig{};
    StaticMap<output_patch_t, SpeakerAudioConfig, MAX_NUM_SPEAKERS> speakersAudioConfig{};

    tl::optional<float> pinkNoiseGain{};

    // LBAP-specific
    LbapAttenuationConfig lbapAttenuationConfig{};
};

//==============================================================================
struct HrtfData {
    std::array<unsigned, 16> count{};
    std::array<std::array<float, 128>, 16> inputTmp{};
    std::array<std::array<float, 128>, 16> leftImpulses{};
    std::array<std::array<float, 128>, 16> rightImpulses{};
};

//==============================================================================
struct AudioState {
    StrongArray<source_index_t, SourceAudioState, MAX_NUM_SOURCES> sourcesAudioState{};
    StrongArray<output_patch_t, SpeakerAudioState, MAX_NUM_SPEAKERS> speakersAudioState{};
    StrongArray<source_index_t, AtomicExchanger<SpeakersSpatGains>::Ticket *, MAX_NUM_SOURCES> mostRecentSpatGains{};
    // HRTF-specific
    HrtfData hrtf{};
};

//==============================================================================
using SourcePeaks = StrongArray<source_index_t, float, MAX_NUM_SOURCES>;
using SpeakerPeaks = StrongArray<output_patch_t, float, MAX_NUM_SPEAKERS>;

//==============================================================================
struct AudioData {
    // Offline message thread -> audio thread
    std::unique_ptr<AudioConfig> config{};

    // Live audio thread -> audio thread
    AudioState state{};

    // Live message thread -> audio thread
    StrongArray<source_index_t, AtomicExchanger<SpeakersSpatGains>, MAX_NUM_SOURCES> spatGainMatrix{};
    StrongArray<source_index_t, std::atomic<float>, MAX_NUM_SOURCES> lbapSourceDistances{}; // Lbap-specific

    // Live audio thread -> message thread
    AtomicExchanger<SourcePeaks> sourcePeaks{};
    AtomicExchanger<SpeakerPeaks> speakerPeaks{};
};

//==============================================================================
// LOGIC/GUI SIDE
//==============================================================================

//==============================================================================
// Spat algorithms side
//=============================================================================

static auto constexpr LBAP_MATRIX_SIZE = 64;
using matrix_t = std::array<std::array<float, LBAP_MATRIX_SIZE + 1>, LBAP_MATRIX_SIZE + 1>;

struct LbapLayer {
    int id{};
    radians_t elevation{};
    float gainExponent{};
    std::vector<matrix_t> amplitudeMatrix{};
};

struct LbapData {
    LbapSourceAttenuationState attenuationData{};
    PolarVector lastVector{};
};