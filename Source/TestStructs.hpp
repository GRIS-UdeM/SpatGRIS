#pragma once

#include <JuceHeader.h>

#include "lib/tl/optional.hpp"

#include "Manager.hpp"
#include "StaticVector.h"
#include "StrongTypes.hpp"
#include "constants.hpp"
#include "narrow.hpp"
#include <bitset>
#include <variant>

template<typename key_t, typename value_t, size_t CAPACITY>
class StaticManager
{
    static_assert(
        std::is_trivially_default_constructible_v<value_t> && std::is_trivially_default_constructible_v<key_t>);
    static_assert(std::is_same_v<key_t, speaker_index_t> || std::is_same_v<key_t, output_patch_t>);

    std::array<value_t, CAPACITY> mData{};
    std::bitset<CAPACITY> mUsed{};
    juce::CriticalSection mCriticalSection{};

public:
    void clear() { mUsed.reset(); }

    void set(key_t const key, value_t && value)
    {
        auto const index{ narrow<size_t>(key.get() - 1) };
        mData[index] = std::forward<value_t>(value);
        mUsed.set(index);
    }

    value_t const & get(key_t const key) const
    {
        auto const index{ narrow<size_t>(key.get() - 1) };
        jassert(mUsed.test(index));
        return mData[index];
    }

    auto const & getCriticalSection() const { return mCriticalSection; }
};

// Spat static

struct SpatStatic {
};

// Spat dynamic

struct PolarVector {
    radians_t azimuth{};
    radians_t zenith{};
    float length{};
};

struct CartesianVector {
    float x{};
    float y{};
    float z{};
};

struct CrossoverData {
    struct ActiveData {
        double x1{};
        double x2{};
        double x3{};
        double x4{};
        double y1{};
        double y2{};
        double y3{};
        double y4{};
    };

    hz_t freq{};

    double b1{};
    double b2{};
    double b3{};
    double b4{};
    double ha0{};
    double ha1{};
    double ha2{};

    ActiveData activeData{};
};

enum class PortState { normal, muted, solo };

struct AttenuationData {
    float lastGain{};
    float lastCoefficient{};
    float lowpassY{};
    float lowpassZ{};
};

template<typename T>
class PerOutputArray
{
    static constexpr size_t CAPACITY{ MAX_OUTPUTS };

    std::array<float, CAPACITY> mData{};

public:
    T & operator[](output_patch_t const outputPatch)
    {
        auto const index{ narrow<size_t>(outputPatch.get() - 1) };
        jassert(index < CAPACITY);
        return mData[index];
    }
    T const & operator[](output_patch_t const outputPatch) const
    {
        auto const index{ narrow<size_t>(outputPatch.get() - 1) };
        jassert(index < CAPACITY);
        return mData[index];
    }
};

struct SpeakerTriplet {
    std::array<speaker_id_t, 3> ids{};
    std::array<float, 9> inverseMatrix{};
    std::array<float, 3> gains{};
    float smallestWt{};
    int neg_g_am{};
};

struct VbapSourceData {
    PerOutputArray<float> gains{};
    PerOutputArray<float> gainsSmoothing{};
};

struct VbapData {
    enum class Dimensions { three, two };

    StaticVector<VbapSourceData, MAX_INPUTS> sourceData{};
    Dimensions dimensions{ Dimensions::three };
    std::vector<SpeakerTriplet> triplets{};
};

static auto constexpr LBAP_MATRIX_SIZE = 64;
using matrix_t = std::array<std::array<float, LBAP_MATRIX_SIZE + 1>, LBAP_MATRIX_SIZE + 1>;

struct LbapLayer {
    int id{};
    radians_t elevation{};
    float gainExponent{};
    std::vector<matrix_t> amplitudeMatrix{};
};

struct LbapData {
    AttenuationData attenuationData{};
    PerOutputArray<float> y{};
    PolarVector lastVector{};
};

struct StereoData {
    radians_t lastAzimuth{};
};

struct Speaker {
    speaker_id_t id{};
    output_patch_t outputPatch{};
    PortState state{};
    bool isDirectOut{};
    PolarVector vector{};
    CartesianVector position{};
    float gain{};
    tl::optional<CrossoverData> crossoverData{};
    float peak{};
};

struct Source {
    int index{};
    PortState state{};
    PolarVector vector{};
    CartesianVector position{};
    float azimuthSpan{};
    float zenithSpan{};
    float gain{};
    tl::optional<output_patch_t> directOut{};
    float peak{};

    juce::Colour colour{};
};

struct SpatGrisData {
    StaticVector<Source, MAX_INPUTS> sources{};
    Manager<Speaker, speaker_id_t> speakers{};
};

static auto constexpr TEST{ sizeof(SpatGrisData) / 1024.0f / 1024.0f };