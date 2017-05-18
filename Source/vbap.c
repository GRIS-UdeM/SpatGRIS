/*
 * Functions library for VBAP processing based on vbap version 0.1
 * (vbap.c, define_louds_routines.c and define_loudspeakers.c) by Ville Pulkki.
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
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "vbap.h"

/* Returns 1 if there is loudspeaker(s) inside given ls triplet. 
 *
 * No external use.
 */
int any_ls_inside_triplet(int a, int b, int c, ls lss[MAX_LS_AMOUNT], 
                          int ls_amount) {
    float invdet, tmp, invmx[9];
    cart_vec *lp1, *lp2, *lp3;
    int i, j, k, any_ls_inside, this_inside;

    lp1 = &(lss[a].coords);
    lp2 = &(lss[b].coords);
    lp3 = &(lss[c].coords);

    /* Matrix inversion. */
    invdet = 1.0 / (lp1->x * ((lp2->y * lp3->z) - (lp2->z * lp3->y))
             - lp1->y * ((lp2->x * lp3->z) - (lp2->z * lp3->x))
             + lp1->z * ((lp2->x * lp3->y) - (lp2->y * lp3->x)));

    invmx[0] = ((lp2->y * lp3->z) - (lp2->z * lp3->y)) * invdet;
    invmx[3] = ((lp1->y * lp3->z) - (lp1->z * lp3->y)) * -invdet;
    invmx[6] = ((lp1->y * lp2->z) - (lp1->z * lp2->y)) * invdet;
    invmx[1] = ((lp2->x * lp3->z) - (lp2->z * lp3->x)) * -invdet;
    invmx[4] = ((lp1->x * lp3->z) - (lp1->z * lp3->x)) * invdet;
    invmx[7] = ((lp1->x * lp2->z) - (lp1->z * lp2->x)) * -invdet;
    invmx[2] = ((lp2->x * lp3->y) - (lp2->y * lp3->x)) * invdet;
    invmx[5] = ((lp1->x * lp3->y) - (lp1->y * lp3->x)) * -invdet;
    invmx[8] = ((lp1->x * lp2->y) - (lp1->y * lp2->x)) * invdet;

    any_ls_inside = 0;
    for(i=0; i< ls_amount; i++) {
        if (i != a && i != b && i != c) {
            this_inside = 1;
            for(j=0; j< 3; j++) {
                tmp = lss[i].coords.x * invmx[0 + j*3];
                tmp += lss[i].coords.y * invmx[1 + j*3];
                tmp += lss[i].coords.z * invmx[2 + j*3];
                if (tmp < -0.001)
                    this_inside = 0;
            }
            if (this_inside == 1)
                any_ls_inside = 1;
        }
    }
    return any_ls_inside;
}

/* Adds i, j, k triplet to triplet chain. 
 *
 * No external use.
 */
void add_ldsp_triplet(int i, int j, int k, 
                      struct ls_triplet_chain **ls_triplets,
                      ls lss[MAX_LS_AMOUNT]) {
    struct ls_triplet_chain *trip_ptr, *prev;
    trip_ptr = *ls_triplets;
    prev = NULL;

    while (trip_ptr != NULL) {
        prev = trip_ptr;
        trip_ptr = trip_ptr->next;
    }
    trip_ptr = (struct ls_triplet_chain*)malloc(sizeof(struct ls_triplet_chain));
    if (prev == NULL)
        *ls_triplets = trip_ptr;
    else 
        prev->next = trip_ptr;
    trip_ptr->next = NULL;
    trip_ptr->ls_nos[0] = i;
    trip_ptr->ls_nos[1] = j;
    trip_ptr->ls_nos[2] = k;
}

/*
 * No external use.
 */
float vec_length(cart_vec v1) {
    return sqrtf(v1.x * v1.x + v1.y * v1.y + v1.z * v1.z);
}

/*
 * No external use.
 */
float vec_prod(cart_vec v1, cart_vec v2) {
    return (v1.x * v2.x + v1.y * v2.y + v1.z * v2.z);
}

/*
 * No external use.
 */
float vec_angle(cart_vec v1, cart_vec v2) {
    float inner = ((v1.x * v2.x + v1.y * v2.y + v1.z * v2.z) /
                   (vec_length(v1) * vec_length(v2)));
    if (inner > 1.0)
        inner= 1.0;
    if (inner < -1.0)
        inner = -1.0;
    return fabsf(acosf(inner));
}

/*
 * No external use.
 */
void cross_prod(cart_vec v1, cart_vec v2, cart_vec *res) {
    float length;
    res->x = (v1.y * v2.z ) - (v1.z * v2.y);
    res->y = (v1.z * v2.x ) - (v1.x * v2.z);
    res->z = (v1.x * v2.y ) - (v1.y * v2.x);

    length = vec_length(*res);
    res->x /= length;
    res->y /= length;
    res->z /= length;
}

/*
 * No external use.
 */
double *
angle_to_cart(int azi, int ele) {
    double *res;
    double atorad = (2 * 3.1415927 / 360) ;
    res = (double *)malloc(3 * sizeof(double));
    res[0] = cos((double)(azi * atorad)) * cos((double)(ele * atorad));
    res[1] = sin((double)(azi * atorad)) * cos((double)(ele * atorad));
    res[2] = sin((double)(ele * atorad));
    return res;
}

/* Converts a vector from angular to cartesian coordinates.
 * 
 * Used in test_def_speakers.
 */
void vec_angle_to_cart(ang_vec *from, cart_vec *to) {
    float ang2rad = (float)(2 * M_PI / 360);
    to->x = cosf(from->azi * ang2rad) * cosf(from->ele * ang2rad);
    to->y = sinf(from->azi * ang2rad) * cosf(from->ele * ang2rad);
    to->z = sinf(from->ele * ang2rad);
}

/* Calculate volume of the parallelepiped defined by the loudspeaker
 * direction vectors and divide it with total length of the triangle sides. 
 * This is used when removing too narrow triangles.
 *
 * No external use.
 */
float vol_p_side_lgth(int i, int j, int k, ls lss[MAX_LS_AMOUNT]) {
    float volper, lgth;
    cart_vec xprod;
    cross_prod(lss[i].coords, lss[j].coords, &xprod);
    volper = fabsf(vec_prod(xprod, lss[k].coords));
    lgth = (fabsf(vec_angle(lss[i].coords,lss[j].coords)) +
            fabsf(vec_angle(lss[i].coords,lss[k].coords)) +
            fabsf(vec_angle(lss[j].coords,lss[k].coords)));
    if (lgth > 0.00001)
        return volper / lgth;
    else
        return 0.0;
}

/* Checks if two lines intersect on 3D sphere see theory in paper 
 * Pulkki, V. Lokki, T. "Creating Auditory Displays with Multiple 
 * Loudspeakers Using VBAP: A Case Study with DIVA Project" in 
 * International Conference on Auditory Displays -98. 
 * E-mail Ville.Pulkki@hut.fi if you want to have that paper.
 *
 * No external use.
 */
int lines_intersect(int i, int j, int k, int l, ls  lss[MAX_LS_AMOUNT]) {
    cart_vec v1;
    cart_vec v2;
    cart_vec v3, neg_v3;
    float angle;
    float dist_ij, dist_kl, dist_iv3, dist_jv3, dist_inv3, dist_jnv3;
    float dist_kv3, dist_lv3, dist_knv3, dist_lnv3;

    cross_prod(lss[i].coords, lss[j].coords,&v1);
    cross_prod(lss[k].coords, lss[l].coords,&v2);
    cross_prod(v1, v2, &v3);

    neg_v3.x = 0.0 - v3.x;
    neg_v3.y = 0.0 - v3.y;
    neg_v3.z = 0.0 - v3.z;

    dist_ij = (vec_angle(lss[i].coords, lss[j].coords));
    dist_kl = (vec_angle(lss[k].coords, lss[l].coords));
    dist_iv3 = (vec_angle(lss[i].coords, v3));
    dist_jv3 = (vec_angle(v3, lss[j].coords));
    dist_inv3 = (vec_angle(lss[i].coords, neg_v3));
    dist_jnv3 = (vec_angle(neg_v3, lss[j].coords));
    dist_kv3 = (vec_angle(lss[k].coords, v3));
    dist_lv3 = (vec_angle(v3, lss[l].coords));
    dist_knv3 = (vec_angle(lss[k].coords, neg_v3));
    dist_lnv3 = (vec_angle(neg_v3, lss[l].coords));

    /* If one of loudspeakers is close to crossing point, don't do anything. */
    if (fabsf(dist_iv3) <= 0.01 || fabsf(dist_jv3) <= 0.01 || 
        fabsf(dist_kv3) <= 0.01 || fabsf(dist_lv3) <= 0.01 ||
        fabsf(dist_inv3) <= 0.01 || fabsf(dist_jnv3) <= 0.01 || 
        fabsf(dist_knv3) <= 0.01 || fabsf(dist_lnv3) <= 0.01) {
        return(0);
    }

    if (((fabsf(dist_ij - (dist_iv3 + dist_jv3)) <= 0.01 ) &&
         (fabsf(dist_kl - (dist_kv3 + dist_lv3))  <= 0.01)) ||
        ((fabsf(dist_ij - (dist_inv3 + dist_jnv3)) <= 0.01)  &&
         (fabsf(dist_kl - (dist_knv3 + dist_lnv3)) <= 0.01 ))) {
        return (1);
    } else {
        return (0);
    }
}

/*
 * No external use.
 */
void sort_2D_lss(ls lss[MAX_LS_AMOUNT], 
                 int sorted_lss[MAX_LS_AMOUNT], 
                 int ls_amount) {
    int i, j, index;
    float x, y, tmp, tmp_azi;
    float rad2ang = (float)(360.0 / ( 2 * M_PI));

    /* Transforming angles between -180 and 180. */
    for (i=0; i<ls_amount; i++) {
        vec_angle_to_cart(&lss[i].angles, &lss[i].coords);
        lss[i].angles.azi = acosf(lss[i].coords.x);
        if (fabsf(lss[i].coords.y) <= 0.001)
            tmp = 1.0;
        else
            tmp = lss[i].coords.y / fabsf(lss[i].coords.y);
        lss[i].angles.azi *= tmp;
    }
    for (i=0; i<ls_amount; i++) {
        tmp = 2000;
        for (j=0 ; j<ls_amount; j++) {
            if (lss[j].angles.azi <= tmp) {
                tmp=lss[j].angles.azi;
                index = j;
            }
        }
        sorted_lss[i]=index;
        tmp_azi = lss[index].angles.azi;
        lss[index].angles.azi = (tmp_azi + 4000.0);
    }
    for (i=0; i<ls_amount; i++) {
        tmp_azi = lss[i].angles.azi;
        lss[i].angles.azi = (tmp_azi - 4000.0);
    }
}
  
/*
 * No external use.
 */
int calc_2D_inv_tmatrix(float azi1, float azi2, float inv_mat[4]) {
    float x1, x2, x3, x4; /* x1 x3 */
    float y1, y2, y3, y4; /* x2 x4 */
    float det;
    x1 = cosf(azi1);
    x2 = sinf(azi1);
    x3 = cosf(azi2);
    x4 = sinf(azi2);
    det = (x1 * x4) - (x3 * x2);
    if (fabsf(det) <= 0.001) {
        inv_mat[0] = 0.0;
        inv_mat[1] = 0.0;
        inv_mat[2] = 0.0;
        inv_mat[3] = 0.0;
        return 0;
    } else {
        inv_mat[0] = x4 / det;
        inv_mat[1] = -x3 / det;
        inv_mat[2] = -x2 / det;
        inv_mat[3] = x1 / det;
        return 1;
    }
}

speakers_setup *
load_speakers_setup_from_file(const char *filename) {
    int i = 0, dim, count;
    float azi, ele;
    char *toke;
    char c[10000];
    FILE *fp;
    speakers_setup *setup;
    setup = malloc(sizeof(speakers_setup));

    if ((fp = fopen(filename, "r")) == NULL) {
        fprintf(stderr, "Could not open loudspeaker setup file.\n");
        free(setup);
        exit(-1);
    }

    fgets(c, 10000, fp);
    toke = (char *)strtok(c, " ");
    sscanf(toke, "%d", &dim);
    if (!((dim == 2) || (dim == 3))){
        fprintf(stderr, "Error in loudspeaker dimension.\n");
        free(setup);
        exit(-1);
    }

    fgets(c, 10000, fp);
    toke = (char *)strtok(c, " ");
    sscanf(toke, "%d", &count);
    if (count < dim) {
        fprintf(stderr, "Too few loudspeakers %d\n", count);
        free(setup);
        exit(-1);
    }

    setup->azimuth = calloc(count, sizeof(float));
    setup->elevation = calloc(count, sizeof(float));
    while (1) {
        if (fgets(c, 10000, fp) == NULL)
            break;
        toke = (char *)strtok(c, " ");
        if (sscanf(toke, "%f", &azi) > 0) {
            if(dim == 3) {
                toke = (char *)strtok(NULL, " ");
                sscanf(toke, "%f", &ele);
            } else if (dim == 2) {
                ele = 0.0;
            }
        } else {
            break;
        }

        setup->azimuth[i] = azi;
        setup->elevation[i] = ele;
        i++;
        if (i == count)
            break;
    }
    setup->dimension = dim;
    setup->count = count;
    return setup;
}

void
build_speakers_list(speakers_setup *setup, ls lss[MAX_LS_AMOUNT]) {
    int i;
    ang_vec a_vector;
    cart_vec c_vector;
    for(i=0; i<setup->count; i++) {
        a_vector.azi = setup->azimuth[i];
        a_vector.ele = setup->elevation[i];
        vec_angle_to_cart(&a_vector, &c_vector);
        lss[i].coords.x = c_vector.x;
        lss[i].coords.y = c_vector.y;
        lss[i].coords.z = c_vector.z;
        lss[i].angles.azi = a_vector.azi;
        lss[i].angles.ele = a_vector.ele;
        lss[i].angles.length = 1.0;
    }
}


/* Selects the loudspeaker triplets, and calculates the inversion 
 * matrices for each selected triplet. A line (connection) is drawn 
 * between each loudspeaker. The lines denote the sides of the 
 * triangles. The triangles should not be intersecting. All crossing 
 * connections are searched and the longer connection is erased. 
 * This yields non-intesecting triangles, which can be used in panning.
 *
 * Used in test_def_speakers.
 */
void choose_ls_triplets(ls lss[MAX_LS_AMOUNT],
                        struct ls_triplet_chain **ls_triplets,
                        int ls_amount) {
    int i, j, k, l, m, li, table_size;
    int *i_ptr;
    cart_vec vb1, vb2, tmp_vec;
    int connections[MAX_LS_AMOUNT][MAX_LS_AMOUNT];
    float angles[MAX_LS_AMOUNT];
    int sorted_angles[MAX_LS_AMOUNT];
    float distance_table[((MAX_LS_AMOUNT * (MAX_LS_AMOUNT - 1)) / 2)];
    int distance_table_i[((MAX_LS_AMOUNT * (MAX_LS_AMOUNT - 1)) / 2)];
    int distance_table_j[((MAX_LS_AMOUNT * (MAX_LS_AMOUNT - 1)) / 2)];
    float distance;
    struct ls_triplet_chain *trip_ptr, *prev, *tmp_ptr;

    if (ls_amount == 0) {
        fprintf(stderr, "Number of loudspeakers is zero.\nExiting!\n");
        exit(-1);
    }
    for(i=0; i<ls_amount; i++) {
        for(j=i+1; j<ls_amount; j++) {
            for(k=j+1; k<ls_amount; k++) {
                if(vol_p_side_lgth(i, j, k, lss) > MIN_VOL_P_SIDE_LGTH) {
                    connections[i][j] = 1;
                    connections[j][i] = 1;
                    connections[i][k] = 1;
                    connections[k][i] = 1;
                    connections[j][k] = 1;
                    connections[k][j] = 1;
                    add_ldsp_triplet(i, j, k, ls_triplets, lss);
                }
            }
        }
    }

    /* Calculate distancies between all lss and sorting them. */
    table_size = (((ls_amount - 1) * (ls_amount)) / 2); 
    for(i=0; i<table_size; i++) {
        distance_table[i] = 100000.0;
    }
    for(i=0; i<ls_amount; i++) { 
        for(j=(i+1); j<ls_amount; j++) { 
            if(connections[i][j] == 1) {
                distance = fabs(vec_angle(lss[i].coords, lss[j].coords));
                k=0;
                while(distance_table[k] < distance) {
                    k++;
                }
                for (l=(table_size - 1); l > k ; l--) {
                    distance_table[l] = distance_table[l-1];
                    distance_table_i[l] = distance_table_i[l-1];
                    distance_table_j[l] = distance_table_j[l-1];
                }
                distance_table[k] = distance;
                distance_table_i[k] = i;
                distance_table_j[k] = j;
            } else {
                table_size--;
            }
        }
    }

    /* Disconnecting connections which are crossing shorter ones,
     * starting from shortest one and removing all that cross it,
     * and proceeding to next shortest.
     */
    for (i=0; i<(table_size); i++) {
        int fst_ls = distance_table_i[i];
        int sec_ls = distance_table_j[i];
        if (connections[fst_ls][sec_ls] == 1) {
            for(j=0; j<ls_amount; j++) {
                for(k=j+1; k<ls_amount; k++) {
                    if ((j!=fst_ls) && (k != sec_ls) && (k!=fst_ls) && (j != sec_ls)) {
                        if (lines_intersect(fst_ls, sec_ls, j, k, lss) == 1) {
                            connections[j][k] = 0;
                            connections[k][j] = 0;
                        }
                    }
                }
            }
        }
    }

    /* Remove triangles which had crossing sides with 
     * smaller triangles or include loudspeakers.
     */
    trip_ptr = *ls_triplets;
    prev = NULL;
    while (trip_ptr != NULL) {
        i = trip_ptr->ls_nos[0];
        j = trip_ptr->ls_nos[1];
        k = trip_ptr->ls_nos[2];
        if (connections[i][j] == 0 || 
            connections[i][k] == 0 || 
            connections[j][k] == 0 ||
            any_ls_inside_triplet(i, j, k, lss, ls_amount) == 1 ) {
            if (prev != NULL) {
                prev->next = trip_ptr->next;
                tmp_ptr = trip_ptr;
                trip_ptr = trip_ptr->next;
                free(tmp_ptr);
            } else {
                *ls_triplets = trip_ptr->next;
                tmp_ptr = trip_ptr;
                trip_ptr = trip_ptr->next;
                free(tmp_ptr);
            }
        } else {
            prev = trip_ptr;
            trip_ptr = trip_ptr->next;

        }
    }
}

/* Calculates the inverse matrices for 3D.
 *
 * After this call, ls_triplets contains the speakers numbers
 * and the inverse matrix needed to compute channel gains.
 *
 * Used in test_def_speakers.
 */
void calculate_3x3_matrixes(struct ls_triplet_chain *ls_triplets, 
                             ls lss[MAX_LS_AMOUNT], int ls_amount) {  
    float invdet;
    cart_vec *lp1, *lp2, *lp3;
    float *invmx;
    struct ls_triplet_chain *tr_ptr = ls_triplets;

    if (tr_ptr == NULL) {
        fprintf(stderr,"Not valid 3-D configuration.\n");
        exit(-1);
    }

     /* Calculations and data storage. */
    while(tr_ptr != NULL) {
        lp1 = &(lss[tr_ptr->ls_nos[0]].coords);
        lp2 = &(lss[tr_ptr->ls_nos[1]].coords);
        lp3 = &(lss[tr_ptr->ls_nos[2]].coords);

        /* Matrix inversion. */
        invmx = tr_ptr->inv_mx;
        invdet = 1.0 / (lp1->x * ((lp2->y * lp3->z) - (lp2->z * lp3->y))
                      - lp1->y * ((lp2->x * lp3->z) - (lp2->z * lp3->x))
                      + lp1->z * ((lp2->x * lp3->y) - (lp2->y * lp3->x)));

        invmx[0] = ((lp2->y * lp3->z) - (lp2->z * lp3->y)) * invdet;
        invmx[3] = ((lp1->y * lp3->z) - (lp1->z * lp3->y)) * -invdet;
        invmx[6] = ((lp1->y * lp2->z) - (lp1->z * lp2->y)) * invdet;
        invmx[1] = ((lp2->x * lp3->z) - (lp2->z * lp3->x)) * -invdet;
        invmx[4] = ((lp1->x * lp3->z) - (lp1->z * lp3->x)) * invdet;
        invmx[7] = ((lp1->x * lp2->z) - (lp1->z * lp2->x)) * -invdet;
        invmx[2] = ((lp2->x * lp3->y) - (lp2->y * lp3->x)) * invdet;
        invmx[5] = ((lp1->x * lp3->y) - (lp1->y * lp3->x)) * -invdet;
        invmx[8] = ((lp1->x * lp2->y) - (lp1->y * lp2->x)) * invdet;
        tr_ptr = tr_ptr->next;
    }
}

/* Selects the loudspeaker pairs, calculates the inversion
 * matrices and stores the data to a global array.
 *
 * Should return "float *ls_table" ?
 *
 * Used in test_def_speakers.
 */
void choose_ls_tuplets(ls lss[MAX_LS_AMOUNT], 
                       ls_triplet_chain **ls_triplets,
                       int ls_amount) {
    float atorad = (float)(2 * M_PI / 360);
    int i, j;
    int sorted_lss[MAX_LS_AMOUNT];
    int exist[MAX_LS_AMOUNT];
    float inv_mat[MAX_LS_AMOUNT][4];
    struct ls_triplet_chain *prev, *tr_ptr = *ls_triplets;
    prev = NULL;

    for(i=0; i<MAX_LS_AMOUNT; i++) {
        exist[i]=0;
    }

    /* Sort loudspeakers according their azimuth angle. */
    sort_2D_lss(lss, sorted_lss, ls_amount);

    /* Adjacent loudspeakers are the loudspeaker pairs to be used. */
    for(i=0; i<(ls_amount-1); i++) {
        if ((lss[sorted_lss[i+1]].angles.azi - 
             lss[sorted_lss[i]].angles.azi) <= (M_PI - 0.175)) {
            if (calc_2D_inv_tmatrix(lss[sorted_lss[i]].angles.azi, 
                                    lss[sorted_lss[i+1]].angles.azi, 
                                    inv_mat[i]) != 0) {
                exist[i] = 1;
            }
        }
    }

    if (((6.283 - lss[sorted_lss[ls_amount-1]].angles.azi) +
         lss[sorted_lss[0]].angles.azi) <= (M_PI - 0.175)) {
        if (calc_2D_inv_tmatrix(lss[sorted_lss[ls_amount-1]].angles.azi, 
                                lss[sorted_lss[0]].angles.azi, 
                                inv_mat[ls_amount-1]) != 0) { 
            exist[ls_amount-1] = 1;
        } 
    }

    for (i=0; i<ls_amount; i++) {
        if(exist[i] == 1) {
            while (tr_ptr != NULL) {
                prev = tr_ptr;
                tr_ptr = tr_ptr->next;
            }
            tr_ptr = (struct ls_triplet_chain *)malloc(sizeof(struct ls_triplet_chain));
            if (prev == NULL)
                *ls_triplets = tr_ptr;
            else
                prev->next = tr_ptr;
            tr_ptr->next = NULL;
            tr_ptr->ls_nos[0] = sorted_lss[i];
            tr_ptr->ls_nos[1] = sorted_lss[(i+1)%ls_amount];
            for(j=0; j<4; j++) {
                tr_ptr->inv_mx[j] = inv_mat[i][j];
            }
        }
    }
}

/*
 * To be implemented witout file reading...
 *
 * Used in test_def_speakers.
 */
void load_ls_triplets(ls lss[MAX_LS_AMOUNT], 
                      struct ls_triplet_chain **ls_triplets, 
                      int ls_amount, char *filename) 
{
    struct ls_triplet_chain *trip_ptr, *prev;
    int i, j, k;
    FILE *fp;
    char c[10000];
    char *toke;

    trip_ptr = *ls_triplets;
    prev = NULL;
    while (trip_ptr != NULL) {
        prev = trip_ptr;
        trip_ptr = trip_ptr->next;
    }

    if ((fp = fopen(filename, "r")) == NULL) {
        fprintf(stderr,"Could not open loudspeaker setup file.\n");
        exit(-1);
    }

    while(1) {
        if(fgets(c,10000,fp) == NULL)
            break;
        toke = (char *)strtok(c, " ");
        if(sscanf(toke, "%d",&i)>0) {
            toke = (char *)strtok(NULL," ");
            sscanf(toke, "%d",&j);
            toke = (char *)strtok(NULL," ");
            sscanf(toke, "%d",&k);
        } else {
            break;
        }

        trip_ptr = (struct ls_triplet_chain*)malloc(sizeof(struct ls_triplet_chain));
    
        if (prev == NULL)
            *ls_triplets = trip_ptr;
        else 
            prev->next = trip_ptr;
    
        trip_ptr->next = NULL;
        trip_ptr->ls_nos[0] = i-1;
        trip_ptr->ls_nos[1] = j-1;
        trip_ptr->ls_nos[2] = k-1;
        prev = trip_ptr;
        trip_ptr = NULL;
    }
}

/* Reads from specified file the loudspeaker triplet setup.
 * Returns number of dimension.
 *
 * Used in test_vbap.c.
 */
int read_ls_conf(FILE *fp, double lsm[MAX_TRIPLET_AMOUNT][9],
                 int lstripl[MAX_TRIPLET_AMOUNT][3],
                 int *triplet_amount) {
    int amount, i, j, a, b, d=0;
    int dimension;
    char *toke;
    char c[1000];
    double mx[9];
    fgets(c, 1000, fp);
    toke = (char *)strtok(c, " ");
    toke = (char *)strtok(NULL, " ");
    toke = (char *)strtok(NULL, " ");
    if ((toke = (char *)strtok(NULL, " ")) == NULL) {
        fprintf(stderr, "Wrong ls matrix file?\n");
        exit(-1);
    }
    sscanf(toke, "%d", &amount);
    toke = (char *)strtok(NULL, " ");
    toke = (char *)strtok(NULL, " ");
    if ((toke = (char *)strtok(NULL, " ")) == NULL) {
        fprintf(stderr, "Wrong ls matrix file?\n");
        exit(-1);
    }
    sscanf(toke, "%d",&dimension);
    printf("dim %d\n",dimension);
    *triplet_amount = amount;
    for (i=0; i<amount; i++) {
        fgets(c, 1000, fp);
        toke = (char *)strtok(c, " "); 
        if (strncmp(toke, "Trip", 4) != 0 && dimension == 3) {
            fprintf(stderr, "Something wrong in ls matrix file\n");
            exit(-1);
        }
        if (strncmp(toke, "Pair", 4) != 0 && dimension == 2) {
            fprintf(stderr, "Something wrong in ls matrix file\n");
            exit(-1);
        }
        toke = (char *)strtok(NULL, " "); 
        toke = (char *)strtok(NULL, " ");
        toke = (char *)strtok(NULL, " ");
        sscanf(toke, "%d", &a);
        lstripl[i][0] = a; 
        toke = (char *)strtok(NULL, " ");
        sscanf(toke, "%d", &b);
        lstripl[i][1] = b; 
        if (dimension == 3) {
            toke = (char *)strtok(NULL, " ");
            sscanf(toke, "%d", &d);
            lstripl[i][2] = d;
        }

        toke = (char *)strtok(NULL, " ");
        for (j=0; j<(dimension*dimension); j++) {
            toke = (char *)strtok(NULL, " ");
            sscanf(toke, "%lf", &(mx[j]));
            lsm[i][j] = mx[j];
        }
    }
    return dimension;
}

/* Calculates gain factors using loudspeaker setup and given direction. 
 *
 * Used in test_vbap.c.
 */
void vbap(double g[3], int ls[3], float x, float y, float z, int dimension,
          double lsm[MAX_TRIPLET_AMOUNT][9],
          int lstripl[MAX_TRIPLET_AMOUNT][3],
          int triplet_amount) {
    double *cartdir;
    double power;
    int i, j, k;
    double small_g;
    double big_sm_g, gtmp[3];
    int winner_triplet;

    cartdir = (double *)malloc(3 * sizeof(double));//angle_to_cart(azi, ele);
    cartdir[0]=x;
    cartdir[1]=y;
    cartdir[2]=z;
    
    big_sm_g = -100000.0;
    for (i=0; i<triplet_amount; i++) {
        small_g = 10000000.0;
        for (j=0; j<dimension; j++) {
            gtmp[j] = 0.0;
            for (k=0; k<dimension; k++)
                gtmp[j] += cartdir[k] * lsm[i][k+j*dimension]; 
            if (gtmp[j] < small_g)
                small_g = gtmp[j];
        }
        if (small_g > big_sm_g) {
            big_sm_g = small_g;
            winner_triplet = i;
            g[0] = gtmp[0];
            g[1] = gtmp[1]; 
            ls[0] = lstripl[i][0];
            ls[1] = lstripl[i][1]; 
            if (dimension == 3) {
                g[2] = gtmp[2];
                ls[2] = lstripl[i][2];
            } else {
                g[2] = 0.0;
                ls[2] = 0;
            }
        }
    }

    power=sqrt(g[0]*g[0] + g[1]*g[1] + g[2]*g[2]);
 
    g[0] /= power; 
    g[1] /= power;
    g[2] /= power;
    free(cartdir);
}

