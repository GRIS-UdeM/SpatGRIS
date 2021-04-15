#pragma once

#include "AbstractSpatAlgorithm.hpp"

#include "lbap.hpp"

//==============================================================================
class LbapSpatAlgorithm final : public AbstractSpatAlgorithm
{
    lbap_field mData{};

public:
    void init(SpeakersData const & speakers) override;
    [[nodiscard]] SpeakersSpatGains computeSpeakerGains(SourceData const & source) const noexcept override;
    [[nodiscard]] bool hasTriplets() const override { return false; }
    [[nodiscard]] juce::Array<Triplet> getTriplets() const override
    {
        jassertfalse;
        return juce::Array<Triplet>{};
    }
};