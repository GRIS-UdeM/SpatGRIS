#pragma once

#include <JuceHeader.h>

#include "lib/tl/optional.hpp"

#include "OwnedMap.hpp"
#include "PolarVector.h"
#include "SpatMode.hpp"
#include "StaticMap.hpp"
#include "StaticVector.hpp"
#include "StrongTypes.hpp"
#include "ThreadsafePtr.hpp"
#include "constants.hpp"

enum class VbapDimensions { two, three };

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

// The parameters of a speaker highpass filter.
struct SpeakerHighpassConfig {
    double b1{};
    double b2{};
    double b3{};
    double b4{};
    double ha0{};
    double ha1{};
    double ha2{};

    void process(float * data, int const numSamples, SpeakerHighpassState & state) const
    {
        for (int sampleIndex{}; sampleIndex < numSamples; ++sampleIndex) {
            auto const inval{ static_cast<double>(data[sampleIndex]) };
            auto const val{ ha0 * inval + ha1 * state.x1 + ha2 * state.x2 + ha1 * state.x3 + ha0 * state.x4
                            - b1 * state.y1 - b2 * state.y2 - b3 * state.y3 - b4 * state.y4 };
            state.y4 = state.y3;
            state.y3 = state.y2;
            state.y2 = state.y1;
            state.y1 = val;
            state.x4 = state.x3;
            state.x3 = state.x2;
            state.x2 = state.x1;
            state.x1 = inval;
            data[sampleIndex] = static_cast<float>(val);
        }
    }
};

struct SpeakerAudioConfig {
    bool isDirectOutOnly{};
    float gain{};
    bool isMuted{};
    tl::optional<SpeakerHighpassConfig> highpassConfig{};
};

// The current data info of a speaker. This is only accessed by the audioProcessor and the message thread never
// interferes.
struct SpeakerAudioState {
    SpeakerHighpassState highpassState{};
};

struct LbapSourceAttenuationState {
    float lastGain{};
    float lastCoefficient{};
    float lowpassY{};
    float lowpassZ{};
};

struct LbapAttenuationConfig {
    float linearGain{};
    float lowpassCoefficient{};

    void process(float * data, int const numSamples, float const distance, LbapSourceAttenuationState & state) const
    {
        auto const distanceGain{ (1.0f - distance) * (1.0f - linearGain) + linearGain };
        auto const distanceCoefficient{ distance * lowpassCoefficient };
        auto const diffGain{ (distanceGain - state.lastGain) / narrow<float>(numSamples) };
        auto const diffCoefficient = (distanceCoefficient - state.lastCoefficient) / narrow<float>(numSamples);
        auto filterInY{ state.lowpassY };
        auto filterInZ{ state.lowpassZ };
        auto lastCoefficient{ state.lastCoefficient };
        auto lastGain{ state.lastGain };
        // TODO : this could be greatly optimized
        if (diffCoefficient == 0.0f && diffGain == 0.0f) {
            // simplified version
            for (int sampleIndex{}; sampleIndex < numSamples; ++sampleIndex) {
                filterInY = data[sampleIndex] + (filterInY - data[sampleIndex]) * lastCoefficient;
                filterInZ = filterInY + (filterInZ - filterInY) * lastCoefficient;
                data[sampleIndex] = filterInZ * lastGain;
            }
        } else {
            // full version
            for (int sampleIndex{}; sampleIndex < numSamples; ++sampleIndex) {
                lastCoefficient += diffCoefficient;
                lastGain += diffGain;
                filterInY = data[sampleIndex] + (filterInY - data[sampleIndex]) * lastCoefficient;
                filterInZ = filterInY + (filterInZ - filterInY) * lastCoefficient;
                data[sampleIndex] = filterInZ * lastGain;
            }
        }
        state.lowpassY = filterInY;
        state.lowpassZ = filterInZ;
        state.lastGain = distanceGain;
        state.lastCoefficient = distanceCoefficient;
    }
};

typedef StaticMap<output_patch_t, float, MAX_OUTPUTS> SpeakersSpatGains;

struct SourceAudioState {
    SpeakersSpatGains lastSpatGains{};

    // LBAP-specific
    LbapSourceAttenuationState lbapAttenuationState{};
    // STEREO-specific
    radians_t stereoLastAzimuth{};
};

struct SourceAudioConfig {
    bool isMuted{};
    tl::optional<output_patch_t> directOut{};
};

struct AudioConfig {
    SpatMode spatMode{};
    float masterGain{};
    float spatGainsInterpolation{};

    juce::Array<std::pair<source_index_t, output_patch_t>> directOutPairs{};

    StaticMap<source_index_t, SourceAudioConfig, MAX_INPUTS> sourcesAudioConfig{};
    StaticMap<output_patch_t, SpeakerAudioConfig, MAX_OUTPUTS> speakersAudioConfig{};

    tl::optional<float> pinkNoiseGain{};

    // LBAP-specific
    LbapAttenuationConfig lbapAttenuationConfig{};
};

struct HrtfData {
    std::array<unsigned, 16> count{};
    std::array<std::array<float, 128>, 16> inputTmp{};
    std::array<std::array<float, 128>, 16> leftImpulses{};
    std::array<std::array<float, 128>, 16> rightImpulses{};
};

struct AudioState {
    StaticMap<source_index_t, SourceAudioState, MAX_INPUTS> sourcesAudioState{};
    StaticMap<output_patch_t, SpeakerAudioState, MAX_OUTPUTS> speakersAudioState{};

    // HRTF-specific
    HrtfData hrtf{};
};

typedef StaticMap<source_index_t, float, MAX_INPUTS> SourcePeaks;
typedef StaticMap<output_patch_t, float, MAX_OUTPUTS> SpeakerPeaks;
struct AudioData {
    // Offline message thread -> audio thread
    AudioConfig config{};

    // Live audio thread -> audio thread
    AudioState state{};

    // Live message thread -> audio thread
    StaticMap<source_index_t, ThreadsafePtr<SpeakersSpatGains>, MAX_INPUTS> spatGainMatrix{};
    StaticMap<source_index_t, juce::Atomic<float>, MAX_INPUTS> lbapSourceDistances{};      // Lbap-specific
    StaticMap<source_index_t, juce::Atomic<radians_t>, MAX_INPUTS> stereoSourceAzimuths{}; // STEREO-specific

    // Live audio thread -> message thread
    ThreadsafePtr<SourcePeaks> sourcePeaks{};
    ThreadsafePtr<SpeakerPeaks> speakerPeaks{};
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