/*
 * Functions for 3D VBAP processing based on work by Ville Pulkki.
 * (c) Ville Pulkki - 2.2.1999 Helsinki University of Technology.
 * Updated by belangeo, 2017.
 */

#include "vbap.hpp"

#include "narrow.hpp"

#include <cstdio>
#include <cstdlib>
#include <cstring>

/* Linked-list of all loudspeakers. */
struct ls_triplet_chain {
    int tripletSpeakerNumber[3];    /* Triplet speaker numbers */
    float tripletInverseMatrix[9];  /* Triplet inverse matrix */
    ls_triplet_chain * nextTriplet; /* Next triplet */
};

/* Fast-forward declarations. */
void compute_gains(int speaker_set_am,
                   SpeakerSet * sets,
                   float * gains,
                   int numSpeakers,
                   CartesianVector cart_dir,
                   int dim) noexcept;

/* Returns 1 if there is loudspeaker(s) inside given ls triplet. */
static bool any_speaker_inside_triplet(int const a,
                                       int const b,
                                       int const c,
                                       LoudSpeaker const speakers[MAX_SPEAKER_COUNT],
                                       int const numSpeakers) noexcept
{
    float inverseMatrix[9];

    auto const * const lp1 = &(speakers[a].coords);
    auto const * const lp2 = &(speakers[b].coords);
    auto const * const lp3 = &(speakers[c].coords);

    /* Matrix inversion. */
    auto const invdet
        = 1.0f
          / (lp1->x * ((lp2->y * lp3->z) - (lp2->z * lp3->y)) - lp1->y * ((lp2->x * lp3->z) - (lp2->z * lp3->x))
             + lp1->z * ((lp2->x * lp3->y) - (lp2->y * lp3->x)));

    inverseMatrix[0] = ((lp2->y * lp3->z) - (lp2->z * lp3->y)) * invdet;
    inverseMatrix[3] = ((lp1->y * lp3->z) - (lp1->z * lp3->y)) * -invdet;
    inverseMatrix[6] = ((lp1->y * lp2->z) - (lp1->z * lp2->y)) * invdet;
    inverseMatrix[1] = ((lp2->x * lp3->z) - (lp2->z * lp3->x)) * -invdet;
    inverseMatrix[4] = ((lp1->x * lp3->z) - (lp1->z * lp3->x)) * invdet;
    inverseMatrix[7] = ((lp1->x * lp2->z) - (lp1->z * lp2->x)) * -invdet;
    inverseMatrix[2] = ((lp2->x * lp3->y) - (lp2->y * lp3->x)) * invdet;
    inverseMatrix[5] = ((lp1->x * lp3->y) - (lp1->y * lp3->x)) * -invdet;
    inverseMatrix[8] = ((lp1->x * lp2->y) - (lp1->y * lp2->x)) * invdet;

    for (int i{}; i < numSpeakers; ++i) {
        if (i != a && i != b && i != c) {
            auto this_inside{ true };
            for (int j{}; j < 3; ++j) {
                auto tmp = speakers[i].coords.x * inverseMatrix[0 + j * 3];
                tmp += speakers[i].coords.y * inverseMatrix[1 + j * 3];
                tmp += speakers[i].coords.z * inverseMatrix[2 + j * 3];
                if (tmp < -0.001f) {
                    this_inside = false;
                    break;
                }
            }
            if (this_inside) {
                return true;
            }
        }
    }
    return false;
}

/* Properly free an automatically allocated ls_triplet_chain linked-list.
 */
void free_ls_triplet_chain(ls_triplet_chain * const ls_triplets) noexcept
{
    ls_triplet_chain * ptr = ls_triplets;
    while (ptr != nullptr) {
        ls_triplet_chain * const next = ptr->nextTriplet;
        delete ptr;
        ptr = next;
    }
}

/* Adds i, j, k triplet to triplet chain. */
static void add_ldsp_triplet(int const i, int const j, int const k, ls_triplet_chain ** const speakerTriplets) noexcept
{
    auto * trip_ptr = *speakerTriplets;
    ls_triplet_chain * prev{};

    while (trip_ptr != nullptr) {
        prev = trip_ptr;
        trip_ptr = trip_ptr->nextTriplet;
    }
    trip_ptr = new ls_triplet_chain;
    if (prev == nullptr) {
        *speakerTriplets = trip_ptr;
    } else {
        prev->nextTriplet = trip_ptr;
    }
    trip_ptr->nextTriplet = nullptr;
    trip_ptr->tripletSpeakerNumber[0] = i;
    trip_ptr->tripletSpeakerNumber[1] = j;
    trip_ptr->tripletSpeakerNumber[2] = k;
}

/* Checks if two lines intersect on 3D sphere see theory in paper
 * Pulkki, V. Lokki, T. "Creating Auditory Displays with Multiple
 * Loudspeakers Using VBAP: A Case Study with DIVA Project" in
 * International Conference on Auditory Displays -98.
 * E-mail Ville.Pulkki@hut.fi if you want to have that paper. */
static int lines_intersect(int const i,
                           int const j,
                           int const k,
                           int const l,
                           LoudSpeaker const speakers[MAX_SPEAKER_COUNT]) noexcept
{
    auto const v1{ speakers[i].coords.crossProduct(speakers[j].coords) };
    auto const v2{ speakers[k].coords.crossProduct(speakers[l].coords) };
    auto const v3{ v1.crossProduct(v2) };
    auto const negV3{ -v3 };

    auto const dist_ij = speakers[i].coords.angleWith(speakers[j].coords);
    auto const dist_kl = speakers[k].coords.angleWith(speakers[l].coords);
    auto const dist_iv3 = speakers[i].coords.angleWith(v3);
    auto const dist_jv3 = v3.angleWith(speakers[j].coords);
    auto const dist_inv3 = speakers[i].coords.angleWith(negV3);
    auto const dist_jnv3 = negV3.angleWith(speakers[j].coords);
    auto const dist_kv3 = speakers[k].coords.angleWith(v3);
    auto const dist_lv3 = v3.angleWith(speakers[l].coords);
    auto const dist_knv3 = speakers[k].coords.angleWith(negV3);
    auto const dist_lnv3 = negV3.angleWith(speakers[l].coords);

    /*One of loudspeakers is close to crossing point, don't do anything.*/
    if (std::abs(dist_iv3) <= 0.01 || std::abs(dist_jv3) <= 0.01 || std::abs(dist_kv3) <= 0.01
        || std::abs(dist_lv3) <= 0.01 || std::abs(dist_inv3) <= 0.01 || std::abs(dist_jnv3) <= 0.01
        || std::abs(dist_knv3) <= 0.01 || std::abs(dist_lnv3) <= 0.01) {
        return (0);
    }

    if (((std::abs(dist_ij - (dist_iv3 + dist_jv3)) <= 0.01) && (std::abs(dist_kl - (dist_kv3 + dist_lv3)) <= 0.01))
        || ((std::abs(dist_ij - (dist_inv3 + dist_jnv3)) <= 0.01)
            && (std::abs(dist_kl - (dist_knv3 + dist_lnv3)) <= 0.01))) {
        return (1);
    }
    return (0);
}

/* subroutine for spreading */
static CartesianVector newSpreadDir(CartesianVector const vsCartesianDir,
                                    CartesianVector spreadBase,
                                    float const azimuth,
                                    float const spread)
{
    auto sum = std::clamp(vsCartesianDir.dotProduct(spreadBase), -1.0f, 1.0f);
    auto gamma = std::acos(sum) / juce::MathConstants<float>::pi * 180.0f;
    if (std::abs(gamma) < 1.0f) {
        AngularVector const tmp{ azimuth + 90.0f, 0.0f, 1.0f };
        spreadBase = tmp.toCartesian();
        sum = std::clamp(vsCartesianDir.dotProduct(spreadBase), -1.0f, 1.0f);
        gamma = std::acos(sum) / juce::MathConstants<float>::pi * 180.0f;
    }
    auto const beta = 180.0f - gamma;
    auto const b = fast::sin(spread * juce::MathConstants<float>::pi / 180.0f)
                   / fast::sin(beta * juce::MathConstants<float>::pi / 180.0f);
    auto const a = fast::sin((180.0f - spread - beta) * juce::MathConstants<float>::pi / 180.0f)
                   / fast::sin(beta * juce::MathConstants<float>::pi / 180.0f);
    auto const x{ a * vsCartesianDir.x + b * spreadBase.x };
    auto const y{ a * vsCartesianDir.y + b * spreadBase.y };
    auto const z{ a * vsCartesianDir.z + b * spreadBase.z };
    CartesianVector const unscaledResult{ x, y, z };
    auto const power = std::sqrt(unscaledResult.dotProduct(unscaledResult));
    auto const result{ unscaledResult / power };

    return result;
}

/* subroutine for spreading */
static CartesianVector
    newSpreadBase(CartesianVector const spreadDir, CartesianVector const vscartdir, float const spread)
{
    auto const d = fast::cos(spread / 180.0f * juce::MathConstants<float>::pi);

    auto const x{ spreadDir.x - d * vscartdir.x };
    auto const y{ spreadDir.y - d * vscartdir.y };
    auto const z{ spreadDir.z - d * vscartdir.z };
    CartesianVector const unscaledResult{ x, y, z };

    auto const power = std::sqrt(unscaledResult.dotProduct(unscaledResult));
    auto const result{ unscaledResult / power };

    return result;
}

/*
 * apply the sound signal to multiple panning directions
 * that causes some spreading.
 * See theory in paper V. Pulkki "Uniform spreading of amplitude panned
 * virtual sources" in WASPAA 99
 */
static void spreadIt(float const azimuth, float const spread, VbapData * const data) noexcept
{
    /* four orthogonal dirs */
    CartesianVector spreadDir[16];
    CartesianVector spreadBase[16];
    spreadDir[0] = newSpreadDir(data->cartesianDirection, data->spreadingVector, azimuth, spread);
    data->spreadingVector = newSpreadBase(spreadDir[0], data->cartesianDirection, spread);
    spreadBase[1] = data->spreadingVector.crossProduct(data->cartesianDirection);
    spreadBase[2] = spreadBase[1].crossProduct(data->cartesianDirection);
    spreadBase[3] = spreadBase[2].crossProduct(data->cartesianDirection);

    /* four between them */
    spreadBase[4] = data->spreadingVector.mean(spreadBase[1]);
    spreadBase[5] = spreadBase[1].mean(spreadBase[2]);
    spreadBase[6] = spreadBase[2].mean(spreadBase[3]);
    spreadBase[7] = spreadBase[3].mean(data->spreadingVector);

    /* four at half spread angle */
    spreadBase[8] = data->cartesianDirection.mean(data->spreadingVector);
    spreadBase[9] = data->cartesianDirection.mean(spreadBase[1]);
    spreadBase[10] = data->cartesianDirection.mean(spreadBase[2]);
    spreadBase[11] = data->cartesianDirection.mean(spreadBase[3]);

    /* four at quarter spread angle */
    spreadBase[12] = data->cartesianDirection.mean(spreadBase[8]);
    spreadBase[13] = data->cartesianDirection.mean(spreadBase[9]);
    spreadBase[14] = data->cartesianDirection.mean(spreadBase[10]);
    spreadBase[15] = data->cartesianDirection.mean(spreadBase[11]);

    std::array<float, MAX_SPEAKER_COUNT> tmpGain{};
    int const cnt = data->numSpeakers;
    static constexpr auto SPREAD_DIR_NUM = 16;
    for (auto i = 1; i < SPREAD_DIR_NUM; ++i) {
        spreadDir[i] = newSpreadDir(data->cartesianDirection, spreadBase[i], azimuth, spread);
        compute_gains(data->numTriplets,
                      data->speakerSets,
                      tmpGain.data(),
                      data->numSpeakers,
                      spreadDir[i],
                      data->dimension);
        for (int j{}; j < cnt; ++j) {
            data->gains[j] += tmpGain[j];
        }
    }

    if (spread > 70.0) {
        for (int i{}; i < cnt; ++i) {
            data->gains[i] += (spread - 70.0f) / 30.0f * (spread - 70.0f) / 30.0f * 20.0f;
        }
    }
    float sum{};
    for (int i{}; i < cnt; ++i) {
        sum += (data->gains[i] * data->gains[i]);
    }
    sum = std::sqrt(sum);
    for (int i{}; i < cnt; ++i) {
        data->gains[i] /= sum;
    }
}

static void spreadit_azi_ele(float /*azi*/, float /*ele*/, float sp_azi, float sp_ele, VbapData * data) noexcept
{
    int ind;
    static constexpr auto num = 4;
    float newAzimuth;
    float newElevation;
    float comp;
    int const cnt = data->numSpeakers;
    std::array<float, MAX_SPEAKER_COUNT> tmp_gains{};
    float sum = 0.0f;

    sp_azi = std::clamp(sp_azi, 0.0f, 1.0f);
    sp_ele = std::clamp(sp_ele, 0.0f, 1.0f);

    // If both sp_azi and sp_ele are active, we want to put a virtual source at
    // (azi, ele +/- eledev) and (azi +/- azidev, ele) locations.
    auto const kNum{ sp_azi > 0.0 && sp_ele > 0.0 ? 8 : 4 };

    for (int i{}; i < num; ++i) {
        auto const iFloat{ static_cast<float>(i) };
        comp = std::pow(10.0f, (iFloat + 1.0f) * -3.0f * 0.05f);
        auto const azimuthDev = (iFloat + 1.0f) * sp_azi * 45.0f;
        auto const elevationDev = (iFloat + 1.0f) * sp_ele * 22.5f;
        for (int k{}; k < kNum; ++k) {
            switch (k) {
            case 0:
                newAzimuth = data->angularDirection.azimuth + azimuthDev;
                newElevation = data->angularDirection.elevation + elevationDev;
                break;
            case 1:
                newAzimuth = data->angularDirection.azimuth - azimuthDev;
                newElevation = data->angularDirection.elevation - elevationDev;
                break;
            case 2:
                newAzimuth = data->angularDirection.azimuth + azimuthDev;
                newElevation = data->angularDirection.elevation - elevationDev;
                break;
            case 3:
                newAzimuth = data->angularDirection.azimuth - azimuthDev;
                newElevation = data->angularDirection.elevation + elevationDev;
                break;
            case 4:
                newAzimuth = data->angularDirection.azimuth;
                newElevation = data->angularDirection.elevation + elevationDev;
                break;
            case 5:
                newAzimuth = data->angularDirection.azimuth;
                newElevation = data->angularDirection.elevation - elevationDev;
                break;
            case 6:
                newAzimuth = data->angularDirection.azimuth + azimuthDev;
                newElevation = data->angularDirection.elevation;
                break;
            case 7:
                newAzimuth = data->angularDirection.azimuth - azimuthDev;
                newElevation = data->angularDirection.elevation;
                break;
            default:
                jassertfalse;
            }

            if (newAzimuth > 180) {
                newAzimuth -= 360;
            } else if (newAzimuth < -180) {
                newAzimuth += 360;
            }
            if (newElevation > 90) {
                newElevation = 90;
            } else if (newElevation < 0) {
                newElevation = 0;
            }
            AngularVector const spreadAngle{ newAzimuth, newElevation, 1.0f };
            auto const spreadCartesian{ spreadAngle.toCartesian() };
            compute_gains(data->numTriplets,
                          data->speakerSets,
                          tmp_gains.data(),
                          data->numSpeakers,
                          spreadCartesian,
                          data->dimension);
            for (int j{}; j < cnt; ++j) {
                data->gains[j] += tmp_gains[j] * comp;
            }
        }
    }

    if (sp_azi > 0.8f && sp_ele > 0.8f) {
        comp = (sp_azi - 0.8f) / 0.2f * (sp_ele - 0.8f) / 0.2f * 10.0f;
        for (int i{}; i < data->numOutputPatches; ++i) {
            data->gains[data->outputPatches[i] - 1] += comp;
        }
    }

    for (int i{}; i < data->numOutputPatches; ++i) {
        ind = data->outputPatches[i] - 1;
        sum += (data->gains[ind] * data->gains[ind]);
    }
    sum = std::sqrt(sum);
    for (int i{}; i < data->numOutputPatches; ++i) {
        ind = data->outputPatches[i] - 1;
        data->gains[ind] /= sum;
    }
}

static void
    spreadit_azi_ele_flip_y_z(float /*azi*/, float /*ele*/, float sp_azi, float sp_ele, VbapData * const data) noexcept
{
    float newAzimuth{};
    float newElevation{};
    float comp;
    auto const cnt = data->numSpeakers;
    std::array<float, MAX_SPEAKER_COUNT> tmp_gains{};
    float sum = 0.0;

    sp_azi = std::clamp(sp_azi, 0.0f, 1.0f);
    sp_azi = std::clamp(sp_ele, 0.0f, 1.0f);

    // If both sp_azi and sp_ele are active, we want to put a virtual source at
    // (azi, ele +/- eledev) and (azi +/- azidev, ele) locations.
    auto const kNum{ sp_azi > 0.0f && sp_ele > 0.0f ? 8 : 4 };

    static constexpr auto NUM = 4;
    for (int i{}; i < NUM; ++i) {
        auto const iFloat{ static_cast<float>(i) };
        comp = std::pow(10.0f, (iFloat + 1.0f) * -3.0f * 0.05f);
        auto const azimuthDev = (iFloat + 1.0f) * sp_azi * 45.0f;
        auto const elevationDev = (iFloat + 1.0f) * sp_ele * 22.5f;
        for (int k{}; k < kNum; ++k) {
            if (k == 0) {
                newAzimuth = data->angularDirection.azimuth + azimuthDev;
                newElevation = data->angularDirection.elevation + elevationDev;
            } else if (k == 1) {
                newAzimuth = data->angularDirection.azimuth - azimuthDev;
                newElevation = data->angularDirection.elevation - elevationDev;
            } else if (k == 2) {
                newAzimuth = data->angularDirection.azimuth + azimuthDev;
                newElevation = data->angularDirection.elevation - elevationDev;
            } else if (k == 3) {
                newAzimuth = data->angularDirection.azimuth - azimuthDev;
                newElevation = data->angularDirection.elevation + elevationDev;
            } else if (k == 4) {
                newAzimuth = data->angularDirection.azimuth;
                newElevation = data->angularDirection.elevation + elevationDev;
            } else if (k == 5) {
                newAzimuth = data->angularDirection.azimuth;
                newElevation = data->angularDirection.elevation - elevationDev;
            } else if (k == 6) {
                newAzimuth = data->angularDirection.azimuth + azimuthDev;
                newElevation = data->angularDirection.elevation;
            } else if (k == 7) {
                newAzimuth = data->angularDirection.azimuth - azimuthDev;
                newElevation = data->angularDirection.elevation;
            }
            if (newAzimuth > 180) {
                newAzimuth -= 360;
            } else if (newAzimuth < -180) {
                newAzimuth += 360;
            }
            if (newElevation > 90) {
                newElevation = 90;
            } else if (newElevation < 0) {
                newElevation = 0;
            }
            AngularVector const spreadAngle{ newAzimuth, newElevation, 1.0f };
            CartesianVector spreadCartesian = spreadAngle.toCartesian();
            auto const tmp = spreadCartesian.z;
            spreadCartesian.z = spreadCartesian.y;
            spreadCartesian.y = tmp;
            compute_gains(data->numTriplets,
                          data->speakerSets,
                          tmp_gains.data(),
                          data->numSpeakers,
                          spreadCartesian,
                          data->dimension);
            for (int j{}; j < cnt; ++j) {
                data->gains[j] += tmp_gains[j] * comp;
            }
        }
    }

    if (sp_azi > 0.8f && sp_ele > 0.8f) {
        comp = (sp_azi - 0.8f) / 0.2f * (sp_ele - 0.8f) / 0.2f * 10.0f;
        for (int i{}; i < data->numOutputPatches; ++i) {
            data->gains[data->outputPatches[i] - 1] += comp;
        }
    }

    for (int i{}; i < data->numOutputPatches; ++i) {
        auto const index = data->outputPatches[i] - 1;
        sum += (data->gains[index] * data->gains[index]);
    }
    sum = std::sqrt(sum);
    for (int i{}; i < data->numOutputPatches; ++i) {
        auto const index = data->outputPatches[i] - 1;
        data->gains[index] /= sum;
    }
}

static void spreadit_azi(float /*azi*/, float azimuthSpread, VbapData * data)
{
    int i;
    static constexpr auto num = 4;
    float newazi{};
    AngularVector spreadang;
    int cnt = data->numSpeakers;
    std::array<float, MAX_SPEAKER_COUNT> tmp_gains{};
    float sum = 0.0;

    if (azimuthSpread < 0.0) {
        azimuthSpread = 0.0;
    } else if (azimuthSpread > 1.0) {
        azimuthSpread = 1.0;
    }

    for (i = 0; i < num; i++) {
        float comp = std::pow(10.0f, (i + 1) * -3.0f * 0.05f);
        float azidev = (i + 1) * azimuthSpread * 45.0f;
        for (int k = 0; k < 2; k++) {
            if (k == 0) {
                newazi = data->angularDirection.azimuth + azidev;
            } else if (k == 1) {
                newazi = data->angularDirection.azimuth - azidev;
            }
            if (newazi > 180) {
                newazi -= 360;
            } else if (newazi < -180) {
                newazi += 360;
            }
            spreadang.azimuth = newazi;
            spreadang.elevation = 0.0;
            spreadang.length = 1.0;
            CartesianVector spreadcart = spreadang.toCartesian();
            compute_gains(data->numTriplets,
                          data->speakerSets,
                          tmp_gains.data(),
                          data->numSpeakers,
                          spreadcart,
                          data->dimension);
            for (int j = 0; j < cnt; j++) {
                data->gains[j] += (tmp_gains[j] * comp);
            }
        }
    }

    for (i = 0; i < cnt; i++) {
        sum += (data->gains[i] * data->gains[i]);
    }
    sum = std::sqrt(sum);
    for (i = 0; i < cnt; i++) {
        data->gains[i] /= sum;
    }
}

static void spreadit_azi_flip_y_z(float /*azi*/, float sp_azi, VbapData * data)
{
    int i;
    static constexpr auto NUM = 4;
    float newazi;
    float comp;
    float tmp;
    AngularVector spreadang;
    CartesianVector spreadcart;
    int const cnt = data->numSpeakers;
    std::array<float, MAX_SPEAKER_COUNT> tmp_gains{};
    float sum = 0.0;

    sp_azi = std::clamp(sp_azi, 0.0f, 1.0f);

    for (i = 0; i < NUM; i++) {
        comp = std::pow(10.0f, (i + 1) * -3.0f * 0.05f);
        float azidev = (i + 1) * sp_azi * 45.0f;
        for (int k = 0; k < 2; k++) {
            if (k == 0) {
                newazi = data->angularDirection.azimuth + azidev;
            } else if (k == 1) {
                newazi = data->angularDirection.azimuth - azidev;
            }
            if (newazi > 180) {
                newazi -= 360;
            } else if (newazi < -180) {
                newazi += 360;
            }
            spreadang.azimuth = newazi;
            spreadang.elevation = 0.0;
            spreadang.length = 1.0;
            spreadcart = spreadang.toCartesian();
            tmp = spreadcart.z;
            spreadcart.z = spreadcart.y;
            spreadcart.y = tmp;
            compute_gains(data->numTriplets,
                          data->speakerSets,
                          tmp_gains.data(),
                          data->numSpeakers,
                          spreadcart,
                          data->dimension);
            for (int j = 0; j < cnt; j++) {
                data->gains[j] += (tmp_gains[j] * comp);
            }
        }
    }

    for (i = 0; i < cnt; i++) {
        sum += (data->gains[i] * data->gains[i]);
    }
    sum = std::sqrt(sum);
    for (i = 0; i < cnt; i++) {
        data->gains[i] /= sum;
    }
}

void free_speakers_setup(SpeakersSetup * const setup) noexcept
{
    free(setup->azimuth);
    free(setup->elevation);
    free(setup);
}

SpeakersSetup *
    load_speakers_setup(int const count, float const * const azimuth, float const * const elevation) noexcept
{
    auto * setup = new SpeakersSetup;

    if (count < 3) {
        fprintf(stderr, "Too few loudspeakers %d\n", count);
        free(setup);
        exit(-1);
    }

    setup->azimuth = static_cast<float *>(calloc(count, sizeof(float)));
    setup->elevation = static_cast<float *>(calloc(count, sizeof(float)));
    for (int i{}; i < count; ++i) {
        setup->azimuth[i] = azimuth[i];
        setup->elevation[i] = elevation[i];
    }
    setup->dimension = 3;
    setup->count = count;
    return setup;
}

SpeakersSetup * load_speakers_setup_from_file(char const * const filename) noexcept
{
    int i = 0;
    int count;
    float azi;
    float ele;
    char c[10000];
    FILE * fp;
    auto * setup = new SpeakersSetup;

    if ((fp = fopen(filename, "r")) == nullptr) {
        fprintf(stderr, "Could not open loudspeaker setup file.\n");
        free(setup);
        exit(-1);
    }

    fgets(c, 10000, fp);
    char * toke = (char *)strtok(c, " ");
    sscanf(toke, "%d", &count);
    if (count < 3) {
        fprintf(stderr, "Too few loudspeakers %d\n", count);
        free(setup);
        exit(-1);
    }

    setup->azimuth = (float *)calloc(count, sizeof(float));
    setup->elevation = (float *)calloc(count, sizeof(float));
    while (1) {
        if (fgets(c, 10000, fp) == nullptr)
            break;
        toke = (char *)strtok(c, " ");
        if (sscanf(toke, "%f", &azi) > 0) {
            toke = (char *)strtok(nullptr, " ");
            sscanf(toke, "%f", &ele);
        } else {
            break;
        }

        setup->azimuth[i] = azi;
        setup->elevation[i] = ele;
        i++;
        if (i == count)
            break;
    }
    setup->dimension = 3;
    setup->count = count;
    return setup;
}

/* Build a speakers list from a SPEAKERS_SETUP structure.
 */
void build_speakers_list(SpeakersSetup * setup, LoudSpeaker speakers[MAX_SPEAKER_COUNT])
{
    for (int i = 0; i < setup->count; i++) {
        AngularVector a_vector;
        a_vector.azimuth = setup->azimuth[i];
        a_vector.elevation = setup->elevation[i];
        auto const c_vector = a_vector.toCartesian();
        speakers[i].coords = c_vector;
        speakers[i].angles.azimuth = a_vector.azimuth;
        speakers[i].angles.elevation = a_vector.elevation;
        speakers[i].angles.length = 1.0f;
    }
}

/*
 * No external use.
 */
void sort_2D_lss(LoudSpeaker lss[MAX_SPEAKER_COUNT], int sorted_lss[MAX_SPEAKER_COUNT], int ls_amount)
{
    int i, j, index = 0;
    float tmp, tmp_azi;

    /* Transforming angles between -180 and 180. */
    for (i = 0; i < ls_amount; i++) {
        lss[i].coords = lss[i].angles.toCartesian();
        lss[i].angles.azimuth = std::acos(lss[i].coords.x);
        if (std::abs(lss[i].coords.y) <= 0.001)
            tmp = 1.0;
        else
            tmp = lss[i].coords.y / std::abs(lss[i].coords.y);
        lss[i].angles.azimuth *= tmp;
    }
    for (i = 0; i < ls_amount; i++) {
        tmp = 2000;
        for (j = 0; j < ls_amount; j++) {
            if (lss[j].angles.azimuth <= tmp) {
                tmp = lss[j].angles.azimuth;
                index = j;
            }
        }
        sorted_lss[i] = index;
        tmp_azi = lss[index].angles.azimuth;
        lss[index].angles.azimuth = tmp_azi + 4000.0f;
    }
    for (i = 0; i < ls_amount; i++) {
        tmp_azi = lss[i].angles.azimuth;
        lss[i].angles.azimuth = tmp_azi - 4000.0f;
    }
}

/*
 * No external use.
 */
int calc_2D_inv_tmatrix(float azi1, float azi2, float inv_mat[4])
{
    auto const x1 = fast::cos(azi1);
    auto const x2 = fast::sin(azi1);
    auto const x3 = fast::cos(azi2);
    auto const x4 = fast::sin(azi2);
    auto const det = (x1 * x4) - (x3 * x2);
    if (std::abs(det) <= 0.001) {
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

/* Selects the loudspeaker pairs, calculates the inversion
 * matrices and stores the data to a global array.
 */
void choose_ls_tuplets(LoudSpeaker lss[MAX_SPEAKER_COUNT], ls_triplet_chain ** ls_triplets, int ls_amount)
{
    int i, j, amount = 0;
    int sorted_lss[MAX_SPEAKER_COUNT];
    int exist[MAX_SPEAKER_COUNT];
    float inv_mat[MAX_SPEAKER_COUNT][4];
    struct ls_triplet_chain *prev, *tr_ptr = *ls_triplets;
    prev = nullptr;

    for (i = 0; i < MAX_SPEAKER_COUNT; i++) {
        exist[i] = 0;
    }

    /* Sort loudspeakers according their azimuth angle. */
    sort_2D_lss(lss, sorted_lss, ls_amount);

    /* Adjacent loudspeakers are the loudspeaker pairs to be used. */
    for (i = 0; i < (ls_amount - 1); i++) {
        if ((lss[sorted_lss[i + 1]].angles.azimuth - lss[sorted_lss[i]].angles.azimuth)
            <= (juce::MathConstants<float>::pi - 0.175f)) {
            if (calc_2D_inv_tmatrix(lss[sorted_lss[i]].angles.azimuth,
                                    lss[sorted_lss[i + 1]].angles.azimuth,
                                    inv_mat[i])
                != 0) {
                exist[i] = 1;
                amount++;
            }
        }
    }

    if (((juce::MathConstants<float>::twoPi - lss[sorted_lss[ls_amount - 1]].angles.azimuth)
         + lss[sorted_lss[0]].angles.azimuth)
        <= (juce::MathConstants<float>::pi - 0.175f)) {
        if (calc_2D_inv_tmatrix(lss[sorted_lss[ls_amount - 1]].angles.azimuth,
                                lss[sorted_lss[0]].angles.azimuth,
                                inv_mat[ls_amount - 1])
            != 0) {
            exist[ls_amount - 1] = 1;
            amount++;
        }
    }

    for (i = 0; i < ls_amount - 1; i++) {
        if (exist[i] == 1) {
            while (tr_ptr != nullptr) {
                prev = tr_ptr;
                tr_ptr = tr_ptr->nextTriplet;
            }
            tr_ptr = new ls_triplet_chain;
            if (prev == nullptr)
                *ls_triplets = tr_ptr;
            else
                prev->nextTriplet = tr_ptr;
            tr_ptr->nextTriplet = nullptr;
            tr_ptr->tripletSpeakerNumber[0] = sorted_lss[i] + 1;
            tr_ptr->tripletSpeakerNumber[1] = sorted_lss[i + 1] + 1;
            for (j = 0; j < 4; j++) {
                tr_ptr->tripletInverseMatrix[j] = inv_mat[i][j];
            }
        }
    }

    if (exist[ls_amount - 1] == 1) {
        while (tr_ptr != nullptr) {
            prev = tr_ptr;
            tr_ptr = tr_ptr->nextTriplet;
        }
        tr_ptr = new ls_triplet_chain;
        if (prev == nullptr)
            *ls_triplets = tr_ptr;
        else
            prev->nextTriplet = tr_ptr;
        tr_ptr->nextTriplet = nullptr;
        tr_ptr->tripletSpeakerNumber[0] = sorted_lss[ls_amount - 1] + 1;
        tr_ptr->tripletSpeakerNumber[1] = sorted_lss[0] + 1;
        for (j = 0; j < 4; j++) {
            tr_ptr->tripletInverseMatrix[j] = inv_mat[ls_amount - 1][j];
        }
    }
}

///* Calculate volume of the parallelepiped defined by the loudspeaker
// * direction vectors and divide it with total length of the triangle sides.
// * This is used when removing too narrow triangles. */
// static float
// vol_p_side_lgth(LoudSpeaker const & i, LoudSpeaker const & j, LoudSpeaker const & k) noexcept
//{
//    auto const length = i.coords.angleWith(j.coords)
//        + i.coords.angleWith(k.coords)
//        + j.coords.angleWith(k.coords);
//    /* At least, two speakers should be on the same elevation plane.
//       This fix wrong triplet in the Dome32Sub2UdeM template. */
//    auto const hasTwoSpeakersOnSameElevation = i.angles.isOnSameElevation(j.angles)
//        || i.angles.isOnSameElevation(k.angles)
//        || j.angles.isOnSameElevation(k.angles);
//    if (length <= 0.00001f || !hasTwoSpeakersOnSameElevation) {
//        return 0.0f;
//    }
//    auto const xProduct{ i.coords.crossProduct(j.coords) };
//    auto const volper = std::abs(xProduct.dotProduct(k.coords));
//
//    return volper / length;
//}

/* Calculate volume of the parallelepiped defined by the loudspeaker
 * direction vectors and divide it with total length of the triangle sides.
 * This is used when removing too narrow triangles. */
static float
    vol_p_side_lgth(int const i, int const j, int const k, LoudSpeaker const speakers[MAX_SPEAKER_COUNT]) noexcept
{
    auto const length = speakers[i].coords.angleWith(speakers[j].coords)
                        + speakers[i].coords.angleWith(speakers[k].coords)
                        + speakers[j].coords.angleWith(speakers[k].coords);
    /* At least, two speakers should be on the same elevation plane.
       This fix wrong triplet in the Dome32Sub2UdeM template. */
    auto const hasTwoSpeakersOnSameElevation = speakers[i].angles.isOnSameElevation(speakers[j].angles)
                                               || speakers[i].angles.isOnSameElevation(speakers[k].angles)
                                               || speakers[j].angles.isOnSameElevation(speakers[k].angles);
    if (length <= 0.00001f || !hasTwoSpeakersOnSameElevation) {
        return 0.0f;
    }
    auto const xProduct{ speakers[i].coords.crossProduct(speakers[j].coords) };
    auto const volper = std::abs(xProduct.dotProduct(speakers[k].coords));

    return volper / length;
}

/* Selects the loudspeaker triplets, and calculates the inversion
 * matrices for each selected triplet. A line (connection) is drawn
 * between each loudspeaker. The lines denote the sides of the
 * triangles. The triangles should not be intersecting. All crossing
 * connections are searched and the longer connection is erased.
 * This yields non-intersecting triangles, which can be used in panning.
 */
void choose_ls_triplets(LoudSpeaker const speakers[MAX_SPEAKER_COUNT],
                        ls_triplet_chain ** const speakerTriplets,
                        int const numSpeakers) noexcept
{
    static constexpr size_t MAGIC = MAX_SPEAKER_COUNT * (MAX_SPEAKER_COUNT - 1) / 2;

    int connections[MAX_SPEAKER_COUNT][MAX_SPEAKER_COUNT];

    float distance_table[MAGIC];
    int distance_table_i[MAGIC];
    int distance_table_j[MAGIC];
    ls_triplet_chain * tmp_ptr;

    if (numSpeakers == 0) {
        fprintf(stderr, "Number of loudspeakers is zero.\nExiting!\n");
        exit(-1);
    }

    /*
     * Ok, so the next part of the algorithm has to check vol_p_side_lgth() for EVERY possible speaker triplet. This
     * takes an absurd amount of time for setups bigger than 100 speakers.
     *
     * Luckily, at least two speakers have to be at a similar elevation for the triplet to be valid. Instead of looking
     * for this inside vol_p_side_lgth(), we can take advantage of this fact to reduce the search space :
     *
     * 1- Sort all the speaker indexes according to their elevation.
     *
     * 2- Select every pair of speaker that is within the maximum elevation range.
     *
     * 3- Check for vol_p_side_lgth()
     */

    // 1- Sort all the speaker indexes according to their elevation.

    // we first build an array with all the indexes
    std::vector<size_t> speakerIndexesSortedByElevation{ narrow<size_t>(numSpeakers) };
    std::iota(std::begin(speakerIndexesSortedByElevation), std::end(speakerIndexesSortedByElevation), 0);

    // we then sort it
    auto const compare = [speakers](size_t const & indexA, size_t const & indexB) -> bool {
        return speakers[indexA].angles.elevation < speakers[indexB].angles.elevation;
    };
    std::sort(std::begin(speakerIndexesSortedByElevation), std::end(speakerIndexesSortedByElevation), compare);

    // 2 - Select every pair of speaker that is within the maximum elevation range.

    // TODO : this is fucking unacceptable
    for (int i{}; i < numSpeakers; ++i) {
        for (auto j = i + 1; j < numSpeakers; ++j) {
            for (auto k = j + 1; k < numSpeakers; ++k) {
                if (vol_p_side_lgth(i, j, k, speakers) > MIN_VOL_P_SIDE_LENGTH) {
                    connections[i][j] = 1;
                    connections[j][i] = 1;
                    connections[i][k] = 1;
                    connections[k][i] = 1;
                    connections[j][k] = 1;
                    connections[k][j] = 1;
                    add_ldsp_triplet(i, j, k, speakerTriplets);
                }
            }
        }
    }

    /* Calculate distances between all lss and sorting them. */
    auto table_size = (((numSpeakers - 1) * (numSpeakers)) / 2);
    for (int i{}; i < table_size; i++) {
        distance_table[i] = 100000.0f;
    }

    for (int i{}; i < numSpeakers; i++) {
        for (auto j = (i + 1); j < numSpeakers; j++) {
            if (connections[i][j] == 1) {
                auto const distance = speakers[i].coords.angleWith(speakers[j].coords);
                int k{};
                while (distance_table[k] < distance) {
                    ++k;
                }
                for (auto l = (table_size - 1); l > k; l--) {
                    distance_table[l] = distance_table[l - 1];
                    distance_table_i[l] = distance_table_i[l - 1];
                    distance_table_j[l] = distance_table_j[l - 1];
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
     * and proceeding to next shortest. */
    for (int i{}; i < table_size; ++i) {
        auto const fst_ls = distance_table_i[i];
        auto const sec_ls = distance_table_j[i];
        if (connections[fst_ls][sec_ls] == 1) {
            for (int j{}; j < numSpeakers; j++) {
                for (auto k = j + 1; k < numSpeakers; k++) {
                    if ((j != fst_ls) && (k != sec_ls) && (k != fst_ls) && (j != sec_ls)) {
                        if (lines_intersect(fst_ls, sec_ls, j, k, speakers)) {
                            connections[j][k] = 0;
                            connections[k][j] = 0;
                        }
                    }
                }
            }
        }
    }

    /* Remove triangles which had crossing sides with
     * smaller triangles or include loudspeakers. */
    ls_triplet_chain * trip_ptr = *speakerTriplets;
    ls_triplet_chain * prev = nullptr;
    while (trip_ptr != nullptr) {
        auto const & i = trip_ptr->tripletSpeakerNumber[0];
        auto const & j = trip_ptr->tripletSpeakerNumber[1];
        auto const & k = trip_ptr->tripletSpeakerNumber[2];
        if (connections[i][j] == 0 || connections[i][k] == 0 || connections[j][k] == 0
            || any_speaker_inside_triplet(i, j, k, speakers, numSpeakers) == 1) {
            if (prev != nullptr) {
                prev->nextTriplet = trip_ptr->nextTriplet;
                tmp_ptr = trip_ptr;
                trip_ptr = trip_ptr->nextTriplet;
                free(tmp_ptr);
            } else {
                *speakerTriplets = trip_ptr->nextTriplet;
                tmp_ptr = trip_ptr;
                trip_ptr = trip_ptr->nextTriplet;
                free(tmp_ptr);
            }
        } else {
            prev = trip_ptr;
            trip_ptr = trip_ptr->nextTriplet;
        }
    }
}

/* Calculates the inverse matrices for 3D.
 *
 * After this call, ls_triplets contains the speakers numbers
 * and the inverse matrix needed to compute channel gains.
 */
int calculate_3x3_matrixes(ls_triplet_chain * const ls_triplets,
                           LoudSpeaker speakers[MAX_SPEAKER_COUNT],
                           int /*numSpeakers*/)
{
    auto * tr_ptr = ls_triplets;

    if (tr_ptr == nullptr) {
        fprintf(stderr, "Not valid 3-D configuration.\n");
        return 0;
    }

    /* Calculations and data storage. */
    while (tr_ptr != nullptr) {
        CartesianVector * lp1 = &(speakers[tr_ptr->tripletSpeakerNumber[0]].coords);
        CartesianVector * lp2 = &(speakers[tr_ptr->tripletSpeakerNumber[1]].coords);
        CartesianVector * lp3 = &(speakers[tr_ptr->tripletSpeakerNumber[2]].coords);

        /* Matrix inversion. */
        float * invmx = tr_ptr->tripletInverseMatrix;
        float invdet
            = 1.0f
              / (lp1->x * ((lp2->y * lp3->z) - (lp2->z * lp3->y)) - lp1->y * ((lp2->x * lp3->z) - (lp2->z * lp3->x))
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
        tr_ptr = tr_ptr->nextTriplet;
    }
    return 1;
}

/* To be implemented without file reading...
 * Load explicit speakers triplets. Not tested yet...
 */
void load_ls_triplets(LoudSpeaker /*lss*/[MAX_SPEAKER_COUNT],
                      ls_triplet_chain ** ls_triplets,
                      int /*ls_amount*/,
                      const char * filename)
{
    int i;
    int j;
    int k;
    FILE * fp;
    char c[10000];
    char * toke;

    ls_triplet_chain * trip_ptr = *ls_triplets;
    ls_triplet_chain * prev = nullptr;
    while (trip_ptr != nullptr) {
        prev = trip_ptr;
        trip_ptr = trip_ptr->nextTriplet;
    }

    if ((fp = fopen(filename, "r")) == nullptr) {
        fprintf(stderr, "Could not open loudspeaker setup file.\n");
        exit(-1);
    }

    while (1) {
        if (fgets(c, 10000, fp) == nullptr)
            break;
        toke = strtok(c, " ");
        if (sscanf(toke, "%d", &i) > 0) {
            toke = strtok(nullptr, " ");
            sscanf(toke, "%d", &j);
            toke = strtok(nullptr, " ");
            sscanf(toke, "%d", &k);
        } else {
            break;
        }

        trip_ptr = new ls_triplet_chain;

        if (prev == nullptr)
            *ls_triplets = trip_ptr;
        else
            prev->nextTriplet = trip_ptr;

        trip_ptr->nextTriplet = nullptr;
        trip_ptr->tripletSpeakerNumber[0] = i - 1;
        trip_ptr->tripletSpeakerNumber[1] = j - 1;
        trip_ptr->tripletSpeakerNumber[2] = k - 1;
        prev = trip_ptr;
        trip_ptr = nullptr;
    }
}

VbapData * init_vbap_data(SpeakersSetup * const setup, int const * const * triplets) noexcept
{
    int i, j, ret;
    LoudSpeaker lss[MAX_SPEAKER_COUNT];
    ls_triplet_chain * ls_triplets = nullptr;
    ls_triplet_chain * ls_ptr;
    VbapData * data = (VbapData *)malloc(sizeof(VbapData));

    build_speakers_list(setup, lss);

    if (triplets == nullptr)
        choose_ls_triplets(lss, &ls_triplets, setup->count);
    else
        load_ls_triplets(lss, &ls_triplets, setup->count, "filename");

    ret = calculate_3x3_matrixes(ls_triplets, lss, setup->count);
    if (ret == 0) {
        free(data);
        return nullptr;
    }

    data->dimension = setup->dimension;
    data->numSpeakers = setup->count;
    for (i = 0; i < MAX_SPEAKER_COUNT; i++) {
        data->gains[i] = data->gainsSmoothing[i] = 0.0;
    }

    i = 0;
    ls_ptr = ls_triplets;
    while (ls_ptr != nullptr) {
        ls_ptr = ls_ptr->nextTriplet;
        i++;
    }
    data->numTriplets = i;
    data->speakerSets = new SpeakerSet[i];

    i = 0;
    ls_ptr = ls_triplets;
    while (ls_ptr != nullptr) {
        for (j = 0; j < data->dimension; j++) {
            data->speakerSets[i].speakerNos[j] = ls_ptr->tripletSpeakerNumber[j] + 1;
        }
        for (j = 0; j < (data->dimension * data->dimension); j++) {
            data->speakerSets[i].invMx[j] = ls_ptr->tripletInverseMatrix[j];
        }
        ls_ptr = ls_ptr->nextTriplet;
        i++;
    }

    free_ls_triplet_chain(ls_triplets);

    return data;
}

VbapData * init_vbap_from_speakers(LoudSpeaker lss[MAX_SPEAKER_COUNT],
                                   int count,
                                   int dim,
                                   int outputPatches[MAX_SPEAKER_COUNT],
                                   int maxOutputPatch,
                                   int const * const * triplets) noexcept
{
    int i, j, ret, offset = 0;
    ls_triplet_chain * ls_triplets = nullptr;
    ls_triplet_chain * ls_ptr;
    VbapData * data = (VbapData *)malloc(sizeof(VbapData));

    if (dim == 3) {
        if (triplets == nullptr)
            choose_ls_triplets(lss, &ls_triplets, count);
        else
            load_ls_triplets(lss, &ls_triplets, count, "filename");
        ret = calculate_3x3_matrixes(ls_triplets, lss, count);
        if (ret == 0) {
            free(data);
            return nullptr;
        }
        offset = 1;
    } else if (dim == 2) {
        choose_ls_tuplets(lss, &ls_triplets, count);
    }

    data->numOutputPatches = count;
    for (i = 0; i < count; i++) {
        data->outputPatches[i] = outputPatches[i];
    }

    data->dimension = dim;
    data->numSpeakers = maxOutputPatch;
    for (i = 0; i < MAX_SPEAKER_COUNT; i++) {
        data->gains[i] = data->gainsSmoothing[i] = 0.0;
    }

    i = 0;
    ls_ptr = ls_triplets;
    while (ls_ptr != nullptr) {
        ls_ptr = ls_ptr->nextTriplet;
        i++;
    }
    data->numTriplets = i;
    data->speakerSets = (SpeakerSet *)malloc(sizeof(SpeakerSet) * i);

    i = 0;
    ls_ptr = ls_triplets;
    while (ls_ptr != nullptr) {
        for (j = 0; j < data->dimension; j++) {
            // data->ls_sets[i].ls_nos[j] = ls_ptr->ls_nos[j] + offset;
            data->speakerSets[i].speakerNos[j] = outputPatches[ls_ptr->tripletSpeakerNumber[j] + offset - 1];
        }
        for (j = 0; j < (data->dimension * data->dimension); j++) {
            data->speakerSets[i].invMx[j] = ls_ptr->tripletInverseMatrix[j];
        }
        ls_ptr = ls_ptr->nextTriplet;
        i++;
    }

    free_ls_triplet_chain(ls_triplets);

    return data;
}

VbapData * copy_vbap_data(VbapData const * const data) noexcept
{
    int i;
    int j;
    auto * nw = new VbapData;
    nw->dimension = data->dimension;
    nw->numOutputPatches = data->numOutputPatches;
    for (i = 0; i < data->numOutputPatches; i++) {
        nw->outputPatches[i] = data->outputPatches[i];
    }
    nw->numSpeakers = data->numSpeakers;
    nw->numTriplets = data->numTriplets;
    for (i = 0; i < MAX_SPEAKER_COUNT; i++) {
        nw->gains[i] = data->gains[i];
        nw->gainsSmoothing[i] = data->gainsSmoothing[i];
    }
    nw->speakerSets = (SpeakerSet *)malloc(sizeof(SpeakerSet) * nw->numTriplets);
    for (i = 0; i < nw->numTriplets; i++) {
        for (j = 0; j < nw->dimension; j++) {
            nw->speakerSets[i].speakerNos[j] = data->speakerSets[i].speakerNos[j];
        }
        for (j = 0; j < nw->dimension * nw->dimension; j++) {
            nw->speakerSets[i].invMx[j] = data->speakerSets[i].invMx[j];
        }
    }
    nw->angularDirection.azimuth = data->angularDirection.azimuth;
    nw->angularDirection.elevation = data->angularDirection.elevation;
    nw->angularDirection.length = data->angularDirection.length;
    nw->cartesianDirection.x = data->cartesianDirection.x;
    nw->cartesianDirection.y = data->cartesianDirection.y;
    nw->cartesianDirection.z = data->cartesianDirection.z;
    nw->spreadingVector.x = data->spreadingVector.x;
    nw->spreadingVector.y = data->spreadingVector.y;
    nw->spreadingVector.z = data->spreadingVector.z;
    return nw;
}

void free_vbap_data(VbapData * const data) noexcept
{
    free(data->speakerSets);
    free(data);
}

void vbap(float const azimuth, float const elevation, float const spread, VbapData * const data) noexcept
{
    data->angularDirection.azimuth = azimuth;
    data->angularDirection.elevation = elevation;
    data->angularDirection.length = 1.0;
    data->cartesianDirection = data->angularDirection.toCartesian();
    data->spreadingVector.x = data->cartesianDirection.x;
    data->spreadingVector.y = data->cartesianDirection.y;
    data->spreadingVector.z = data->cartesianDirection.z;
    for (int i = 0; i < data->numSpeakers; i++) {
        data->gains[i] = 0.0;
    }
    compute_gains(data->numTriplets,
                  data->speakerSets,
                  data->gains,
                  data->numSpeakers,
                  data->cartesianDirection,
                  data->dimension);
    if (spread > 0) {
        spreadIt(azimuth, spread, data);
    }
}

void vbap2(float const azimuth,
           float const elevation,
           float const spAzimuth,
           float const spElevation,
           VbapData * const data) noexcept
{
    int i;
    data->angularDirection.azimuth = azimuth;
    data->angularDirection.elevation = elevation;
    data->angularDirection.length = 1.0;
    data->cartesianDirection = data->angularDirection.toCartesian();
    for (i = 0; i < data->numSpeakers; i++) {
        data->gains[i] = 0.0;
    }
    compute_gains(data->numTriplets,
                  data->speakerSets,
                  data->gains,
                  data->numSpeakers,
                  data->cartesianDirection,
                  data->dimension);
    if (data->dimension == 3) {
        if (spAzimuth > 0 || spElevation > 0) {
            spreadit_azi_ele(azimuth, elevation, spAzimuth, spElevation, data);
        }
    } else {
        if (spAzimuth > 0) {
            spreadit_azi(azimuth, spAzimuth, data);
        }
    }
}

void vbap_flip_y_z(float const azimuth, float const elevation, float const spread, VbapData * const data) noexcept
{
    data->angularDirection.azimuth = azimuth;
    data->angularDirection.elevation = elevation;
    data->angularDirection.length = 1.0;
    data->cartesianDirection = data->angularDirection.toCartesian();
    float tmp = data->cartesianDirection.z;
    data->cartesianDirection.z = data->cartesianDirection.y;
    data->cartesianDirection.y = tmp;
    data->spreadingVector.x = data->cartesianDirection.x;
    data->spreadingVector.y = data->cartesianDirection.y;
    data->spreadingVector.z = data->cartesianDirection.z;
    for (int i = 0; i < data->numSpeakers; i++) {
        data->gains[i] = 0.0;
    }
    compute_gains(data->numTriplets,
                  data->speakerSets,
                  data->gains,
                  data->numSpeakers,
                  data->cartesianDirection,
                  data->dimension);
    if (spread > 0) {
        spreadIt(azimuth, spread, data);
    }
}

void vbap2_flip_y_z(float const azimuth,
                    float const elevation,
                    float const spAzimuth,
                    float const spElevation,
                    VbapData * data) noexcept
{
    data->angularDirection.azimuth = azimuth;
    data->angularDirection.elevation = elevation;
    data->angularDirection.length = 1.0f;
    data->cartesianDirection = data->angularDirection.toCartesian();
    auto const tmp = data->cartesianDirection.z;
    data->cartesianDirection.z = data->cartesianDirection.y;
    data->cartesianDirection.y = tmp;
    for (int i{}; i < data->numSpeakers; ++i) {
        data->gains[i] = 0.0f;
    }
    compute_gains(data->numTriplets,
                  data->speakerSets,
                  data->gains,
                  data->numSpeakers,
                  data->cartesianDirection,
                  data->dimension);
    if (data->dimension == 3) {
        if (spAzimuth > 0 || spElevation > 0) {
            spreadit_azi_ele_flip_y_z(azimuth, elevation, spAzimuth, spElevation, data);
        }
    } else {
        if (spAzimuth > 0) {
            spreadit_azi_flip_y_z(azimuth, spAzimuth, data);
        }
    }
}

/* Selects a vector base of a virtual source.
 * Calculates gain factors in that base. */
void compute_gains(int const speaker_set_am,
                   SpeakerSet * sets,
                   float * gains,
                   int const numSpeakers,
                   CartesianVector const cart_dir,
                   int const dim) noexcept
{
    float vec[3];
    /* Direction of the virtual source in cartesian coordinates. */
    vec[0] = cart_dir.x;
    vec[1] = cart_dir.y;
    vec[2] = cart_dir.z;

    for (int i = 0; i < speaker_set_am; i++) {
        sets[i].setGains[0] = 0.0;
        sets[i].setGains[1] = 0.0;
        sets[i].setGains[2] = 0.0;
        sets[i].smallestWt = 1000.0;
        sets[i].neg_g_am = 0;
    }

    for (int i = 0; i < speaker_set_am; i++) {
        for (int j = 0; j < dim; j++) {
            for (int k = 0; k < dim; k++) {
                sets[i].setGains[j] += vec[k] * sets[i].invMx[((dim * j) + k)];
            }
            if (sets[i].smallestWt > sets[i].setGains[j])
                sets[i].smallestWt = sets[i].setGains[j];
            if (sets[i].setGains[j] < -0.05)
                sets[i].neg_g_am++;
        }
    }

    int j = 0;
    float tmp = sets[0].smallestWt;
    int tmp2 = sets[0].neg_g_am;
    for (int i = 1; i < speaker_set_am; i++) {
        if (sets[i].neg_g_am < tmp2) {
            tmp = sets[i].smallestWt;
            tmp2 = sets[i].neg_g_am;
            j = i;
        } else if (sets[i].neg_g_am == tmp2) {
            if (sets[i].smallestWt > tmp) {
                tmp = sets[i].smallestWt;
                tmp2 = sets[i].neg_g_am;
                j = i;
            }
        }
    }

    if (sets[j].setGains[0] <= 0.0 && sets[j].setGains[1] <= 0.0 && sets[j].setGains[2] <= 0.0) {
        sets[j].setGains[0] = 1.0;
        sets[j].setGains[1] = 1.0;
        sets[j].setGains[2] = 1.0;
    }

    memset(gains, 0, numSpeakers * sizeof(float));

    gains[sets[j].speakerNos[0] - 1] = sets[j].setGains[0];
    gains[sets[j].speakerNos[1] - 1] = sets[j].setGains[1];
    if (dim == 3)
        gains[sets[j].speakerNos[2] - 1] = sets[j].setGains[2];

    for (int i = 0; i < numSpeakers; i++) {
        if (gains[i] < 0.0)
            gains[i] = 0.0;
    }
}

int vbap_get_triplets(VbapData const * const data, int *** triplets)
{
    int const num = data->numTriplets;
    (*triplets) = (int **)malloc(num * sizeof(int *));
    for (int i = 0; i < num; i++) {
        (*triplets)[i] = (int *)malloc(3 * sizeof(int));
        (*triplets)[i][0] = data->speakerSets[i].speakerNos[0];
        (*triplets)[i][1] = data->speakerSets[i].speakerNos[1];
        (*triplets)[i][2] = data->speakerSets[i].speakerNos[2];
    }
    return num;
}
