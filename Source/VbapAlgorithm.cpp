#include "VbapAlgorithm.h"

#include "narrow.hpp"

//==============================================================================
void VbapAlgorithm::init(SpatGrisData::SpeakersData const & speakers)
{
    mData = init_vbap_from_speakers(speakers);
}
