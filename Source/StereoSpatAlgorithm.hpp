#pragma once

#include "AbstractSpatAlgorithm.hpp"

//==============================================================================
class StereoSpatAlgorithm final : public AbstractSpatAlgorithm
{
public:
    void init([[maybe_unused]] SpeakersData const & speakers) override;
    [[nodiscard]] SpeakersSpatGains computeSpeakerGains(SourceData const & source) const noexcept override;
    [[nodiscard]] bool hasTriplets() const override { return false; }
    [[nodiscard]] juce::Array<Triplet> getTriplets() const override
    {
        jassertfalse;
        return juce::Array<Triplet>{};
    }
};