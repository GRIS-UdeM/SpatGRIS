#include "LbapSpatAlgorithm.hpp"

//==============================================================================
void LbapSpatAlgorithm::init(SpeakersData const & speakers)
{
    mData = lbap_field_setup(speakers);
}

//==============================================================================
SpeakersSpatGains LbapSpatAlgorithm::computeSpeakerGains(SourceData const & source) const noexcept
{
    return lbap_field_compute(source, mData);
}
