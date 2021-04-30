
#pragma once

#include <JuceHeader.h>

#include "AudioStructs.hpp"
#include "CartesianVector.h"
#include "LogicStrucs.hpp"
#include "PolarVector.h"
#include "StrongTypes.hpp"
#include "constants.hpp"

struct SourceData;
constexpr auto MIN_VOL_P_SIDE_LENGTH = 0.01f;

using fast = juce::dsp::FastMathApproximations;

struct SpeakersSetup {
    int dimension;         /* Number of dimension, always 3. */
    int count;             /* Number of speakers. */
    degrees_t * azimuth;   /* Azimuth angle of speakers. */
    degrees_t * elevation; /* Elevation angle of speakers. */
};

using InverseMatrix = std::array<float, 9>;

/* A struct for a loudspeaker triplet or pair (set). */
struct SpeakerSet {
    std::array<output_patch_t, 3> speakerNos;
    InverseMatrix invMx;
    std::array<float, 3> setGains;
    float smallestWt;
    int neg_g_am;
};

/* A struct for a loudspeaker instance. */
struct LoudSpeaker {
    CartesianVector coords;
    PolarVector angles;
};

/* VBAP structure of n loudspeaker panning */
struct VbapData {
    std::array<output_patch_t, MAX_OUTPUTS> outputPatches; /* Physical outputs (starts at 1). */
    // std::array<float, MAX_OUTPUTS> gains;                  /* Loudspeaker gains. */
    std::array<float, MAX_OUTPUTS> gainsSmoothing; /* Loudspeaker gains smoothing. */
    int dimension;                                 /* Dimensions, 2 or 3. */
    juce::Array<SpeakerSet> speakerSets;           /* Loudspeaker triplet structure. */
    int numOutputPatches;                          /* Number of output patches. */
    int numSpeakers;                               /* Number of loudspeakers. */
    PolarVector angularDirection;                  /* Angular direction. */
    CartesianVector cartesianDirection;            /* Cartesian direction. */
    CartesianVector spreadingVector;               /* Spreading vector. */
};

VbapData * init_vbap_from_speakers(std::array<LoudSpeaker, MAX_OUTPUTS> & speakers,
                                   int count,
                                   int dimensions,
                                   std::array<output_patch_t, MAX_OUTPUTS> const & outputPatches,
                                   output_patch_t maxOutputPatch);

/* Calculates gain factors using loudspeaker setup and angle direction.
 */
void vbap2(SourceData const & source, SpeakersSpatGains & gains, VbapData & data) noexcept;

juce::Array<Triplet> vbap_get_triplets(VbapData const & data);