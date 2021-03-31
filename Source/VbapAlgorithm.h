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
    void updateSourcePosition(source_index_t sourceIndex, PolarVector const & position) override {}

protected:
    void processChannel(float const * input,
                        float * output,
                        source_index_t sourceIndex,
                        output_patch_t outputPatch,
                        AudioConfig const & config) override
    {
        auto & currentGain{ mLastSpatGains[sourceIndex][outputPatch] };
        auto const targetGain{ (*mSpatGains[sourceIndex].get())[outputPatch] };
        if (config.spatGainsInterpolation == 0.0f) {
            // linear interpolation over buffer size
            auto const gainSlope = (targetGain - currentGain) / narrow<float>(numSamples);
            if (targetGain < SMALL_GAIN && currentGain < SMALL_GAIN) {
                // this is not going to produce any more sounds!
                continue;
            }
            for (int sampleIndex{}; sampleIndex < numSamples; ++sampleIndex) {
                currentGain += gainSlope;
                outputSamples[sampleIndex] += inputSamples[sampleIndex] * currentGain;
            }
        } else {
            // log interpolation with 1st order filter
            for (int sampleIndex{}; sampleIndex < numSamples; ++sampleIndex) {
                currentGain = targetGain + (currentGain - targetGain) * gainFactor;
                if (currentGain < SMALL_GAIN && targetGain < SMALL_GAIN) {
                    // If the gain is near zero and the target gain is also near zero, this means that
                    // currentGain will no ever increase over this buffer
                    break;
                }
                outputSamples[sampleIndex] += inputSamples[sampleIndex] * currentGain;
            }
        }
    }
};
