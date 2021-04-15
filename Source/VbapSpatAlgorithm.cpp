#include "VbapSpatAlgorithm.hpp"

//==============================================================================
void VbapSpatAlgorithm::init(SpeakersData const & speakers)
{
    mData = init_vbap_from_speakers(speakers);
}

//==============================================================================
SpeakersSpatGains VbapSpatAlgorithm::computeSpeakerGains(SourceData const & source) const noexcept
{
    jassert(source.vector);
    if (mData->dimension == 3) {
        return vbap2_flip_y_z(source.vector->azimuth,
                              source.vector->elevation,
                              source.azimuthSpan,
                              source.zenithSpan,
                              mData.get());
    }
    jassert(mData->dimension == 2);
    return vbap2(source.vector->azimuth, source.vector->elevation, source.azimuthSpan, source.zenithSpan, mData.get());
}

//==============================================================================
juce::Array<Triplet> VbapSpatAlgorithm::getTriplets() const
{
    // TODO
    jassertfalse;
    return juce::Array<Triplet>{};
}
