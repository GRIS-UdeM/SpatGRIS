#pragma once

#include "AudioStructs.hpp"
#include "PolarVector.h"
#include "SpatMode.hpp"

class AbstractSpatAlgorithm
{
public:
    AbstractSpatAlgorithm() = default;
    virtual ~AbstractSpatAlgorithm() = default;
    //==============================================================================
    AbstractSpatAlgorithm(AbstractSpatAlgorithm const &) = delete;
    AbstractSpatAlgorithm(AbstractSpatAlgorithm &&) = delete;
    AbstractSpatAlgorithm & operator=(AbstractSpatAlgorithm const &) = delete;
    AbstractSpatAlgorithm & operator=(AbstractSpatAlgorithm &&) = delete;
    //==============================================================================
    virtual void init(SpatGrisData::SpeakersData const & speakers) = 0;
    [[nodiscard]] virtual SpatMode getSpatMode() const noexcept = 0;
    [[nodiscard]] virtual SpeakersSpatGains computeSpeakerGains(PolarVector const & sourcePosition) const noexcept = 0;
    //==============================================================================
    [[nodiscard]] static std::unique_ptr<AbstractSpatAlgorithm> make(SpatMode spatMode);
};
