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
struct SpeakerHighpassConfig {
    double b1{};
    double b2{};
    double b3{};
    double b4{};
    double ha0{};
    double ha1{};
    double ha2{};
};

// The running variables of a speaker highpass filter.
struct SpeakerHighpassData {
    double x1{};
    double x2{};
    double x3{};
    double x4{};
    double y1{};
    double y2{};
    double y3{};
    double y4{};
};

struct LbapDistanceAttenuationConfig {
    float linearGain{};
    float lowpassCoefficient{};
};

struct LbapDistanceAttenuationData {
    float lastGain{};
    float lastCoefficient{};
    float lowpassY{};
    float lowpassZ{};
};

// The info needed per speaker to spatialize sound. This info is updated constantly by the message thread.
struct SpeakerAudioConfig {
    float gain{};
    tl::optional<SpeakerHighpassConfig> highpassConfig{};
};

// The current data info of a speaker. This is only accessed by the audioProcessor and the message thread never
// interferes.
struct SpeakerAudioData {
    // Last effectiv gains used in spatialization algorithms. Used for interpolation.
    StaticManager<source_index_t, float, MAX_INPUTS> lastSpatGains{};
    // The current state of the per-speaker highpass.
    SpeakerHighpassData highpassData{};
};

struct SourceAudioData {
    // LBAP-specific
    LbapDistanceAttenuationData lbapSourceAttenuationVars{};
    // STEREO-specific
    radians_t stereoLastAzimuth{};
};

struct AudioConfig {
    float masterGain{};
    float gainInterpolation{};

    StaticManager<output_patch_t, SpeakerAudioConfig, MAX_OUTPUTS> speakersAudioConfig{};

    // LBAP-specific
    LbapDistanceAttenuationConfig lbapDistanceAttenuationConfig{};
};

struct AudioState {
    using SpeakersSpatGains = StaticManager<output_patch_t, float, MAX_OUTPUTS>;
    StaticManager<source_index_t, ThreadsafePtr<SpeakersSpatGains>, MAX_INPUTS> spatGainMatrix{};

    AudioConfig audioConfig{};

    StaticManager<output_patch_t, SpeakerAudioData, MAX_OUTPUTS> speakersAudioData{};

    juce::Atomic<StaticManager<source_index_t, float, MAX_INPUTS> *> sourcePeaks{};
    juce::Atomic<StaticManager<output_patch_t, float, MAX_OUTPUTS> *> speakerPeaks{};

    // STEREO-specific
    StaticManager<source_index_t, ThreadsafePtr<radians_t>, MAX_OUTPUTS> stereoAzimuths{};
    //==============================================================================
    AudioState() = default;
    ~AudioState()
    {
        for (auto & gainArray : spatGainMatrix) {
            gainArray.free();
        }
        for (auto & azimuth : stereoAzimuths) {
            azimuth.free();
        }
        delete speakerPeaks.get();
        delete sourcePeaks.get();
    }
    //==============================================================================
    AudioState(AudioState const &) = delete;
    AudioState(AudioState &&) = delete;
    AudioState & operator=(AudioState const &) = delete;
    AudioState & operator=(AudioState &&) = delete;
};

//==============================================================================
// LOGIC/GUI SIDE
//==============================================================================

enum class PortState { normal, muted, solo };

struct SourceData {
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

struct SpeakerData {
    speaker_id_t id{};
    output_patch_t outputPatch{};
    PortState state{};
    PolarVector vector{};
    CartesianVector position{};
    float gain{};
    tl::optional<SpeakerHighpassConfig> crossoverData{};
    float peak{};
    bool isSelected{};
};

struct SpatGrisData {
    using Sources = StaticManager<source_index_t, SourceData, MAX_INPUTS>;
    using Speakers = StaticManager<output_patch_t, SourceData, MAX_OUTPUTS>;
    using DirectOutSpeakers = Manager<SpeakerData, speaker_id_t>;

    Sources sources{};
    Speakers speakers{};
    DirectOutSpeakers directOutSpeakers{};
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
    LbapDistanceAttenuationData attenuationData{};
    PolarVector lastVector{};
};

class SpatAlgorithm
{
public:
    SpatAlgorithm() = default;
    virtual ~SpatAlgorithm() = default;
    //==============================================================================
    SpatAlgorithm(SpatAlgorithm const &) = delete;
    SpatAlgorithm(SpatAlgorithm &&) = delete;
    SpatAlgorithm & operator=(SpatAlgorithm const &) = delete;
    SpatAlgorithm & operator=(SpatAlgorithm &&) = delete;
    //==============================================================================
    virtual void init(SpatGrisData::Speakers const & speakers) = 0;
    [[nodiscard]] virtual AudioState::SpeakersSpatGains
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