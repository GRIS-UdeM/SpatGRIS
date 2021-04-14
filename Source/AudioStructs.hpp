#pragma once

#include "lib/tl/optional.hpp"
#include <JuceHeader.h>

#include "PolarVector.h"
#include "SpatMode.hpp"
#include "StaticMap.hpp"
#include "ThreadsafePtr.hpp"
#include "constants.hpp"

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
    float lastGain{};
    float lastCoefficient{};
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
typedef StaticMap<output_patch_t, float, MAX_OUTPUTS> SpeakersSpatGains;

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

    StaticMap<source_index_t, SourceAudioConfig, MAX_INPUTS> sourcesAudioConfig{};
    StaticMap<output_patch_t, SpeakerAudioConfig, MAX_OUTPUTS> speakersAudioConfig{};

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
    StaticMap<source_index_t, SourceAudioState, MAX_INPUTS> sourcesAudioState{};
    StaticMap<output_patch_t, SpeakerAudioState, MAX_OUTPUTS> speakersAudioState{};

    // HRTF-specific
    HrtfData hrtf{};
};

//==============================================================================
typedef StaticMap<source_index_t, float, MAX_INPUTS> SourcePeaks;
typedef StaticMap<output_patch_t, float, MAX_OUTPUTS> SpeakerPeaks;

//==============================================================================
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