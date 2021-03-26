/** \file lbap.h
 *  \brief Layer-Based Amplitude Panning framework.
 *
 * LBAP (Layer-Based Amplitude Panning) is a framework written in C
 * to do 2-D or 3-D sound spatialisation. It uses a pre-computed
 * gain matrices to perform the spatialisation of the sources very
 * efficiently.
 */

/** \mainpage Welcome to LBAP documentation
 *
 * LBAP (Layer-Based Amplitude Panning) is a framework written in C
 * to do 2-D or 3-D sound spatialisation. It uses a pre-computed
 * gain matrices to perform the spatialisation of the sources very
 * efficiently.
 *
 * \section reference API Reference
 *
 * Complete API Reference: lbap.h
 *
 * ---
 *
 * \author Olivier Belanger, 2018
 */

#pragma once

#include "StaticVector.h"
#include "StrongTypes.hpp"
#include "constants.hpp"

static auto constexpr LBAP_MATRIX_SIZE = 64;

struct SpeakerData;

/** \brief A structure containing coordinates of a speaker in the field.
 *
 * This structure is used to hold informations about a given speaker in
 * the field. It is the responsibility of the user to provide angular
 * coordinates (`azi`, `ele` and `rad`) values. parameters `azi` (-pi .. pi)
 * and `ele` (0 .. pi/2) must be given in radians and parameter `rad`
 * (the length of the vector) is in the range 0 to 1.
 *
 * The order of the physical outputs may not match the order in which the
 * field will process the speakers. The `spkid` is an integer (starting
 * at 0) used by the field to properly order the output signals.
 */
struct lbap_speaker {
    radians_t azimuth;          /**< Azimuth in the range -pi .. pi. */
    radians_t elevation;        /**< Elevation in the range 0 .. pi/2. */
    float radius;               /**< Length of the vector in the range 0 .. 1. */
    output_patch_t outputPatch; /**< Physical output id. */
};

/** \brief A structure containing coordinates of a point in the field.
 *
 * This structure is used by other functions of the framework to compute
 * the position of the speakers and the sources in the field. It is the
 * responsibility of the user to provide angular coordinates (`azi`, `ele`
 * and `rad`) values. parameters `azi` (-pi .. pi) and `ele` (0 .. pi/2)
 * must be given in radians and parameter `rad` (the length of the vector)
 * is in the range 0 to 1.
 *
 * The user should never give values to the cartesian coordinates (`x`, `y`
 * and `z`), the framework will automatically fill these values according
 * to the angular coordinates.
 */
struct lbap_pos {
    radians_t azimuth;   /**< Azimuth in the range -pi .. pi. */
    radians_t elevation; /**< Elevation in the range 0 .. pi/2. */
    float radius;        /**< Length of the vector in the range 0 .. 1. */
    float x;
    float y;
    float z;
    float radiusSpan;
    float elevationSpan;

    [[nodiscard]] constexpr bool operator==(lbap_pos const & other) const noexcept
    {
        return azimuth == other.azimuth && elevation == other.elevation && radius == other.radius && x == other.x
               && y == other.y && z == other.z && radiusSpan == other.radiusSpan
               && elevationSpan == other.elevationSpan;
    }
    [[nodiscard]] constexpr bool operator!=(lbap_pos const & other) const noexcept { return !(*this == other); }
};

/* =================================================================================
Opaque data type declarations.
================================================================================= */

using matrix_t = std::array<std::array<float, LBAP_MATRIX_SIZE + 1>, LBAP_MATRIX_SIZE + 1>;

struct lbap_layer {
    int id;                                /**< Layer id. */
    radians_t elevation;                   /**< Elevation of the layer in the range 0 .. pi/2. */
    float gainExponent;                    /**< Speaker gain exponent for 4+ speakers. */
    std::vector<matrix_t> amplitudeMatrix; /**< Arrays of amplitude values [spk][x][y]. */
    std::vector<lbap_pos> speakers;        /**< Array of speakers. */
};

struct lbap_field {
    std::vector<output_patch_t> outputOrder; /**< Physical output order as a list of int. */
    std::vector<lbap_layer> layers;          /**< Array of layers. */
    [[nodiscard]] size_t getNumSpeakers() const
    {
        return std::reduce(layers.cbegin(), layers.cend(), size_t{}, [](size_t const sum, lbap_layer const & layer) {
            return sum + layer.speakers.size();
        });
    }
    void reset()
    {
        outputOrder.clear();
        layers.clear();
    }
};

/** \brief Creates the field's layers according to the position of speakers.
 *
 * This function creates the field's layer according to the position of
 * speakers given as `speakers` argument. The argument `num` is the number of
 * speakers passed to the function.
 */
void lbap_field_setup(lbap_field & field, std::vector<lbap_speaker> & speakers);

/** \brief Calculates the gain of the outputs for a source's position.
 *
 * This function uses the position `pos` to retrieve the gain for every
 * output from the field's layers and fill the array of float `gains`.
 * The user must provide the array of float and is responsible of its
 * memory. This array can be passed to the audio processing function
 * to control the gain of the signal outputs.
 */
void lbap_field_compute(lbap_field const & field, lbap_pos const & position, float * gains);

/** \brief Computes an array of lbap_speaker from lists of angular positions.
 *
 * This function takes as parameters an array of azimuth positions (given
 * in degrees from -180 to 180), an array of elevation positions (also given
 * in degrees, from 0 to 90), an array of radius (vector length between 0 and 1)
 * and an array of speaker output id (the physical output associated to the
 * speaker) and returns a corresponding array of lbap_speaker structures.
 * The parameter `num` is the number of elements in the arrays.
 *
 * _The user is responsible for freeing the array when finished with it._
 *
 * \return lbap_speaker array pointer.
 */
std::vector<lbap_speaker> lbap_speakers_from_positions(StaticVector<SpeakerData, MAX_OUTPUTS> const & speakers);

/** \brief Initialize an lbap_pos structure from a position in radians.
 *
 * This function takes as parameters a position where `azi` and `ele` are given
 * in radians, and where `rad` is the length of the vector between 0 and 1. The
 * first parameter is a pointer to an lbap_pos which will be properly initialized.
 */
lbap_pos lbap_pos_init_from_radians(radians_t azimuth, radians_t elevation, float radius);
