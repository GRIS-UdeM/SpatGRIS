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
#include "DummySpatAlgorithm.hpp"

//==============================================================================
VbapType getVbapType(SpeakersData const & speakers)
{
    auto const firstSpeaker{ *speakers.begin() };
    auto const firstZenith{ firstSpeaker.value->position.getPolar().elevation };
    auto const minZenith{ firstZenith - degrees_t{ 4.9f } };
    auto const maxZenith{ firstZenith + degrees_t{ 4.9f } };

    auto const areSpeakersOnSamePlane{ std::all_of(speakers.cbegin(),
                                                   speakers.cend(),
                                                   [&](SpeakersData::ConstNode const node) {
                                                       auto const zenith{ node.value->position.getPolar().elevation };
                                                       return zenith < maxZenith && zenith > minZenith;
                                                   }) };
    return areSpeakersOnSamePlane ? VbapType::twoD : VbapType::threeD;
}

//==============================================================================
VbapSpatAlgorithm::VbapSpatAlgorithm(SpeakersData const & speakers)
{
    JUCE_ASSERT_MESSAGE_THREAD;

    std::array<Position, MAX_NUM_SPEAKERS> loudSpeakers{};
    std::array<output_patch_t, MAX_NUM_SPEAKERS> outputPatches{};
    size_t index{};
    for (auto const & speaker : speakers) {
        if (speaker.value->isDirectOutOnly) {
            continue;
        }

        loudSpeakers[index] = speaker.value->position;
        outputPatches[index] = speaker.key;
        ++index;
    }
    auto const dimensions{ getVbapType(speakers) == VbapType::twoD ? 2 : 3 };
    auto const numSpeakers{ narrow<int>(index) };

    mSetupData = vbapInit(loudSpeakers, numSpeakers, dimensions, outputPatches);
}

//==============================================================================
void VbapSpatAlgorithm::updateSpatData(source_index_t const sourceIndex, SourceData const & sourceData) noexcept
{
    jassert(!isProbablyAudioThread());

    auto & spatDataQueue{ mData[sourceIndex].spatDataQueue };
    auto * ticket{ spatDataQueue.acquire() };
    auto & gains{ ticket->get() };

    if (sourceData.position) {
        vbapCompute(sourceData, gains, *mSetupData);
    } else {
        gains = SpeakersSpatGains{};
    }

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

//==============================================================================
std::unique_ptr<AbstractSpatAlgorithm> VbapSpatAlgorithm::make(SpeakerSetup const & speakerSetup,
                                                               juce::Component * parent)
{
    using error_t = tl::optional<juce::String>;

    auto const getDummy = [&](error_t error) {
        if (error) {
            juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::AlertIconType::InfoIcon,
                                                   "Disabled spatialization",
                                                   *error,
                                                   "Ok",
                                                   parent);
        }
        return std::make_unique<DummySpatAlgorithm>();
    };

    auto const getVbap = [&]() { return std::make_unique<VbapSpatAlgorithm>(speakerSetup.speakers); };

    static bool missingSpeakersErrorShown{};
    if (speakerSetup.numOfSpatializedSpeakers() < 3) {
        auto const error{ missingSpeakersErrorShown ? error_t{}
                                                    : error_t{ "The Dome mode needs at least 3 speakers.\n" } };
        missingSpeakersErrorShown = true;
        return getDummy(error);
    }
    missingSpeakersErrorShown = false;

    auto const dimensions{ getVbapType(speakerSetup.speakers) };

    if (dimensions == VbapType::threeD) {
        return getVbap();
    }

    // Verify that the speakers are not too far apart

    juce::Array<radians_t> angles{};
    angles.ensureStorageAllocated(speakerSetup.speakers.size());
    for (auto const & speaker : speakerSetup.speakers) {
        if (speaker.value->isDirectOutOnly) {
            continue;
        }
        angles.add(speaker.value->position.getPolar().azimuth.balanced());
    }

    angles.sort();

    static constexpr degrees_t MAX_ANGLE_DIFF{ 170.0f };

    auto const * invalidSpeaker{ std::adjacent_find(
        angles.begin(),
        angles.end(),
        [](radians_t const a, radians_t const b) { return b - a > MAX_ANGLE_DIFF; }) };

    auto const innerAreValid{ invalidSpeaker == angles.end() };
    auto const firstAndLastAreValid{ angles.getFirst() + degrees_t{ 360.0f } - angles.getLast() <= MAX_ANGLE_DIFF };

    static bool invalidAnglesErrorShown{};
    if (innerAreValid && firstAndLastAreValid) {
        invalidAnglesErrorShown = false;
        return getVbap();
    }

    auto const error{ invalidAnglesErrorShown
                          ? error_t{}
                          : error_t{ "If all speakers are at the same height, Dome mode requires speakers to be not "
                                     "more than 170 degrees apart from each others.\n" } };
    invalidAnglesErrorShown = true;
    return getDummy(error);
}
