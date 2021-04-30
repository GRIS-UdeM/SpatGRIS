#include "VbapSpatAlgorithm.hpp"

VbapType getVbapType(SpeakersData const & speakers)
{
    auto const firstSpeaker{ *speakers.begin() };
    auto const firstZenith{ firstSpeaker.value->vector.elevation };
    auto const minZenith{ firstZenith - degrees_t{ 4.9f } };
    auto const maxZenith{ firstZenith + degrees_t{ 4.9f } };

    auto const areSpeakersOnSamePlane{ std::all_of(speakers.cbegin(),
                                                   speakers.cend(),
                                                   [&](SpeakersData::ConstNode const node) {
                                                       auto const zenith{ node.value->vector.elevation };
                                                       return zenith < maxZenith && zenith > minZenith;
                                                   }) };
    return areSpeakersOnSamePlane ? VbapType::twoD : VbapType::threeD;
}

//==============================================================================
VbapSpatAlgorithm::VbapSpatAlgorithm(SpeakersData const & speakers)
{
    std::array<LoudSpeaker, MAX_OUTPUTS> loudSpeakers{};
    std::array<output_patch_t, MAX_OUTPUTS> outputPatches{};
    size_t index{};
    output_patch_t maxOutputPatch{};
    for (auto const & speaker : speakers) {
        if (speaker.value->isDirectOutOnly) {
            continue;
        }

        maxOutputPatch = std::max(maxOutputPatch, speaker.key);

        loudSpeakers[index].coords = speaker.value->position;
        loudSpeakers[index].angles = speaker.value->vector;
        outputPatches[index] = speaker.key;
        ++index;
    }
    auto const dimensions{ getVbapType(speakers) == VbapType::twoD ? 2 : 3 };
    auto const numSpeakers{ narrow<int>(index) };

    mData.reset(init_vbap_from_speakers(loudSpeakers, numSpeakers, dimensions, outputPatches, maxOutputPatch));
}

//==============================================================================
void VbapSpatAlgorithm::computeSpeakerGains(SourceData const & source, SpeakersSpatGains & gains) const noexcept
{
    jassert(source.vector);
    vbap2(source, gains, *mData);
}

//==============================================================================
juce::Array<Triplet> VbapSpatAlgorithm::getTriplets() const noexcept
{
    jassert(hasTriplets());
    return vbap_get_triplets(*mData);
}

//==============================================================================
bool VbapSpatAlgorithm::hasTriplets() const noexcept
{
    if (!mData) {
        return false;
    }
    return mData->dimension == 3;
}
