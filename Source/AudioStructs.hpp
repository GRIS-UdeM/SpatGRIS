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

//==============================================================================
// AUDIO SIDE
//==============================================================================

enum class PortState { normal, muted, solo };
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

    // VBAP-specific
    VbapDimensions vbapDimensions{};

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

struct SourceData {
    PortState state{};
    PolarVector vector{};
    CartesianVector position{};
    float azimuthSpan{};
    float zenithSpan{};
    tl::optional<output_patch_t> directOut{};
    float peak{};
    bool isSelected{};
    juce::Colour colour{};

    [[nodiscard]] SourceAudioConfig toConfig(bool const soloMode) const
    {
        SourceAudioConfig result;
        result.directOut = directOut;
        result.isMuted = soloMode ? state == PortState::solo : state != PortState::muted;
        return result;
    }
};

struct SpeakerHighpassData {
    hz_t freq{};

    [[nodiscard]] SpeakerHighpassConfig toConfig(double const sampleRate) const
    {
        auto const f{ narrow<double>(freq.get()) };
        auto const wc{ 2.0 * juce::MathConstants<double>::pi * f };
        auto const wc2{ wc * wc };
        auto const wc3{ wc2 * wc };
        auto const wc4{ wc2 * wc2 };
        auto const k{ wc / std::tan(juce::MathConstants<double>::pi * f / sampleRate) };
        auto const k2{ k * k };
        auto const k3{ k2 * k };
        auto const k4{ k2 * k2 };
        auto const sqTmp1{ juce::MathConstants<double>::sqrt2 * wc3 * k };
        auto const sqTmp2{ juce::MathConstants<double>::sqrt2 * wc * k3 };
        auto const aTmp{ 4.0 * wc2 * k2 + 2.0 * sqTmp1 + k4 + 2.0 * sqTmp2 + wc4 };
        auto const k4ATmp{ k4 / aTmp };

        /* common */
        auto const b1{ (4.0 * (wc4 + sqTmp1 - k4 - sqTmp2)) / aTmp };
        auto const b2{ (6.0 * wc4 - 8.0 * wc2 * k2 + 6.0 * k4) / aTmp };
        auto const b3{ (4.0 * (wc4 - sqTmp1 + sqTmp2 - k4)) / aTmp };
        auto const b4{ (k4 - 2.0 * sqTmp1 + wc4 - 2.0 * sqTmp2 + 4.0 * wc2 * k2) / aTmp };

        /* highpass */
        auto const ha0{ k4ATmp };
        auto const ha1{ -4.0 * k4ATmp };
        auto const ha2{ 6.0 * k4ATmp };

        return SpeakerHighpassConfig{ b1, b2, b3, b4, ha0, ha1, ha2 };
    }
};

struct SpeakerData {
    PortState state{};
    PolarVector vector{};
    CartesianVector position{};
    float gain{};
    tl::optional<SpeakerHighpassData> crossoverData{};
    float peak{};
    bool isSelected{};
    bool isDirectOutOnly{};

    [[nodiscard]] SpeakerAudioConfig toConfig(bool const soloMode, double const sampleRate) const
    {
        auto const getHighpassConfig = [&](SpeakerHighpassData const & data) { return data.toConfig(sampleRate); };

        SpeakerAudioConfig result;
        result.isMuted = soloMode ? state == PortState::solo : state != PortState::muted;
        result.gain = gain;
        result.highpassConfig = crossoverData.map(getHighpassConfig);
        result.isDirectOutOnly = isDirectOutOnly;
        return result;
    }
};

struct LbapDistanceAttenuationData {
    hz_t freq{};
    dbfs_t attenuation{};

    [[nodiscard]] LbapAttenuationConfig toConfig(double const sampleRate) const
    {
        auto const coefficient{ std::exp(-juce::MathConstants<float>::twoPi * freq.get() / narrow<float>(sampleRate)) };
        auto const gain{ attenuation.toGain() };
        LbapAttenuationConfig const result{ gain, coefficient };
        return result;
    }
};

struct SpatGrisData {
    using SourcesData = OwnedMap<source_index_t, SourceData>;
    using SpeakersData = OwnedMap<output_patch_t, SpeakerData>;
    using DirectOutSpeakers = OwnedMap<output_patch_t, SpeakerData>;

    SourcesData sourcesData{};
    SpeakersData speakersData{};
    DirectOutSpeakers directOutSpeakers{};

    double sampleRate{};
    float masterGain{};
    float spatGainsInterpolation{};
    tl::optional<float> pinkNoiseGain{};

    // Vbap-specific
    VbapDimensions vbapDimensions{};

    // Lbap-specific
    LbapDistanceAttenuationData lbapDistanceAttenuationData{};

    [[nodiscard]] std::unique_ptr<AudioConfig> toAudioConfig() const
    {
        auto result{ std::make_unique<AudioConfig>() };

        for (auto source : sourcesData) {
            if (source.value->directOut) {
                result->directOutPairs.add(std::make_pair(source.key, *source.value->directOut));
            }
        }

        auto const isAtLeastOneSourceSolo{ std::any_of(sourcesData.cbegin(), sourcesData.cend(), [](auto const node) {
            return node.value->state == PortState::solo;
        }) };
        auto const isAtLeastOnSpeakerSolo{ std::any_of(speakersData.cbegin(), speakersData.cend(), [](auto const node) {
            return node.value->state == PortState::solo;
        }) };

        result->lbapAttenuationConfig = lbapDistanceAttenuationData.toConfig(sampleRate);
        result->masterGain = masterGain;
        result->pinkNoiseGain = pinkNoiseGain;
        for (auto const source : sourcesData) {
            result->sourcesAudioConfig.add(source.key, source.value->toConfig(isAtLeastOneSourceSolo));
        }
        result->spatGainsInterpolation = spatGainsInterpolation;
        for (auto const speaker : speakersData) {
            result->speakersAudioConfig.add(speaker.key, speaker.value->toConfig(isAtLeastOnSpeakerSolo, sampleRate));
        }
        result->vbapDimensions = vbapDimensions;

        return result;
    }
};

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