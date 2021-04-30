#pragma once

#include "vbap.hpp"

#include "AbstractSpatAlgorithm.hpp"

VbapType getVbapType(SpeakersData const & speakers);

class VbapSpatAlgorithm final : public AbstractSpatAlgorithm
{
    std::unique_ptr<VbapData> mData{};

public:
    //==============================================================================
    explicit VbapSpatAlgorithm(SpeakersData const & speakers);
    //==============================================================================
    void computeSpeakerGains(SourceData const & source, SpeakersSpatGains & gains) const noexcept override;
    [[nodiscard]] juce::Array<Triplet> getTriplets() const noexcept override;
    [[nodiscard]] bool hasTriplets() const noexcept override;
};
