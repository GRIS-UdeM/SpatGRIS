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

#include "LbapSpatAlgorithm.hpp"

//==============================================================================
LbapSpatAlgorithm::LbapSpatAlgorithm(SpeakersData const & speakers) : mField(lbapInit(speakers))
{
    JUCE_ASSERT_MESSAGE_THREAD;
}

//==============================================================================
void LbapSpatAlgorithm::updateSpatData(source_index_t const sourceIndex, SourceData const & sourceData) noexcept
{
    jassert(!isProbablyAudioThread());

    auto & data{ mData[sourceIndex] };

    auto & exchanger{ data.dataQueue };
    auto * ticket{ exchanger.acquire() };
    auto & spatData{ ticket->get() };

    if (sourceData.vector) {
        lbap(sourceData, spatData.gains, mField);
        spatData.lbapSourceDistance = sourceData.vector->length;
    } else {
        spatData.gains = SpeakersSpatGains{};
    }

    exchanger.setMostRecent(ticket);
}

//==============================================================================
void LbapSpatAlgorithm::process(AudioConfig const & config,
                                SourceAudioBuffer & sourcesBuffer,
                                SpeakerAudioBuffer & speakersBuffer,
                                SourcePeaks const & sourcePeaks,
                                SpeakersAudioConfig const * altSpeakerConfig)
{
    ASSERT_AUDIO_THREAD;

    auto const & gainInterpolation{ config.spatGainsInterpolation };

    auto const gainFactor{ std::pow(gainInterpolation, 0.1f) * 0.0099f + 0.99f };
    auto const numSamples{ sourcesBuffer.getNumSamples() };

    auto const & speakersAudioConfig{ altSpeakerConfig ? *altSpeakerConfig : config.speakersAudioConfig };

    for (auto const & source : config.sourcesAudioConfig) {
        if (source.value.isMuted || source.value.directOut || sourcePeaks[source.key] < SMALL_GAIN) {
            continue;
        }

        auto & data{ mData[source.key] };

        data.dataQueue.getMostRecent(data.currentData);
        if (data.currentData == nullptr) {
            continue;
        }
        auto const & spatData{ data.currentData->get() };
        auto const & targetGains{ spatData.gains };
        auto & lastGains{ data.lastGains };

        // process attenuation
        auto * inputData{ sourcesBuffer[source.key].getWritePointer(0) };
        config.lbapAttenuationConfig.process(inputData, numSamples, spatData.lbapSourceDistance, data.attenuationState);

        // Process spatialization
        for (auto const & speaker : speakersAudioConfig) {
            auto * outputSamples{ speakersBuffer[speaker.key].getWritePointer(0) };
            auto const & targetGain{ targetGains[speaker.key] };
            auto & currentGain{ lastGains[speaker.key] };
            if (gainInterpolation == 0.0f) {
                // linear interpolation over buffer size
                auto const gainSlope = (targetGain - currentGain) / narrow<float>(numSamples);
                if (currentGain < SMALL_GAIN && targetGain < SMALL_GAIN) {
                    // This is not going to produce any more sounds!
                    continue;
                }
                for (int sampleIndex{}; sampleIndex < numSamples; ++sampleIndex) {
                    currentGain += gainSlope;
                    outputSamples[sampleIndex] += inputData[sampleIndex] * currentGain;
                }
            } else {
                // log interpolation with 1st order filter
                for (int sampleIndex{}; sampleIndex < numSamples; ++sampleIndex) {
                    currentGain = (currentGain - targetGain) * gainFactor + targetGain;
                    if (currentGain < SMALL_GAIN && targetGain < SMALL_GAIN) {
                        // If the gain is near zero and the target gain is also near zero, this means that
                        // currentGain will no ever increase over this buffer
                        break;
                    }
                    outputSamples[sampleIndex] += inputData[sampleIndex] * currentGain;
                }
            }
        }
    }
}

//==============================================================================
juce::Array<Triplet> LbapSpatAlgorithm::getTriplets() const noexcept
{
    JUCE_ASSERT_MESSAGE_THREAD;
    jassertfalse;
    return juce::Array<Triplet>{};
}
