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

#include "VbapSpatAlgorithm.hpp"

//==============================================================================
VbapType getVbapType(SpeakersData const & speakers)
{
    auto const firstSpeaker{ *speakers.begin() };
    auto const firstZenith{ firstSpeaker.value->vector.elevation };
    auto const minZenith{ firstZenith - degrees_t{ 4.9f } };
    auto const maxZenith{ firstZenith + degrees_t{ 4.9f } };

    auto const areSpeakersOnSamePlane{ std::all_of(speakers.cbegin(),
                                                   speakers.cend(),
                                                   [&](SpeakersData::ConstNode const node) {
                                                       auto const zenith{ node.value->vector.elevation };
                                                       return zenith < maxZenith && zenith > minZenith;
                                                   }) };
    return areSpeakersOnSamePlane ? VbapType::twoD : VbapType::threeD;
}

//==============================================================================
VbapSpatAlgorithm::VbapSpatAlgorithm(SpeakersData const & speakers)
{
    JUCE_ASSERT_MESSAGE_THREAD;

    std::array<LoudSpeaker, MAX_NUM_SPEAKERS> loudSpeakers{};
    std::array<output_patch_t, MAX_NUM_SPEAKERS> outputPatches{};
    size_t index{};
    for (auto const & speaker : speakers) {
        if (speaker.value->isDirectOutOnly) {
            continue;
        }

        loudSpeakers[index].coords = speaker.value->position;
        loudSpeakers[index].angles = speaker.value->vector;
        outputPatches[index] = speaker.key;
        ++index;
    }
    auto const dimensions{ getVbapType(speakers) == VbapType::twoD ? 2 : 3 };
    auto const numSpeakers{ narrow<int>(index) };

    mSetupData.reset(vbapInit(loudSpeakers, numSpeakers, dimensions, outputPatches));
}

//==============================================================================
void VbapSpatAlgorithm::updateSpatData(source_index_t const sourceIndex, SourceData const & sourceData) noexcept
{
    jassert(!isProbablyAudioThread());
    jassert(sourceData.vector);

    auto & spatDataQueue{ mData[sourceIndex].spatDataQueue };
    auto * ticket{ spatDataQueue.acquire() };

    vbapCompute(sourceData, ticket->get(), *mSetupData);
    spatDataQueue.setMostRecent(ticket);
}

//==============================================================================
void VbapSpatAlgorithm::process(AudioConfig const & config,
                                SourceAudioBuffer & sourcesBuffer,
                                SpeakerAudioBuffer & speakersBuffer,
                                SourcePeaks const & sourcePeaks,
                                SpeakersAudioConfig const * altSpeakerConfig)
{
    ASSERT_AUDIO_THREAD;

    auto const & gainInterpolation{ config.spatGainsInterpolation };
    auto const gainFactor{ std::pow(gainInterpolation, 0.1f) * 0.0099f + 0.99f };

    auto const & speakersAudioConfig{ altSpeakerConfig ? *altSpeakerConfig : config.speakersAudioConfig };

    auto const numSamples{ sourcesBuffer.getNumSamples() };
    for (auto const & source : config.sourcesAudioConfig) {
        if (source.value.isMuted || source.value.directOut || sourcePeaks[source.key] < SMALL_GAIN) {
            continue;
        }

        auto & data{ mData[source.key] };

        data.spatDataQueue.getMostRecent(data.currentSpatData);
        if (data.currentSpatData == nullptr) {
            continue;
        }
        auto const & gains{ data.currentSpatData->get() };

        auto & lastGains{ data.lastGains };
        auto const * inputSamples{ sourcesBuffer[source.key].getReadPointer(0) };

        for (auto const & speaker : speakersAudioConfig) {
            if (speaker.value.isMuted || speaker.value.isDirectOutOnly || speaker.value.gain < SMALL_GAIN) {
                continue;
            }
            auto & currentGain{ lastGains[speaker.key] };
            auto const & targetGain{ gains[speaker.key] };
            auto * outputSamples{ speakersBuffer[speaker.key].getWritePointer(0) };
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
}

//==============================================================================
juce::Array<Triplet> VbapSpatAlgorithm::getTriplets() const noexcept
{
    JUCE_ASSERT_MESSAGE_THREAD;
    jassert(hasTriplets());
    return vbapExtractTriplets(*mSetupData);
}

//==============================================================================
bool VbapSpatAlgorithm::hasTriplets() const noexcept
{
    JUCE_ASSERT_MESSAGE_THREAD;
    if (!mSetupData) {
        return false;
    }
    return mSetupData->dimension == 3;
}
