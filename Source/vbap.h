/*
 * Functions library for VBAP processing based on vbap version 0.1
 * by Ville Pulkki.
 * 
 * (c) Ville Pulkki - 2.2.1999 Helsinki University of Technology
 *
 * Modified by belangeo, 2017.
 * - Added missing headers for gcc.
 * - Merged define_loudspeakers.c, define_louds_routines.c and vbap.c
 *   functions into a single library.
 * - Removed redundant casting by using float version of trig functions.
 * - Remove global variables.
 * - Added function documentation.
 *
 * TODO:
 * - Add the span parameter.
 * - Implement load_speakers_setup without file reading.
 * - Implement load_ls_triplets without file reading.
 *
 */

#ifdef __cplusplus 
extern "C" {
#endif

#define MAX_LS_AMOUNT 128
#define MIN_VOL_P_SIDE_LGTH 0.01
#define MAX_TRIPLET_AMOUNT 64

#define VBAP
    
typedef struct {
    int dimension;
    int count;
    float *azimuth;
    float *elevation;
} speakers_setup;

typedef struct {
    float x;
    float y;
    float z;
} cart_vec;

typedef struct {
    float azi;
    float ele;
    float length;
} ang_vec;

/* A struct for a loudspeaker triplet or pair (set). */
typedef struct {
    int ls_nos[3];
    float ls_mx[9];
    float set_weights[3];
    float smallest_wt;
} LS_SET;

/* A struct for a loudspeaker instance. */
typedef struct { 
    cart_vec coords;
    ang_vec angles;
    int channel_nbr;
} ls; // rename to "loudspeaker".

/* A struct for all loudspeakers. */
typedef struct ls_triplet_chain {
    int ls_nos[3];      // speaker numbers.
    float inv_mx[9];    // inverse matrix.
    struct ls_triplet_chain *next;
} ls_triplet_chain;

/* A struct for all loudspeakers. */
typedef struct {
    int dimension;
    int triplet_count;
    int numbers[MAX_TRIPLET_AMOUNT][3];      // speaker numbers.
    double matrices[MAX_TRIPLET_AMOUNT][9];    // inverse matrix.
} DATA;

/* functions */
void load_ls_triplets(ls lss[MAX_LS_AMOUNT], 
                      struct ls_triplet_chain **ls_triplets, 
                      int ls_amount, char *filename);

/* Fill a speakers_setup structure from the content of a text file.
 *
 * File format:
 *
 * First line starts with an integer which is the number of dimensions.
 * Second line starts with an integer which is the number of speakers.
 * Remaining lines (must be equal to the number of speakers) starts
 * with one (2 dimensions) or two (3 dimensions) floats. They give
 * the azimuth and elevation (3 dimensions) for each speakers.
 *
 */
speakers_setup * load_speakers_setup_from_file(const char *filename);

// speakers_setup * load_speakers_setup(int, int, float *, float *);

void build_speakers_list(speakers_setup *setup, ls lss[MAX_LS_AMOUNT]);

extern void choose_ls_triplets(ls lss[MAX_LS_AMOUNT],
                               ls_triplet_chain **ls_triplets, 
                               int ls_amount);

extern void choose_ls_tuplets(ls lss[MAX_LS_AMOUNT],
                              ls_triplet_chain **ls_triplets,
                              int ls_amount);

extern void calculate_3x3_matrixes(ls_triplet_chain *ls_triplets,
                                   ls lss[MAX_LS_AMOUNT], int ls_amount);

int read_ls_conf(FILE *fp, double lsm[MAX_TRIPLET_AMOUNT][9],
                 int lstripl[MAX_TRIPLET_AMOUNT][3],
                 int *triplet_amount);

void vbap(double g[3], int ls[3], float x, float y, float z, int dimension,
          double lsm[MAX_TRIPLET_AMOUNT][9],
          int lstripl[MAX_TRIPLET_AMOUNT][3],
          int triplet_amount);

#ifdef __cplusplus 
}
#endif

