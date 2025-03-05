/*
 This file is part of SpatGRIS.

 Developers: Gaël Lane Lépine, Samuel Béland, Olivier Bélanger, Nicolas Masson

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

#include "sg_MbapSpatAlgorithm.hpp"
#include "sg_DummySpatAlgorithm.hpp"
#include "Containers/sg_StaticMap.hpp"
#include "Containers/sg_StrongArray.hpp"
#include "Containers/sg_TaggedAudioBuffer.hpp"

namespace gris
{
//==============================================================================
MbapSpatAlgorithm::MbapSpatAlgorithm(SpeakerSetup const & speakerSetup) : mField(mbapInit(speakerSetup.speakers))
{
    JUCE_ASSERT_MESSAGE_THREAD;

    auto constexpr DIFFUSION_IN_MIN{ 1.0f };
    auto constexpr DIFFUSION_IN_MAX{ 0.0f };
    auto constexpr DIFFUSION_OUT_MIN{ 1.0f };
    auto constexpr DIFFUSION_OUT_MAX{ 8.0f };
    auto const valDiff{ speakerSetup.diffusion };

    auto const newDiffusion{ ((valDiff - DIFFUSION_IN_MIN) * (DIFFUSION_OUT_MAX - DIFFUSION_OUT_MIN)
                              / (DIFFUSION_IN_MAX - DIFFUSION_IN_MIN))
                             + DIFFUSION_OUT_MIN };

    mField.fieldExponent = newDiffusion;
}

//==============================================================================
void MbapSpatAlgorithm::updateSpatData(source_index_t const sourceIndex, SourceData const & sourceData) noexcept
{
    jassert(!isProbablyAudioThread());

    auto & data{ mData[sourceIndex] };

    auto & exchanger{ data.dataQueue };
    auto * ticket{ exchanger.acquire() };
    auto & spatData{ ticket->get() };

    if (sourceData.position) {
        auto const distXY{ std::sqrt(std::pow(sourceData.position->getCartesian().x, 2.0f)
                                     + std::pow(sourceData.position->getCartesian().y, 2.0f)) };
        auto const distZ{ sourceData.position->getCartesian().z };
        auto const attenuationRadius{ 1.0f };

        mbap(sourceData, spatData.gains, mField);

        // mbapAttenuation when source is under the floor
        if (distZ < 0.0f && distXY < attenuationRadius) {
            spatData.mbapSourceDistance = std::abs(distZ - attenuationRadius);
        } else if (distZ < 0.0f) {
            spatData.mbapSourceDistance = distXY + std::abs(distZ);
        } else {
            spatData.mbapSourceDistance = std::sqrt(std::pow(sourceData.position->getCartesian().x, 2.0f)
                                                    + std::pow(sourceData.position->getCartesian().y, 2.0f)
                                                    + std::pow(sourceData.position->getCartesian().z, 2.0f));
        }
    } else {
        spatData.gains = SpeakersSpatGains{};
    }

    exchanger.setMostRecent(ticket);
}

//==============================================================================
void MbapSpatAlgorithm::process(AudioConfig const & config,
                                SourceAudioBuffer & sourceBuffer,
                                SpeakerAudioBuffer & speakersBuffer,
                                [[maybe_unused]] juce::AudioBuffer<float> & stereoBuffer,
                                SourcePeaks const & sourcesPeaks,
                                SpeakersAudioConfig const * altSpeakerConfig)
{
    ASSERT_AUDIO_THREAD;

    auto const & gainInterpolation{ config.spatGainsInterpolation };

    auto const gainFactor{ std::pow(gainInterpolation, 0.1f) * 0.0099f + 0.99f };
    auto const numSamples{ sourceBuffer.getNumSamples() };

    auto const & speakersAudioConfig{ altSpeakerConfig ? *altSpeakerConfig : config.speakersAudioConfig };

    for (auto const & source : config.sourcesAudioConfig) {
        if (source.value.isMuted || source.value.directOut || sourcesPeaks[source.key] < SMALL_GAIN) {
            // speaker silent
            continue;
        }

        auto & data{ mData[source.key] };
        data.dataQueue.getMostRecent(data.currentData);
        if (data.currentData == nullptr) {
            // no spat data
            continue;
        }

        auto const & spatData{ data.currentData->get() };
        auto const & targetGains{ spatData.gains };
        auto & lastGains{ data.lastGains };

        // process attenuation if Player does not exist
        auto * inputSamples{ sourceBuffer[source.key].getWritePointer(0) };
        if (config.mbapAttenuationConfig.shouldProcess) {
            config.mbapAttenuationConfig.process(inputSamples,
                                                 numSamples,
                                                 spatData.mbapSourceDistance,
                                                 data.attenuationState);
        }

        // Process spatialization
        for (auto const & speaker : speakersAudioConfig) {
            if (speaker.value.isMuted || speaker.value.isDirectOutOnly || speaker.value.gain < SMALL_GAIN) {
                // speaker silent
                continue;
            }

            auto & currentGain{ lastGains[speaker.key] };
            auto const & targetGain{ targetGains[speaker.key] };
            auto * outputSamples{ speakersBuffer[speaker.key].getWritePointer(0) };
            auto const gainDiff{ targetGain - currentGain };
            auto const gainSlope{ gainDiff / narrow<float>(numSamples) };

            if (gainSlope == 0.0f || std::abs(gainDiff) < SMALL_GAIN) {
                // no interpolation
                currentGain = targetGain;
                if (currentGain >= SMALL_GAIN) {
                    juce::FloatVectorOperations::addWithMultiply(outputSamples, inputSamples, currentGain, numSamples);
                }
                continue;
            }

            if (gainInterpolation == 0.0f) {
                // linear interpolation over buffer size
                for (int sampleIndex{}; sampleIndex < numSamples; ++sampleIndex) {
                    currentGain += gainSlope;
                    outputSamples[sampleIndex] += inputSamples[sampleIndex] * currentGain;
                }
            } else {
                // log interpolation with 1st order filter
                if (targetGain < SMALL_GAIN) {
                    // targeting silence
                    for (int sampleIndex{}; sampleIndex < numSamples && currentGain >= SMALL_GAIN; ++sampleIndex) {
                        currentGain = targetGain + (currentGain - targetGain) * gainFactor;
                        outputSamples[sampleIndex] += inputSamples[sampleIndex] * currentGain;
                    }
                    continue;
                }

                // not targeting silence
                for (int sampleIndex{}; sampleIndex < numSamples; ++sampleIndex) {
                    currentGain = (currentGain - targetGain) * gainFactor + targetGain;
                    outputSamples[sampleIndex] += inputSamples[sampleIndex] * currentGain;
                }
            }
        }
    }
}

//==============================================================================
juce::Array<Triplet> MbapSpatAlgorithm::getTriplets() const noexcept
{
    JUCE_ASSERT_MESSAGE_THREAD;
    jassertfalse;
    return juce::Array<Triplet>{};
}

//==============================================================================
std::unique_ptr<AbstractSpatAlgorithm> MbapSpatAlgorithm::make(SpeakerSetup const & speakerSetup)
{
    JUCE_ASSERT_MESSAGE_THREAD;

    if (speakerSetup.numOfSpatializedSpeakers() < 2) {
        return std::make_unique<DummySpatAlgorithm>(Error::notEnoughCubeSpeakers);
    }

    return std::make_unique<MbapSpatAlgorithm>(speakerSetup);
}

} // namespace gris
