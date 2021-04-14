/*
 * Functions for 3D VBAP processing based on work by Ville Pulkki.
 * (c) Ville Pulkki - 2.2.1999 Helsinki University of Technology.
 * Updated by belangeo, 2017.
 */

#include "OwnedMap.hpp"

#include "vbap.hpp"
//
//#include "LogicStrucs.hpp"
//
//#include <cstdlib>
//
//#include "StrongTypes.hpp"
//#include "narrow.hpp"

using fast = juce::dsp::FastMathApproximations;

struct TripletData {
    SpeakerTriplet triplet;
    std::array<float, 9> inverseMatrix; /* Triplet inverse matrix */
};

SpeakersSpatGains
    compute_gains(int speaker_set_am, SpeakerSet * sets, int numSpeakers, CartesianVector cart_dir, int dim) noexcept;

/* Returns 1 if there is loudspeaker(s) inside given ls triplet. */
static bool any_speaker_inside_triplet(SpeakerTriplet const & triplet, SpeakersData const & speakers) noexcept
{
    std::array<float, 9> inverseMatrix;

    auto const * const lp1 = &(speakers[triplet.patch1].position);
    auto const * const lp2 = &(speakers[triplet.patch2].position);
    auto const * const lp3 = &(speakers[triplet.patch3].position);

    /* Matrix inversion. */
    auto const inverseDeterminant{ 1.0f
                                   / (lp1->x * ((lp2->y * lp3->z) - (lp2->z * lp3->y))
                                      - lp1->y * ((lp2->x * lp3->z) - (lp2->z * lp3->x))
                                      + lp1->z * ((lp2->x * lp3->y) - (lp2->y * lp3->x))) };

    inverseMatrix[0] = ((lp2->y * lp3->z) - (lp2->z * lp3->y)) * inverseDeterminant;
    inverseMatrix[3] = ((lp1->y * lp3->z) - (lp1->z * lp3->y)) * -inverseDeterminant;
    inverseMatrix[6] = ((lp1->y * lp2->z) - (lp1->z * lp2->y)) * inverseDeterminant;
    inverseMatrix[1] = ((lp2->x * lp3->z) - (lp2->z * lp3->x)) * -inverseDeterminant;
    inverseMatrix[4] = ((lp1->x * lp3->z) - (lp1->z * lp3->x)) * inverseDeterminant;
    inverseMatrix[7] = ((lp1->x * lp2->z) - (lp1->z * lp2->x)) * -inverseDeterminant;
    inverseMatrix[2] = ((lp2->x * lp3->y) - (lp2->y * lp3->x)) * inverseDeterminant;
    inverseMatrix[5] = ((lp1->x * lp3->y) - (lp1->y * lp3->x)) * -inverseDeterminant;
    inverseMatrix[8] = ((lp1->x * lp2->y) - (lp1->y * lp2->x)) * inverseDeterminant;

    for (auto const speaker : speakers) {
        if (!triplet.contains(speaker.key)) {
            auto this_inside{ true };
            auto const & speakerPosition{ speaker.value->position };
            for (size_t j{}; j < 3; ++j) {
                auto tmp = speakerPosition.x * inverseMatrix[0 + j * 3];
                tmp += speakerPosition.y * inverseMatrix[1 + j * 3];
                tmp += speakerPosition.z * inverseMatrix[2 + j * 3];
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

/* Checks if two lines intersect on 3D sphere see theory in paper
 * Pulkki, V. Lokki, T. "Creating Auditory Displays with Multiple
 * Loudspeakers Using VBAP: A Case Study with DIVA Project" in
 * International Conference on Auditory Displays -98.
 * E-mail Ville.Pulkki@hut.fi if you want to have that paper. */
static bool linesIntersect(CartesianVector const & a,
                           CartesianVector const & b,
                           CartesianVector const & c,
                           CartesianVector const & d) noexcept
{
    auto const v1{ a.crossProduct(b) };
    auto const v2{ c.crossProduct(d) };
    auto const v3{ v1.crossProduct(v2) };
    auto const negV3{ -v3 };

    auto const dist_ij = a.angleWith(b);
    auto const dist_kl = c.angleWith(d);
    auto const dist_iv3 = a.angleWith(v3);
    auto const dist_jv3 = v3.angleWith(b);
    auto const dist_inv3 = a.angleWith(negV3);
    auto const dist_jnv3 = negV3.angleWith(b);
    auto const dist_kv3 = c.angleWith(v3);
    auto const dist_lv3 = v3.angleWith(d);
    auto const dist_knv3 = c.angleWith(negV3);
    auto const dist_lnv3 = negV3.angleWith(d);

    /*One of loudspeakers is close to crossing point, don't do anything.*/
    if (std::abs(dist_iv3) <= 0.01 || std::abs(dist_jv3) <= 0.01 || std::abs(dist_kv3) <= 0.01
        || std::abs(dist_lv3) <= 0.01 || std::abs(dist_inv3) <= 0.01 || std::abs(dist_jnv3) <= 0.01
        || std::abs(dist_knv3) <= 0.01 || std::abs(dist_lnv3) <= 0.01) {
        return false;
    }

    if (((std::abs(dist_ij - (dist_iv3 + dist_jv3)) <= 0.01) && (std::abs(dist_kl - (dist_kv3 + dist_lv3)) <= 0.01))
        || ((std::abs(dist_ij - (dist_inv3 + dist_jnv3)) <= 0.01)
            && (std::abs(dist_kl - (dist_knv3 + dist_lnv3)) <= 0.01))) {
        return true;
    }
    return false;
}

static void spreadit_azi_ele(degrees_t /*azi*/, degrees_t /*ele*/, float sp_azi, float sp_ele, VbapData * data) noexcept
{
    int ind;
    static constexpr auto num = 4;
    radians_t newAzimuth;
    radians_t newElevation;
    float comp;
    int const cnt = data->numSpeakers;
    float sum = 0.0f;

    sp_azi = std::clamp(sp_azi, 0.0f, 1.0f);
    sp_ele = std::clamp(sp_ele, 0.0f, 1.0f);

    // If both sp_azi and sp_ele are active, we want to put a virtual source at
    // (azi, ele +/- eledev) and (azi +/- azidev, ele) locations.
    auto const kNum{ sp_azi > 0.0 && sp_ele > 0.0 ? 8 : 4 };

    for (int i{}; i < num; ++i) {
        auto const iFloat{ degrees_t{ narrow<degrees_t::type>(i) } };
        comp = std::pow(10.0f, ((iFloat + degrees_t{ 1.0f }) * -3.0f * 0.05f).get());
        auto const azimuthDev = (iFloat + degrees_t{ 1.0f }) * sp_azi * 45.0f;
        auto const elevationDev = (iFloat + degrees_t{ 1.0f }) * sp_ele * 22.5f;
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

            newAzimuth = newAzimuth.centered();
            newElevation = newElevation.centered();
            PolarVector const spreadAngle{ newAzimuth, newElevation, 1.0f };
            auto const spreadCartesian{ spreadAngle.toCartesian() };
            auto const tmp_gains{
                compute_gains(data->numTriplets, data->speakerSets, data->numSpeakers, spreadCartesian, data->dimension)
            };
            // TODO : uncomment
            // for (int j{}; j < cnt; ++j) {
            //    data->gains[j] += tmp_gains[j] * comp;
            //}
        }
    }

    if (sp_azi > 0.8f && sp_ele > 0.8f) {
        comp = (sp_azi - 0.8f) / 0.2f * (sp_ele - 0.8f) / 0.2f * 10.0f;
        for (int i{}; i < data->numOutputPatches; ++i) {
            data->gains[data->outputPatches[i].get() - 1] += comp;
        }
    }

    for (int i{}; i < data->numOutputPatches; ++i) {
        ind = data->outputPatches[i].get() - 1;
        sum += (data->gains[ind] * data->gains[ind]);
    }
    sum = std::sqrt(sum);
    for (int i{}; i < data->numOutputPatches; ++i) {
        ind = data->outputPatches[i].get() - 1;
        data->gains[ind] /= sum;
    }
}

static void spreadit_azi_ele_flip_y_z(degrees_t /*azi*/,
                                      degrees_t /*ele*/,
                                      float sp_azi,
                                      float sp_ele,
                                      VbapData * const data) noexcept
{
    radians_t newAzimuth{};
    radians_t newElevation{};
    float comp;
    auto const cnt = data->numSpeakers;
    // std::array<float, MAX_OUTPUTS> tmp_gains{};
    float sum = 0.0;

    sp_azi = std::clamp(sp_azi, 0.0f, 1.0f);
    sp_ele = std::clamp(sp_ele, 0.0f, 1.0f);

    // If both sp_azi and sp_ele are active, we want to put a virtual source at
    // (azi, ele +/- eledev) and (azi +/- azidev, ele) locations.
    auto const kNum{ sp_azi > 0.0f && sp_ele > 0.0f ? 8 : 4 };

    static constexpr auto NUM = 4;
    for (int i{}; i < NUM; ++i) {
        auto const iFloat{ static_cast<float>(i) };
        comp = std::pow(10.0f, (iFloat + 1.0f) * -3.0f * 0.05f);
        degrees_t const azimuthDev{ (iFloat + 1.0f) * sp_azi * 45.0f };
        degrees_t const elevationDev{ (iFloat + 1.0f) * sp_ele * 22.5f };
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
            newAzimuth = newAzimuth.centered();
            newElevation = newElevation.centered();
            PolarVector const spreadAngle{ newAzimuth, newElevation, 1.0f };
            CartesianVector spreadCartesian = spreadAngle.toCartesian();
            auto const tmp = spreadCartesian.z;
            spreadCartesian.z = spreadCartesian.y;
            spreadCartesian.y = tmp;
            auto const tmp_gains = compute_gains(data->numTriplets,
                                                 data->speakerSets,
                                                 data->numSpeakers,
                                                 spreadCartesian,
                                                 data->dimension);
            // TODO : uncomment
            /*for (int j{}; j < cnt; ++j) {
                data->gains[j] += tmp_gains[j] * comp;
            }*/
        }
    }

    if (sp_azi > 0.8f && sp_ele > 0.8f) {
        comp = (sp_azi - 0.8f) / 0.2f * (sp_ele - 0.8f) / 0.2f * 10.0f;
        for (int i{}; i < data->numOutputPatches; ++i) {
            data->gains[data->outputPatches[i].get() - 1] += comp;
        }
    }

    for (int i{}; i < data->numOutputPatches; ++i) {
        auto const index = data->outputPatches[i].get() - 1;
        sum += (data->gains[index] * data->gains[index]);
    }
    sum = std::sqrt(sum);
    for (int i{}; i < data->numOutputPatches; ++i) {
        auto const index = data->outputPatches[i].get() - 1;
        data->gains[index] /= sum;
    }
}

static void spreadit_azi(degrees_t /*azi*/, float azimuthSpread, VbapData * data)
{
    int i;
    static constexpr auto num = 4;
    radians_t newazi{};
    PolarVector spreadang;
    int cnt = data->numSpeakers;
    std::array<float, MAX_OUTPUTS> tmp_gains{};
    float sum = 0.0;

    if (azimuthSpread < 0.0) {
        azimuthSpread = 0.0;
    } else if (azimuthSpread > 1.0) {
        azimuthSpread = 1.0;
    }

    for (i = 0; i < num; i++) {
        float comp = std::pow(10.0f, (i + 1) * -3.0f * 0.05f);
        degrees_t const azidev{ (i + 1) * azimuthSpread * 45.0f };
        for (int k = 0; k < 2; k++) {
            if (k == 0) {
                newazi = data->angularDirection.azimuth + azidev;
            } else if (k == 1) {
                newazi = data->angularDirection.azimuth - azidev;
            }
            newazi = newazi.centered();
            spreadang.azimuth = newazi;
            spreadang.elevation = degrees_t{};
            spreadang.length = 1.0;
            CartesianVector spreadcart = spreadang.toCartesian();
            // TODO : uncomment
            // compute_gains(data->numTriplets,
            //              data->speakerSets,
            //              tmp_gains.data(),
            //              data->numSpeakers,
            //              spreadcart,
            //              data->dimension);
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

static void spreadit_azi_flip_y_z(degrees_t /*azi*/, float sp_azimuth, VbapData * data)
{
    sp_azimuth = std::clamp(sp_azimuth, 0.0f, 1.0f);

    static constexpr auto NUM = 4;
    std::array<float, MAX_OUTPUTS> tmp_gains{};
    int const cnt = data->numSpeakers;
    for (int i = 0; i < NUM; i++) {
        auto const comp = std::pow(10.0f, (i + 1) * -3.0f * 0.05f);
        degrees_t const azimuthDev{ (i + 1) * sp_azimuth * 45.0f };
        for (int k = 0; k < 2; k++) {
            auto newAzimuth
                = k == 0 ? data->angularDirection.azimuth + azimuthDev : data->angularDirection.azimuth - azimuthDev;
            newAzimuth = newAzimuth.centered();
            PolarVector const spreadAngle{ newAzimuth, degrees_t{}, 1.0f };
            auto spreadCartesian = spreadAngle.toCartesian();
            auto tmp = spreadCartesian.z;
            spreadCartesian.z = spreadCartesian.y;
            spreadCartesian.y = tmp;
            // TODO : uncomment
            /*compute_gains(data->numTriplets,
                          data->speakerSets,
                          tmp_gains.data(),
                          data->numSpeakers,
                          spreadCartesian,
                          data->dimension);*/
            for (int j = 0; j < cnt; j++) {
                data->gains[j] += (tmp_gains[j] * comp);
            }
        }
    }

    float sum = 0.0f;
    for (int i = 0; i < cnt; i++) {
        sum += (data->gains[i] * data->gains[i]);
    }
    sum = std::sqrt(sum);
    for (int i = 0; i < cnt; i++) {
        data->gains[i] /= sum;
    }
}

/* Calculate volume of the parallelepiped defined by the loudspeaker
 * direction vectors and divide it with total length of the triangle sides.
 * This is used when removing too narrow triangles. */
static float vol_p_side_lgth(SpeakerData const & i, SpeakerData const & j, SpeakerData const & k) noexcept
{
    auto const length
        = i.position.angleWith(j.position) + i.position.angleWith(k.position) + j.position.angleWith(k.position);

    if (length <= 0.00001f) {
        return 0.0f;
    }

    auto const xProduct{ i.position.crossProduct(j.position) };
    auto const volper = std::abs(xProduct.dotProduct(k.position));

    return volper / length;
}

/* Selects the loudspeaker triplets, and calculates the inversion
 * matrices for each selected triplet. A line (connection) is drawn
 * between each loudspeaker. The lines denote the sides of the
 * triangles. The triangles should not be intersecting. All crossing
 * connections are searched and the longer connection is erased.
 * This yields non-intersecting triangles, which can be used in panning.
 */
std::vector<TripletData> computeTriplets(SpeakersData const & speakers) noexcept
{
    static constexpr size_t MAGIC = MAX_OUTPUTS * (MAX_OUTPUTS - 1) / 2;
    jassert(speakers.size() >= 3);

    /*
     * Ok, so the next part of the algorithm has to check vol_p_side_lgth() for EVERY possible speaker triplet. This
     * takes an absurd amount of time for setups bigger than 100 speakers.
     *
     * Luckily, at least two speakers have to be at a similar elevation for the triplet to be valid. Instead of looking
     * for this inside vol_p_side_lgth(), we can take advantage of this fact to reduce the search space :
     *
     * 1- Sort all the speaker indexes according to their elevation.
     *
     * 2- Select every pair of speaker that is within the maximum elevation range and for every other speaker, check for
     * vol_p_side_lgth().
     */

    // We first build an array with all the indexes
    std::vector<output_patch_t> speakerIndexesSortedByElevation{};
    speakerIndexesSortedByElevation.resize(speakers.size());
    std::iota(std::begin(speakerIndexesSortedByElevation), std::end(speakerIndexesSortedByElevation), 0);

    // ...then we sort it according to the elevation values
    auto const sortIndexesBySpeakerElevation
        = [&speakers](output_patch_t const & indexA, output_patch_t const & indexB) -> bool {
        return speakers[indexA].vector.elevation < speakers[indexB].vector.elevation;
    };
    std::sort(std::begin(speakerIndexesSortedByElevation),
              std::end(speakerIndexesSortedByElevation),
              sortIndexesBySpeakerElevation);

    // ...then we test for valid triplets ONLY when the elevation difference is within a specified range for two
    // speakers
    StaticMap<output_patch_t, StaticMap<output_patch_t, bool, MAX_OUTPUTS>, MAX_OUTPUTS> connections{};
    std::vector<TripletData> triplets{};
    for (size_t i{}; i < speakerIndexesSortedByElevation.size(); ++i) {
        auto const speaker1Index{ speakerIndexesSortedByElevation[i] };
        auto const & speaker1{ speakers[speaker1Index] };
        for (auto j{ i + 1 }; j < speakerIndexesSortedByElevation.size(); ++j) {
            auto const speaker2Index{ speakerIndexesSortedByElevation[j] };
            auto const & speaker2{ speakers[speaker2Index] };
            static constexpr degrees_t MAX_ELEVATION_DIFF{ 10.0f };
            if (speaker2.vector.elevation - speaker1.vector.elevation > MAX_ELEVATION_DIFF) {
                // The elevation difference is only going to get greater : we can move the 1st speaker and reset the
                // other loops
                break;
            }
            for (size_t k{}; k < speakerIndexesSortedByElevation.size(); ++k) {
                if (k >= i && k <= j) {
                    // If k is between i and j, it means that i and k are within the elevation threshold (as well as k
                    // and j), so they are going to get checked anyway. We also need not to include i or j twice!
                    continue;
                }
                auto const speaker3Index{ speakerIndexesSortedByElevation[k] };
                auto const & speaker3{ speakers[speaker3Index] };
                auto const isValidCandidate{ vol_p_side_lgth(speaker1, speaker2, speaker3) > MIN_VOL_P_SIDE_LENGTH };
                if (isValidCandidate) {
                    connections[speaker1Index][speaker2Index] = true;
                    connections[speaker2Index][speaker1Index] = true;
                    connections[speaker1Index][speaker3Index] = true;
                    connections[speaker3Index][speaker1Index] = true;
                    connections[speaker2Index][speaker3Index] = true;
                    connections[speaker3Index][speaker2Index] = true;
                    triplets.emplace_back(speaker1Index, speaker2Index, speaker3Index);
                }
            }
        }
    }

    /* Calculate distances between all lss and sorting them. */
    auto table_size = (speakers.size() - 1) * speakers.size() / 2;
    std::vector<float> distance_table{};
    distance_table.resize(table_size);
    std::fill(std::begin(distance_table), std::end(distance_table), 100000.0f);

    std::vector<int> distance_table_i{};
    distance_table_i.resize(table_size);

    std::vector<int> distance_table_j{};
    distance_table_j.resize(table_size);

    for (auto i_speaker_it{ speakers.cbegin() }; i_speaker_it != speakers.cend(); ++i_speaker_it) {
        auto const & i_speaker{ *(*i_speaker_it).value };
        auto const i_outputPatch{ (*i_speaker_it).key };
        for (auto j_speaker_it{ ++SpeakersData::iterator_type{ i_speaker_it } }; j_speaker_it != speakers.cend();
             ++j_speaker_it) {
            auto const & j_speaker{ *(*j_speaker_it).value };
            auto const j_outputPatch{ (*j_speaker_it).key };
            if (connections[i_outputPatch][j_outputPatch]) {
                auto const distance = i_speaker.position.angleWith(j_speaker.position);
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
                distance_table_i[k] = i_outputPatch.get() - 1;
                distance_table_j[k] = j_outputPatch.get() - 1;
            } else {
                table_size--;
            }
        }
    }

    /* Disconnecting connections which are crossing shorter ones,
     * starting from shortest one and removing all that cross it,
     * and proceeding to next shortest. */
    for (int i{}; i < table_size; ++i) {
        output_patch_t const i_outputPatch{ distance_table_i[i] + 1 };
        output_patch_t const j_outputPatch{ distance_table_j[i] + 1 };
        if (connections[i_outputPatch][j_outputPatch]) {
            for (auto j_speaker_it{ speakers.cbegin() }; j_speaker_it != speakers.cend(); ++j_speaker_it) {
                auto const j_outputPatch{ (*j_speaker_it).key };
                for (auto k_speaker_it{ ++speakers.cbegin() }; k_speaker_it != speakers.cend(); ++k_speaker_it) {
                    auto const k_outputPatch{ (*k_speaker_it).key };
                    if ((j_outputPatch != i_outputPatch) && (k_outputPatch != j_outputPatch)
                        && (k_outputPatch != i_outputPatch) && (j_outputPatch != j_outputPatch)) {
                        auto const & a{ speakers[i_outputPatch].position };
                        auto const & b{ speakers[j_outputPatch].position };
                        auto const & c{ speakers[j_outputPatch].position };
                        auto const & d{ speakers[k_outputPatch].position };
                        if (linesIntersect(a, b, c, d)) {
                            connections[j_outputPatch][k_outputPatch] = false;
                            connections[k_outputPatch][j_outputPatch] = false;
                        }
                    }
                }
            }
        }
    }

    /* Remove triangles which had crossing sides with
     * smaller triangles or include loudspeakers. */
    auto const predicate = [&](TripletData const & triplet) -> bool {
        auto const & i = triplet.triplet.patch1;
        auto const & j = triplet.triplet.patch2;
        auto const & k = triplet.triplet.patch3;

        auto const anySpeakerInsideTriplet{ any_speaker_inside_triplet(triplet.triplet, speakers) };
        auto const hasAllConnections{ connections[i][j] && connections[i][k] && connections[j][k] };

        auto const result{ hasAllConnections && !anySpeakerInsideTriplet };

        return result;
    };
    auto const newTripletEnd{ std::partition(std::begin(triplets), std::end(triplets), predicate) };

    triplets.erase(newTripletEnd, std::end(triplets));
    triplets.shrink_to_fit();
    return triplets;
}

/* Calculates the inverse matrices for 3D.
 *
 * After this call, ls_triplets contains the speakers numbers
 * and the inverse matrix needed to compute channel gains.
 */
static std::array<float, 9> computeInverseMatrix(TripletData const & triplet, SpeakersData const & speakers)
{
    auto const & lp1{ speakers[triplet.triplet.patch1].position };
    auto const & lp2{ speakers[triplet.triplet.patch2].position };
    auto const & lp3{ speakers[triplet.triplet.patch3].position };

    /* Matrix inversion. */
    auto const inverseDet = 1.0f
                            / (lp1.x * ((lp2.y * lp3.z) - (lp2.z * lp3.y)) - lp1.y * ((lp2.x * lp3.z) - (lp2.z * lp3.x))
                               + lp1.z * ((lp2.x * lp3.y) - (lp2.y * lp3.x)));

    std::array<float, 9> inverseMatrix;
    inverseMatrix[0] = ((lp2.y * lp3.z) - (lp2.z * lp3.y)) * inverseDet;
    inverseMatrix[3] = ((lp1.y * lp3.z) - (lp1.z * lp3.y)) * -inverseDet;
    inverseMatrix[6] = ((lp1.y * lp2.z) - (lp1.z * lp2.y)) * inverseDet;
    inverseMatrix[1] = ((lp2.x * lp3.z) - (lp2.z * lp3.x)) * -inverseDet;
    inverseMatrix[4] = ((lp1.x * lp3.z) - (lp1.z * lp3.x)) * inverseDet;
    inverseMatrix[7] = ((lp1.x * lp2.z) - (lp1.z * lp2.x)) * -inverseDet;
    inverseMatrix[2] = ((lp2.x * lp3.y) - (lp2.y * lp3.x)) * inverseDet;
    inverseMatrix[5] = ((lp1.x * lp3.y) - (lp1.y * lp3.x)) * -inverseDet;
    inverseMatrix[8] = ((lp1.x * lp2.y) - (lp1.y * lp2.x)) * inverseDet;
    return inverseMatrix;
}

std::unique_ptr<VbapData> init_vbap_from_speakers(SpeakersData const & speakers)
{
    static constexpr auto offset = 1;

    auto data{ std::make_unique<VbapData>() };
    auto triplets{ computeTriplets(speakers) };
    for (auto & triplet : triplets) {
        triplet.inverseMatrix = computeInverseMatrix(triplet, speakers);
    }
    // TODO : uncomment

    // data->numOutputPatches = numSpeakers;
    // for (int i = 0; i < numSpeakers; i++) {
    //    data->outputPatches[i] = outputPatches[i];
    //}

    // data->dimension = dimensions;
    // data->numSpeakers = maxOutputPatch.get();

    // data->numTriplets = narrow<int>(triplets.size());
    // data->speakerSets = static_cast<SpeakerSet *>(malloc(sizeof(SpeakerSet) * triplets.size()));

    // for (size_t i{}; i < triplets.size(); ++i) {
    //    auto const & triplet{ triplets[i] };
    //    for (int j = 0; j < data->dimension; j++) {
    //        data->speakerSets[i].speakerNos[j] = outputPatches[triplet.tripletSpeakerNumber[j] + offset - 1].get();
    //    }
    //    for (int j = 0; j < (data->dimension * data->dimension); j++) {
    //        data->speakerSets[i].invMx[j] = triplet.tripletInverseMatrix[j];
    //    }
    //}

    return data;
}

SpeakersSpatGains vbap2(degrees_t const azimuth,
                        degrees_t const elevation,
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
    // TODO : uncomment
    // compute_gains(data->numTriplets,
    //              data->speakerSets,
    //              data->gains,
    //              data->numSpeakers,
    //              data->cartesianDirection,
    //              data->dimension);
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

SpeakersSpatGains vbap2_flip_y_z(degrees_t const azimuth,
                                 degrees_t const elevation,
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
    // TODO : uncomment
    // compute_gains(data->numTriplets,
    //              data->speakerSets,
    //              data->gains,
    //              data->numSpeakers,
    //              data->cartesianDirection,
    //              data->dimension);
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
SpeakersSpatGains compute_gains(int const speaker_set_am,
                                SpeakerSet * sets,
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

    SpeakersSpatGains gains{};
    gains[sets[j].speakerNos[0]] = juce::jmax(sets[j].setGains[0], 0.0f);
    gains[sets[j].speakerNos[1]] = juce::jmax(sets[j].setGains[1], 0.0f);
    jassert(dim == 3);
    /*if (dim == 3)
    {*/
    gains[sets[j].speakerNos[2]] = juce::jmax(sets[j].setGains[2], 0.0f);
    //}
}

juce::Array<std::array<output_patch_t, 3>> vbap_get_triplets(VbapData const * const data)
{
    juce::Array<std::array<output_patch_t, 3>> result{};
    result.resize(data->numSpeakers);
    std::transform(data->speakerSets,
                   data->speakerSets + data->numSpeakers,
                   result.data(),
                   [](SpeakerSet const & speakerSet) { return speakerSet.speakerNos; });
    return result;
}
