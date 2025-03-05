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

#include "sg_LegacySpatFileFormat.hpp"

#include "sg_LegacyLbapPosition.hpp"
#include "sg_LogicStrucs.hpp"

namespace gris
{
//==============================================================================
tl::optional<SpeakerSetup> readLegacySpeakerSetup(juce::XmlElement const & xml)
{
    if (!xml.hasTagName("SpeakerSetup")) {
        return tl::nullopt;
    }

    auto const spatMode{ static_cast<SpatMode>(xml.getIntAttribute("SpatMode")) };
    if (spatMode != SpatMode::mbap && spatMode != SpatMode::vbap) {
        jassertfalse;
        return tl::nullopt;
    }

    juce::Array<std::pair<int, output_patch_t>> layout;
    juce::OwnedArray<SpeakerData> duplicatedSpeakers{};
    SpeakerSetup result{};

    for (auto const * ring : xml.getChildIterator()) {
        if (ring->hasTagName("Ring")) {
            for (auto const * spk : ring->getChildIterator()) {
                if (spk->hasTagName("Speaker")) {
                    // layout
                    auto const layoutIndex{ spk->getIntAttribute("LayoutIndex") - 1 };
                    output_patch_t outputPatch{ spk->getIntAttribute("OutputPatch") };
                    jassert(LEGAL_OUTPUT_PATCH_RANGE.contains(outputPatch));
                    if (!LEGAL_OUTPUT_PATCH_RANGE.contains(outputPatch)) {
                        return tl::nullopt;
                    }

                    // position
                    radians_t const azimuth{ (degrees_t{ static_cast<float>(-spk->getDoubleAttribute("Azimuth", 0.0)) }
                                              + degrees_t{ 90.0f })
                                                 .centered() };
                    radians_t const zenith{
                        degrees_t{ static_cast<float>(spk->getDoubleAttribute("Zenith", 0.0)) }.centered()
                    };
                    auto const length{ static_cast<float>(spk->getDoubleAttribute("Radius", 1.0)) };
                    auto const position{ spatMode == SpatMode::mbap
                                             ? LegacyLbapPosition{ azimuth, zenith, length }.toPosition()
                                             : Position{ PolarVector{ azimuth, zenith, length } } };

                    // audio params
                    dbfs_t const gain{ static_cast<float>(spk->getDoubleAttribute("Gain", 0.0)) };
                    hz_t const highpass{ static_cast<float>(spk->getDoubleAttribute("HighPassCutoff", 0.0)) };
                    auto const isDirectOutOnly{ spk->getBoolAttribute("DirectOut", false) };

                    // build data
                    auto speakerData{ std::make_unique<SpeakerData>() };
                    speakerData->position = position;
                    speakerData->gain = gain;
                    speakerData->highpassData
                        = (highpass == hz_t{} ? tl::optional<SpeakerHighpassData>{} : SpeakerHighpassData{ highpass });
                    speakerData->isDirectOutOnly = isDirectOutOnly;

                    if (!result.speakers.contains(outputPatch)) {
                        layout.add(std::make_pair(layoutIndex, outputPatch));
                        result.speakers.add(outputPatch, std::move(speakerData));
                    } else {
                        duplicatedSpeakers.add(std::move(speakerData));
                    }
                }
            }
        }
    }

    static auto const GET_MAX_OUTPUT_PATCH = [&](SpeakersData const & speakers) {
        if (speakers.size() == 0) {
            return output_patch_t{};
        }
        auto node{ speakers.cbegin() };
        auto maxOutputPatch{ node->key };
        while (++node != speakers.cend()) {
            if (node->key > maxOutputPatch) {
                maxOutputPatch = node->key;
            }
        }
        return maxOutputPatch;
    };

    std::sort(layout.begin(), layout.end());

    auto maxOutputPatch{ GET_MAX_OUTPUT_PATCH(result.speakers) };
    auto maxLayoutIndex{ layout.getLast().first };

    for (auto * speaker : duplicatedSpeakers) {
        auto const outputPatch{ ++maxOutputPatch };
        auto const layoutIndex{ ++maxLayoutIndex };
        result.speakers.add(outputPatch, std::unique_ptr<SpeakerData>(speaker));
        layout.add(std::make_pair(layoutIndex, outputPatch));
    }
    duplicatedSpeakers.clearQuick(false);

    result.ordering.resize(layout.size());
    std::transform(layout.begin(),
                   layout.end(),
                   result.ordering.begin(),
                   [](std::pair<int, output_patch_t> const & indexOutputPair) { return indexOutputPair.second; });

    auto const getCorrectedSpatMode = [&]() {
        if (!result.isDomeLike()) {
            return SpatMode::mbap;
        }
        return spatMode;
    };

    result.spatMode = getCorrectedSpatMode();

    return result;
}

//==============================================================================
tl::optional<ProjectData> readLegacyProjectFile(juce::XmlElement const & xml)
{
    if (!xml.hasTagName("SpatServerGRIS_Preset") && !xml.hasTagName("ServerGRIS_Preset")) {
        return tl::nullopt;
    }

    auto const oscPort{ xml.getIntAttribute("OSC_Input_Port", DEFAULT_OSC_INPUT_PORT) }; // TODO : validate value

    // auto const numInputs{ xml.getStringAttribute("Number_Of_Inputs").getIntValue() }; /* UNUSED */

    auto const masterGain{ LEGAL_MASTER_GAIN_RANGE.clipValue(
        dbfs_t{ static_cast<float>(xml.getDoubleAttribute("Master_Gain_Out", 0.0)) }) };
    auto const gainInterpolation{ LEGAL_GAIN_INTERPOLATION_RANGE.clipValue(
        static_cast<float>(xml.getDoubleAttribute("Master_Interpolation", 0.1))) };

    ProjectData result{};

    for (auto const * source : xml.getChildIterator()) {
        if (source->hasTagName("Input")) {
            source_index_t const index{ source->getIntAttribute("Index") };
            jassert(LEGAL_SOURCE_INDEX_RANGE.contains(index));
            if (!LEGAL_SOURCE_INDEX_RANGE.contains(index)) {
                return tl::nullopt;
            }
            auto const red{ static_cast<float>(source->getDoubleAttribute("R", 1.0f)) };
            auto const green{ static_cast<float>(source->getDoubleAttribute("G", 1.0f)) };
            auto const blue{ static_cast<float>(source->getDoubleAttribute("B", 1.0f)) };
            auto const color{ juce::Colour::fromFloatRGBA(red, green, blue, 1.0f) };
            tl::optional<output_patch_t> directOut{};
            if (source->getIntAttribute("DirectOut") != 0) {
                directOut = output_patch_t{ source->getIntAttribute("DirectOut") };
                jassert(LEGAL_OUTPUT_PATCH_RANGE.contains(*directOut));
                if (!LEGAL_OUTPUT_PATCH_RANGE.contains(*directOut)) {
                    return tl::nullopt;
                }
            }

            auto newSourceData{ std::make_unique<SourceData>() };
            newSourceData->colour = color;
            newSourceData->directOut = directOut;

            result.sources.add(index, std::move(newSourceData));
        }
    }

    result.oscPort = oscPort;
    result.masterGain = masterGain;
    result.spatGainsInterpolation = gainInterpolation;

    return result;
}

} // namespace gris
