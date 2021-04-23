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
    gains[output_patch_t{ 1 }] = fast::cos(source.vector->azimuth.get());
    gains[output_patch_t{ 2 }] = fast::sin(source.vector->azimuth.get());
}
