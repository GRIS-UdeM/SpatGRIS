#pragma once

#include "AudioStructs.hpp"

enum class PortState { normal, muted, solo };
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

    [[nodiscard]] juce::XmlElement * toXml(source_index_t const index) const
    {
        auto result{ std::make_unique<juce::XmlElement>(juce::String{ index.get() }) };

        result->addChildElement(position.toXml());

        return result.release();
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