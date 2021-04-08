#pragma once

#include "vbap.hpp"

#include "AbstractSpatAlgorithm.hpp"

class VbapAlgorithm : public AbstractSpatAlgorithm
{
    std::unique_ptr<VbapData> mData{};

public:
    void init(SpatGrisData::SpeakersData const & speakers) override;

    [[nodiscard]] SpeakersSpatGains computeSpeakerGains(PolarVector const & sourcePosition) const noexcept override;
    [[nodiscard]] SpatMode getSpatMode() const noexcept override { return SpatMode::vbap; }
};
