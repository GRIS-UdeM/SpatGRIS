#pragma once

#include "AbstractSpatAlgorithm.hpp"

//==============================================================================
class StereoSpatAlgorithm final : public AbstractSpatAlgorithm
{
public:
    void init([[maybe_unused]] SpeakersData const & speakers) override;
    void computeSpeakerGains(SourceData const & source, SpeakersSpatGains & gains) const noexcept override;
    [[nodiscard]] juce::Array<Triplet> getTriplets() const noexcept override;
    [[nodiscard]] bool hasTriplets() const noexcept override { return false; }
};