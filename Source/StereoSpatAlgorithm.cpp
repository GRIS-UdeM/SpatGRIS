/*
 This file is part of SpatGRIS.

 Developers: Samuel Béland, Olivier Bélanger, Nicolas Masson

 SpatGRIS is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 SpatGRIS is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with SpatGRIS.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "StereoSpatAlgorithm.hpp"

//==============================================================================
void StereoSpatAlgorithm::updateSpatData(source_index_t const sourceIndex, SourceData const & sourceData) noexcept
{
    ASSERT_OSC_THREAD;
    jassert(sourceData.vector);

    using fast = juce::dsp::FastMathApproximations;

    static auto const TO_GAIN = [](radians_t const angle) { return fast::sin(angle.get()) / 2.0f + 0.5f; };

    auto & queue{ mData[sourceIndex].gainsQueue };
    auto * ticket{ queue.acquire() };

    auto & gains{ ticket->get() };

    gains[0] = TO_GAIN(sourceData.vector->azimuth - HALF_PI);
    gains[1] = TO_GAIN(sourceData.vector->azimuth + HALF_PI);

    queue.setMostRecent(ticket);
}

//==============================================================================
void StereoSpatAlgorithm::process(AudioConfig const & config,
                                  SourceAudioBuffer & sourcesBuffer,
                                  SpeakerAudioBuffer & speakersBuffer,
                                  SourcePeaks const & sourcePeaks,
                                  [[maybe_unused]] SpeakersAudioConfig const * altSpeakerConfig)
{
    ASSERT_AUDIO_THREAD;
    jassert(!altSpeakerConfig);

    auto const getBuffers = [&]() {
        auto it{ speakersBuffer.begin() };
        auto & leftBuffer{ *(*it++).value };
        auto & rightBuffer{ *(*it).value };
        return std::array<juce::AudioBuffer<float> &, 2>{ leftBuffer, rightBuffer };
    };

    auto const & gainInterpolation{ config.spatGainsInterpolation };
    auto const gainFactor{ std::pow(gainInterpolation, 0.1f) * 0.0099f + 0.99f };

    auto buffers{ getBuffers() };

    auto const numSamples{ sourcesBuffer.getNumSamples() };
    for (auto const & source : config.sourcesAudioConfig) {
        if (source.value.isMuted || source.value.directOut || sourcePeaks[source.key] < SMALL_GAIN) {
            continue;
        }

        auto & data{ mData[source.key] };

        data.gainsQueue.getMostRecent(data.currentGains);
        if (data.currentGains == nullptr) {
            continue;
        }
        auto const & gains{ data.currentGains->get() };

        auto & lastGains{ data.lastGains };
        auto const * inputSamples{ sourcesBuffer[source.key].getReadPointer(0) };

        static constexpr std::array<size_t, 2> SPEAKERS{ 0, 1 };

        for (auto const & speaker : SPEAKERS) {
            auto & currentGain{ lastGains[speaker] };
            auto const & targetGain{ gains[speaker] };
            auto * outputSamples{ buffers[speaker].getWritePointer(0) };
            if (gainInterpolation == 0.0f) {
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
    }

    // Apply gain compensation.
    auto const compensation{ std::pow(10.0f, (narrow<float>(config.sourcesAudioConfig.size()) - 1.0f) * -0.005f) };
    buffers[0].applyGain(0, numSamples, compensation);
    buffers[1].applyGain(0, numSamples, compensation);
}

//==============================================================================
juce::Array<Triplet> StereoSpatAlgorithm::getTriplets() const noexcept
{
    JUCE_ASSERT_MESSAGE_THREAD;
    jassertfalse;
    return juce::Array<Triplet>{};
}

//==============================================================================
StereoSpatAlgorithm::StereoSpatAlgorithm(SpeakerSetup const & speakerSetup, SourcesData const & sources)
{
    JUCE_ASSERT_MESSAGE_THREAD;
    fixDirectOutsIntoPlace(sources, speakerSetup);
}
