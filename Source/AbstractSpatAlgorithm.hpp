#pragma once

#include "AudioStructs.hpp"
#include "LogicStrucs.hpp"
#include "SpatMode.hpp"
#include "SpeakerModel.h"

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
    virtual void init(SpeakersData const & speakers) = 0;
    [[nodiscard]] virtual SpeakersSpatGains computeSpeakerGains(SourceData const & source) const noexcept = 0;
    [[nodiscard]] virtual bool hasTriplets() const = 0;
    [[nodiscard]] juce::Array<Triplet> getTriplets() const = 0;
    //==============================================================================
    [[nodiscard]] static std::unique_ptr<AbstractSpatAlgorithm> make(SpatMode spatMode);
};
