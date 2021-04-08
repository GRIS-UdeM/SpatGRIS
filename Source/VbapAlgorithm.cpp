#include "VbapAlgorithm.h"

//==============================================================================
void VbapAlgorithm::init(SpatGrisData::SpeakersData const & speakers)
{
    mData = init_vbap_from_speakers(speakers);
}
