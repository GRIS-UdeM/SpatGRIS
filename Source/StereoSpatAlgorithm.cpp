#include "StereoSpatAlgorithm.hpp"

//==============================================================================
void StereoSpatAlgorithm::init(SpeakersData const & speakers)
{
    jassert(speakers.size() == 2);
}

//==============================================================================
SpeakersSpatGains StereoSpatAlgorithm::computeSpeakerGains(SourceData const & source) const noexcept
{
    jassert(source.vector);
    // Process
    SpeakersSpatGains result{};
    using fast = juce::dsp::FastMathApproximations;
    result[output_patch_t{ 1 }] = fast::cos(source.vector->azimuth.get());
    result[output_patch_t{ 2 }] = fast::sin(source.vector->azimuth.get());

    return result;
}
