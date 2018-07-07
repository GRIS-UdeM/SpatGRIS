/** \file lbap.h
 *  \brief Layer-Based Amplitude Panning framework.
 *
 * LBAP (Layer-Based Amplitude Panning) is a framework written in C 
 * to do 2-D or 3-D sound spatialisation. It uses a pre-computed
 * gain matrices to perform the spatialisation of the sources very
 * efficiently.
 */

// TODO:
// Add a localisation/diffusion parameter ?

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

#ifndef __LBAP_H
#define __LBAP_H

#ifdef __cplusplus 
extern "C" {
#endif

#define LBAP_MATRIX_SIZE 64

/* Opaque data types. */
typedef struct lbap_layer lbap_layer;
typedef struct lbap_field lbap_field;

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
typedef struct {
    float azi;              /**< Azimuth in the range -pi .. pi. */
    float ele;              /**< Elevation in the range 0 .. pi/2. */
    float rad;              /**< Length of the vector in the range 0 .. 1. */
    int spkid;              /**< Physical output id. */
} lbap_speaker;


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
typedef struct {
    float azi;              /**< Azimuth in the range -pi .. pi. */
    float ele;              /**< Elevation in the range 0 .. pi/2. */
    float rad;              /**< Length of the vector in the range 0 .. 1. */
    float x;
    float y;
    float z;
    float azispan;
} lbap_pos;

/** \brief Initializes a new spatialization field.
 *
 * This function creates and initializes a new spatialization field.
 * 
 * It's the reponsibility of the user to call `lbap_field_free(field)`
 * when done with it.
 *
 * \return lbap_field pointer.
 */
lbap_field * lbap_field_init();

/** \brief Frees memory used by a lbap_field.
 *
 * This function must be used to properly release the memory used by the
 * `lbap_field` given as parameter.
 */
void lbap_field_free(lbap_field *field);

/** \brief Resets a lbap_field.
 *
 * This function resets the state of the `lbap_field` given as parameter.
 */
void lbap_field_reset(lbap_field *field);

/** \brief Creates the field's layers according to the position of speakers.
 *
 * This function creates the field's layer according to the position of
 * speakers given as `speakers` argument. The argument `num` is the number of 
 * speakers passed to the function.
 */
void lbap_field_setup(lbap_field *field, lbap_speaker *speakers, int num);

/** \brief Calculates the gain of the outputs for a source's position.
 *
 * This function uses the position `pos` to retrieve the gain for every 
 * output from the field's layers and fill the array of float `gains`.
 * The user must provide the array of float and is responsible of its
 * memory. This array can be passed to the audio processing function
 * to control the gain of the signal outputs.
 */
void lbap_field_compute(lbap_field *field, lbap_pos *pos, float *gains);


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
lbap_speaker * lbap_speakers_from_positions(float *azi, float *ele, float *rad,
                                            int *spkid, int num);


/** \brief Initialize an lbap_pos structure from a position in radians.
 *
 * This function takes as parameters a position where `azi` and `ele` are given
 * in radians, and where `rad` is the length of the vector between 0 and 1. The
 * first parameter is a pointer to an lbap_pos which will be properly initialized. 
 */
void lbap_pos_init_from_radians(lbap_pos *pos, float azi, float ele, float rad);

/** \brief Initialize an lbap_pos structure from a position in degrees.
 *
 * This function takes as parameters a position where `azi` and `ele` are given
 * in degrees, and where `rad` is the length of the vector between 0 and 1. The
 * first parameter is a pointer to an lbap_pos which will be properly initialized. 
 */
void lbap_pos_init_from_degrees(lbap_pos *pos, float azi, float ele, float rad);

/** \brief Compare two positions.
 *
 * This function returns 1 if the two positions are identical, 0 otherwise. 
 */
int lbap_pos_compare(lbap_pos *p1, lbap_pos *p2);

/** \brief Copy the content of a lbap_pos to another.
 *
 * This function copy the content of `src` to `dest`. 
 */
void lbap_pos_copy(lbap_pos *dest, lbap_pos *src);

#ifdef __cplusplus 
}
#endif

#endif /* __LBAP_H */


