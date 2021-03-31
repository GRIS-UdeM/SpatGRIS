#include "SpatAlgorithm.hpp"

#include "VbapAlgorithm.h"

//==============================================================================
std::unique_ptr<AbstractSpatAlgorithm> AbstractSpatAlgorithm::make(SpatMode spatMode)
{
    switch (spatMode) {
    case SpatMode::vbap:
        return std::make_unique<VbapAlgorithm>();
    }
    jassertfalse; // not implemented
    return {};
}
