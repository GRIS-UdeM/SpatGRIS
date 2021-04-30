#pragma once

#include "AudioStructs.hpp"
#include "LogicStrucs.hpp"
#include "SpatMode.hpp"
#include "Triplet.hpp"

//==============================================================================
class AbstractSpatAlgorithm
{
public:
    // AbstractSpatAlgorithm() = default;
    virtual ~AbstractSpatAlgorithm() = default;
    //==============================================================================
    // AbstractSpatAlgorithm(AbstractSpatAlgorithm const &) = delete;
    // AbstractSpatAlgorithm(AbstractSpatAlgorithm &&) = delete;
    // AbstractSpatAlgorithm & operator=(AbstractSpatAlgorithm const &) = delete;
    // AbstractSpatAlgorithm & operator=(AbstractSpatAlgorithm &&) = delete;
    //==============================================================================
    virtual void computeSpeakerGains(SourceData const & source, SpeakersSpatGains & gains) const noexcept = 0;
    [[nodiscard]] virtual juce::Array<Triplet> getTriplets() const noexcept = 0;
    [[nodiscard]] virtual bool hasTriplets() const noexcept = 0;
    //==============================================================================
    [[nodiscard]] static std::unique_ptr<AbstractSpatAlgorithm> make(SpatMode spatMode, SpeakersData const & speakers);
};
