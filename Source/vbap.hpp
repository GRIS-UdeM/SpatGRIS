/*
 * Functions for 3D VBAP processing based on work by Ville Pulkki.
 * (c) Ville Pulkki - 2.2.1999 Helsinki University of Technology.
 * Updated by belangeo, 2017.
 */

#pragma once

#include "macros.h"
DISABLE_WARNINGS
#include <JuceHeader.h>
ENABLE_WARNINGS

#include "AudioStructs.hpp"
#include "PolarVector.h"
#include "StrongTypes.hpp"

constexpr auto MIN_VOL_P_SIDE_LENGTH = 0.01f;
static float constexpr ANGLE_TO_RADIAN = juce::MathConstants<float>::twoPi / 360.0f;

using fast = juce::dsp::FastMathApproximations;

/* A struct for a loudspeaker triplet or pair (set). */
struct SpeakerSet {
    std::array<output_patch_t, 3> speakerNos{};
    std::array<float, 9> invMx{};
    std::array<float, 3> setGains{};
    float smallestWt{};
    int neg_g_am{};
};

/* VBAP structure of n loudspeaker panning */
struct VbapData {
    std::array<output_patch_t, MAX_OUTPUTS> outputPatches{}; /* Physical outputs (starts at 1). */
    std::array<float, MAX_OUTPUTS> gains{};                  /* Loudspeaker gains. */
    int dimension{};                                         /* Dimensions, 2 or 3. */
    SpeakerSet * speakerSets{};                              /* Loudspeaker triplet structure. */
    int numOutputPatches{};                                  /* Number of output patches. */
    int numSpeakers{};                                       /* Number of loudspeakers. */
    int numTriplets{};                                       /* Number of triplets. */
    PolarVector angularDirection{};                          /* Angular direction. */
    CartesianVector cartesianDirection{};                    /* Cartesian direction. */
    CartesianVector spreadingVector{};                       /* Spreading vector. */
};

std::unique_ptr<VbapData> init_vbap_from_speakers(SpatGrisData::SpeakersData const & speakers);

VbapData * copy_vbap_data(VbapData const * data) noexcept;

/* Properly free a previously allocated VBAP_DATA structure.
 */
void free_vbap_data(VbapData * data) noexcept;

/* Calculates gain factors using loudspeaker setup and angle direction.
 */
void vbap2(degrees_t azimuth, degrees_t elevation, float spAzimuth, float spElevation, VbapData * data) noexcept;
void vbap2_flip_y_z(degrees_t azimuth,
                    degrees_t elevation,
                    float spAzimuth,
                    float spElevation,
                    VbapData * data) noexcept;

juce::Array<std::array<output_patch_t, 3>> vbap_get_triplets(VbapData const * data);
