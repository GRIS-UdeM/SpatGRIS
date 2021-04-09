#pragma once

#include "vbap.hpp"

#include "AbstractSpatAlgorithm.hpp"

class VbapSpatAlgorithm final : public AbstractSpatAlgorithm
{
    std::unique_ptr<VbapData> mData{};

public:
    void init(SpeakersData const & speakers) override;

    [[nodiscard]] SpeakersSpatGains computeSpeakerGains(SourceData const & source) const noexcept override;
};
