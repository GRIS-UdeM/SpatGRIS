#include "StereoSpatAlgorithm.hpp"

//==============================================================================
void StereoSpatAlgorithm::init([[maybe_unused]] SpeakersData const & speakers)
{
    jassert(speakers.size() == 2);
}

//==============================================================================
juce::Array<Triplet> StereoSpatAlgorithm::getTriplets() const noexcept
{
    jassertfalse;
    return juce::Array<Triplet>{};
}

//==============================================================================
void StereoSpatAlgorithm::computeSpeakerGains(SourceData const & source, SpeakersSpatGains & gains) const noexcept
{
    jassert(source.vector);
    using fast = juce::dsp::FastMathApproximations;

    static auto const TO_GAIN = [](radians_t const angle) { return fast::sin(angle.get()) / 2.0f + 0.5f; };

    gains[output_patch_t{ 1 }] = TO_GAIN(source.vector->azimuth - HALF_PI);
    gains[output_patch_t{ 2 }] = TO_GAIN(source.vector->azimuth + HALF_PI);
}
