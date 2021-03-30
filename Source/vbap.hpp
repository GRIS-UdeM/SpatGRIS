/*
 * Functions for 3D VBAP processing based on work by Ville Pulkki.
 * (c) Ville Pulkki - 2.2.1999 Helsinki University of Technology.
 * Updated by belangeo, 2017.
 */

#pragma once

#include <cmath>

#include "macros.h"

DISABLE_WARNINGS
#include <JuceHeader.h>
ENABLE_WARNINGS

#include "PolarVector.h"
#include "StrongTypes.hpp"

constexpr auto MAX_SPEAKER_COUNT = 256;
constexpr auto MIN_VOL_P_SIDE_LENGTH = 0.01f;
static float constexpr ANGLE_TO_RADIAN = juce::MathConstants<float>::twoPi / 360.0f;

using fast = juce::dsp::FastMathApproximations;

struct SpeakersSetup {
    int dimension;         /* Number of dimension, always 3. */
    int count;             /* Number of speakers. */
    degrees_t * azimuth;   /* Azimuth angle of speakers. */
    degrees_t * elevation; /* Elevation angle of speakers. */
};

/* A struct for a loudspeaker triplet or pair (set). */
struct SpeakerSet {
    int speakerNos[3];
    float invMx[9];
    float setGains[3];
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
    output_patch_t outputPatches[MAX_SPEAKER_COUNT]; /* Physical outputs (starts at 1). */
    float gains[MAX_SPEAKER_COUNT];                  /* Loudspeaker gains. */
    float gainsSmoothing[MAX_SPEAKER_COUNT];         /* Loudspeaker gains smoothing. */
    int dimension;                                   /* Dimensions, 2 or 3. */
    SpeakerSet * speakerSets;                        /* Loudspeaker triplet structure. */
    int numOutputPatches;                            /* Number of output patches. */
    int numSpeakers;                                 /* Number of loudspeakers. */
    int numTriplets;                                 /* Number of triplets. */
    PolarVector angularDirection;                    /* Angular direction. */
    CartesianVector cartesianDirection;              /* Cartesian direction. */
    CartesianVector spreadingVector;                 /* Spreading vector. */
};

/* Fill a SPEAKERS_SETUP structure from values.
 */
SpeakersSetup * load_speakers_setup(int count, degrees_t * azimuth, degrees_t * elevation) noexcept;

/* Properly free a previously allocated SPEAKERS_SETUP structure.
 */
void free_speakers_setup(SpeakersSetup * setup) noexcept;

VbapData * init_vbap_from_speakers(LoudSpeaker speakers[MAX_SPEAKER_COUNT],
                                   int count,
                                   int dimensions,
                                   output_patch_t const outputPatches[MAX_SPEAKER_COUNT],
                                   output_patch_t maxOutputPatch,
                                   [[maybe_unused]] int const * const * tripletsFileName);

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

int vbap_get_triplets(VbapData const * data, int *** triplets);
