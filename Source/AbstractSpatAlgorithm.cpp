#include "AbstractSpatAlgorithm.hpp"

#include "LbapSpatAlgorithm.hpp"
#include "StereoSpatAlgorithm.hpp"
#include "VbapSpatAlgorithm.hpp"

//==============================================================================
std::unique_ptr<AbstractSpatAlgorithm> AbstractSpatAlgorithm::make(SpatMode const spatMode)
{
    switch (spatMode) {
    case SpatMode::vbap:
        return std::make_unique<VbapSpatAlgorithm>();
    case SpatMode::lbap:
    case SpatMode::hrtfVbap:
        return std::make_unique<LbapSpatAlgorithm>();
    case SpatMode::stereo:
        return std::make_unique<StereoSpatAlgorithm>();
    }
    jassertfalse; // not implemented
    return {};
}
