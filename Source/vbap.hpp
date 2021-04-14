/*
 * Functions for 3D VBAP processing based on work by Ville Pulkki.
 * (c) Ville Pulkki - 2.2.1999 Helsinki University of Technology.
 * Updated by belangeo, 2017.
 */

#pragma once

#include <JuceHeader.h>

#include "AudioStructs.hpp"
#include "LogicStrucs.hpp"
#include "PolarVector.h"
#include "StrongTypes.hpp"

constexpr auto MIN_VOL_P_SIDE_LENGTH = 0.01f;
static float constexpr ANGLE_TO_RADIAN = juce::MathConstants<float>::twoPi / 360.0f;

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

std::unique_ptr<VbapData> init_vbap_from_speakers(SpeakersData const & speakers);

/* Calculates gain factors using loudspeaker setup and angle direction.
 */
SpeakersSpatGains
    vbap2(degrees_t azimuth, degrees_t elevation, float spAzimuth, float spElevation, VbapData * data) noexcept;
SpeakersSpatGains vbap2_flip_y_z(degrees_t azimuth,
                                 degrees_t elevation,
                                 float spAzimuth,
                                 float spElevation,
                                 VbapData * data) noexcept;

struct SpeakerTriplet {
    output_patch_t patch1{};
    output_patch_t patch2{};
    output_patch_t patch3{};
    //==============================================================================
    [[nodiscard]] bool contains(output_patch_t const patch) const noexcept
    {
        return patch == patch1 || patch == patch2 || patch == patch3;
    }
};
juce::Array<SpeakerTriplet> vbap_get_triplets(VbapData const & data);
