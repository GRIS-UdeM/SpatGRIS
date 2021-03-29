#pragma once

#include <JuceHeader.h>

#include "lib/tl/optional.hpp"

#include "Manager.hpp"
#include "PolarVector.h"
#include "StaticManager.hpp"
#include "StaticVector.h"
#include "StrongTypes.hpp"
#include "ThreadsafePtr.hpp"
#include "constants.hpp"

//==============================================================================
// AUDIO SIDE
//==============================================================================

// The parameters of a speaker highpass filter.
struct SpeakerCrossoverParams {
    double b1{};
    double b2{};
    double b3{};
    double b4{};
    double ha0{};
    double ha1{};
    double ha2{};
};

// The running variables of a speaker highpass filter.
struct SpeakerCrossoverVars {
    double x1{};
    double x2{};
    double x3{};
    double x4{};
    double y1{};
    double y2{};
    double y3{};
    double y4{};
};

struct LbapSourceAttenuationParams {
    float linearGain{};
    float lowpassCoefficient{};
};

struct LbapSourceAttenuationVars {
    float lastGain{};
    float lastCoefficient{};
    float lowpassY{};
    float lowpassZ{};
};

// The info needed per speaker to spatialize sound. This info is updated constantly by the message thread.
struct SpeakerParams {
    float gain{};
    tl::optional<SpeakerCrossoverParams> crossoverParams{};
};

// The current data info of a speaker. This is only accessed by the audioProcessor and the message thread never
// interferes.
struct SpeakerVars {
    // Last effective gains used in spatialization algorithms. Used for interpolation.
    StaticManager<source_index_t, float, MAX_INPUTS> lastSpatGains{};
    // The current state of the per-speaker highpass.
    SpeakerCrossoverVars crossoverVars{};
    // The last gains computed for every source for this speaker. Used for gain smoothing.
    StaticManager<source_index_t, float, MAX_INPUTS> lastSourceGains{};

    // LBAP-specific
    LbapSourceAttenuationVars lbapSourceAttenuationVars{};
    // STEREO-specific
    radians_t stereoLastAzimuth{};
};

struct AudioState {
    using SpeakerGains = StaticManager<output_patch_t, float, MAX_OUTPUTS>;

    StaticManager<output_patch_t, SpeakerParams, MAX_OUTPUTS> speakerParams{};
    StaticManager<output_patch_t, SpeakerVars, MAX_OUTPUTS> speakerVars{};
    StaticManager<source_index_t, ThreadsafePtr<SpeakerGains>, MAX_INPUTS> gainMatrix{};

    juce::Atomic<StaticManager<source_index_t, float, MAX_INPUTS> *> sourcePeaks{};
    juce::Atomic<StaticManager<output_patch_t, float, MAX_OUTPUTS> *> speakerPeaks{};

    // LBAP-specific
    ThreadsafePtr<LbapSourceAttenuationParams> lbapAttenuationData{};

    // STEREO-specific
    StaticManager<source_index_t, ThreadsafePtr<StaticManager<output_patch_t, radians_t, MAX_OUTPUTS>>, MAX_INPUTS>
        stereoAzimuths{};

    ~AudioState()
    {
        for (auto & gainArray : gainMatrix) {
            gainArray.free();
        }
        lbapAttenuationData.free();
        delete speakerPeaks.get();
        delete sourcePeaks.get();
    }
};

static auto constexpr AUDIO_DATA{ sizeof(AudioState) / 1024.0f / 1024.0f };

//==============================================================================
// LOGIC/GUI SIDE
//==============================================================================

enum class PortState { normal, muted, solo };

struct Source {
    source_index_t index{};
    PortState state{};
    PolarVector vector{};
    CartesianVector position{};
    float azimuthSpan{};
    float zenithSpan{};
    tl::optional<output_patch_t> directOut{};
    float peak{};
    bool isSelected{};
    juce::Colour colour{};
};

struct Speaker {
    speaker_id_t id{};
    output_patch_t outputPatch{};
    PortState state{};
    PolarVector vector{};
    CartesianVector position{};
    float gain{};
    tl::optional<SpeakerCrossoverParams> crossoverData{};
    float peak{};
    bool isSelected{};
};

struct SpatGrisData {
    using Sources = StaticManager<source_index_t, Source, MAX_INPUTS>;
    using Speakers = StaticManager<output_patch_t, Source, MAX_OUTPUTS>;
    using DirectOutSpeakers = Manager<Speaker, speaker_id_t>;

    Sources sources{};
    Speakers speakers{};
    DirectOutSpeakers directOutSpeakers{};
};

static auto constexpr LOGIC_DATA{ sizeof(SpatGrisData) / 1024.0f / 1024.0f };

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
    LbapSourceAttenuationVars attenuationData{};
    PolarVector lastVector{};
};

class SpatAlgorithm
{
public:
    virtual void init(SpatGrisData::Speakers const & speakers) = 0;
    [[nodiscard]] virtual AudioState::SpeakerGains
        computeSpeakerGains(PolarVector const & sourcePosition) const noexcept = 0;
};

struct VbapSpeakerTriplet {
    std::array<speaker_id_t, 3> ids{};
    std::array<float, 9> inverseMatrix{};
    std::array<float, 3> gains{};
    float smallestWt{};
    int neg_g_am{};
};

class VbapAlgorithm : public SpatAlgorithm
{
    enum class Dimensions { three, two };

    Dimensions mDimensions{ Dimensions::three };
    std::vector<VbapSpeakerTriplet> mTriplets{};

public:
};