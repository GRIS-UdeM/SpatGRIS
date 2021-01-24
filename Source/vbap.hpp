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

constexpr auto MAX_SPEAKER_COUNT = 256;
constexpr auto MAX_TRIPLET_COUNT = 128;
constexpr auto MIN_VOL_P_SIDE_LENGTH = 0.01f;
static float constexpr ANGLE_TO_RADIAN = juce::MathConstants<float>::twoPi / 360.0f;

using fast = juce::dsp::FastMathApproximations;

struct SpeakersSetup {
    int dimension;     /* Number of dimension, always 3. */
    int count;         /* Number of speakers. */
    float * azimuth;   /* Azimuth angle of speakers. */
    float * elevation; /* Elevation angle of speakers. */
};

/* Cartesian vector for a speaker position. */
struct CartesianVector {
    float x;
    float y;
    float z;

    [[nodiscard]] constexpr CartesianVector operator/(float const scalar) const noexcept
    {
        CartesianVector const result{ x / scalar, y / scalar, z / scalar };
        return result;
    }

    /* Returns the vector length without the sqrt. */
    [[nodiscard]] constexpr float length2() const noexcept { return x * x + y * y + z * z; }

    [[nodiscard]] float length() const noexcept { return std::sqrt(length2()); }

    [[nodiscard]] CartesianVector crossProduct(CartesianVector const & other) const noexcept
    {
        auto const newX = (y * other.z) - (z * other.y);
        auto const newY = (z * other.x) - (x * other.z);
        auto const newZ = (x * other.y) - (y * other.x);
        CartesianVector const unscaledResult{ newX, newY, newZ };

        auto const length = unscaledResult.length();
        auto const result{ unscaledResult / length };

        return result;
    }

    [[nodiscard]] constexpr CartesianVector operator-() const noexcept { return CartesianVector{ -x, -y, -z }; }

    [[nodiscard]] constexpr float dotProduct(CartesianVector const & other) const noexcept
    {
        return (x * other.x + y * other.y + z * other.z);
    }

    [[nodiscard]] constexpr CartesianVector mean(CartesianVector const & other) const noexcept
    {
        auto const newX{ (x + other.x) * 0.5f };
        auto const newY{ (y + other.y) * 0.5f };
        auto const newZ{ (z + other.z) * 0.5f };

        CartesianVector const result{ newX, newY, newZ };
        return result;
    }

    [[nodiscard]] float angleWith(CartesianVector const & other) const noexcept
    {
        auto inner = dotProduct(other) / std::sqrt(length2() * other.length2());
        inner = std::clamp(inner, -1.0f, 1.0f);
        return std::abs(std::acos(inner));
    }
};

/* Angular vector for a speaker position. */
struct AngularVector {
    float azimuth;
    float elevation;
    float length;

    /* Converts a vector from angular to cartesian coordinates. */
    CartesianVector toCartesian() const noexcept
    {
        auto const cele = fast::cos(elevation * ANGLE_TO_RADIAN);
        auto const x = fast::cos(azimuth * ANGLE_TO_RADIAN) * cele;
        auto const y = fast::sin(azimuth * ANGLE_TO_RADIAN) * cele;
        auto const z = fast::sin(elevation * ANGLE_TO_RADIAN);

        CartesianVector const result{ x, y, z };
        return result;
    }

    [[nodiscard]] constexpr bool isOnSameElevation(AngularVector const & other) const noexcept
    {
        return elevation > other.elevation - 5.0f && elevation < other.elevation + 5.0f;
    }
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
    AngularVector angles;
};

/* VBAP structure of n loudspeaker panning */
struct VbapData {
    int outputPatches[MAX_SPEAKER_COUNT];    /* Physical outputs (starts at 1). */
    float gains[MAX_SPEAKER_COUNT];          /* Loudspeaker gains. */
    float gainsSmoothing[MAX_SPEAKER_COUNT]; /* Loudspeaker gains smoothing. */
    int dimension;                           /* Dimensions, 2 or 3. */
    SpeakerSet * speakerSets;                /* Loudspeaker triplet structure. */
    int numOutputPatches;                    /* Number of output patches. */
    int numSpeakers;                         /* Number of loudspeakers. */
    int numTriplets;                         /* Number of triplets. */
    AngularVector angularDirection;          /* Angular direction. */
    CartesianVector cartesianDirection;      /* Cartesian direction. */
    CartesianVector spreadingVector;         /* Spreading vector. */
};

/* Fill a SPEAKERS_SETUP structure from values.
 */
SpeakersSetup * load_speakers_setup(int count, float * azimuth, float * elevation) noexcept;

/* Fill a SPEAKERS_SETUP structure from the content of a text file.
 *
 * File format:
 *
 * First line starts with an integer which is the number of speakers.
 * Remaining lines (must be equal to the number of speakers) starts with
 * two floats. They give the azimuth and elevation for each speakers.
 */
SpeakersSetup * load_speakers_setup_from_file(char const * filename) noexcept;

/* Properly free a previously allocated SPEAKERS_SETUP structure.
 */
void free_speakers_setup(SpeakersSetup * setup) noexcept;

/* Initialize a VBAP_DATA structure from a loudspeakers setup and
 * an optional matrix of user-defined triplets.
 */
VbapData * init_vbap_data(SpeakersSetup * setup, int const * const * triplets) noexcept;

VbapData * init_vbap_from_speakers(LoudSpeaker lss[MAX_SPEAKER_COUNT],
                                   int count,
                                   int dim,
                                   int outputPatches[MAX_SPEAKER_COUNT],
                                   int maxOutputPatch,
                                   int const * const * triplets) noexcept;

VbapData * copy_vbap_data(VbapData const * data) noexcept;

/* Properly free a previously allocated VBAP_DATA structure.
 */
void free_vbap_data(VbapData * data) noexcept;

/* Calculates gain factors using loudspeaker setup and angle direction.
 */
void vbap(float azimuth, float elevation, float spread, VbapData * data) noexcept;
void vbap2(float azimuth, float elevation, float spAzimuth, float spElevation, VbapData * data) noexcept;
void vbap_flip_y_z(float azimuth, float elevation, float spread, VbapData * data) noexcept;
void vbap2_flip_y_z(float azimuth, float elevation, float spAzimuth, float spElevation, VbapData * data) noexcept;

int vbap_get_triplets(VbapData const * data, int *** triplets);
