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

#pragma once

#include "sg_AbstractSpatAlgorithm.hpp"
#include "Containers/sg_StrongArray.hpp"
#include "Containers/sg_TaggedAudioBuffer.hpp"
#include "Implementations/sg_mbap.hpp"

namespace gris
{
struct MbapSpatData {
    SpeakersSpatGains gains{};
    float mbapSourceDistance{};
};

using MbapSpatDataQueue = AtomicUpdater<MbapSpatData>;

struct MbapSourceData {
    MbapSpatDataQueue dataQueue{};
    MbapSpatDataQueue::Token * currentData{};
    MbapSourceAttenuationState attenuationState{};
    SpeakersSpatGains lastGains{};
};

//==============================================================================
class MbapSpatAlgorithm final : public AbstractSpatAlgorithm
{
    MbapField mField{};
    StrongArray<source_index_t, MbapSourceData, MAX_NUM_SOURCES> mData{};

public:
    //==============================================================================
    MbapSpatAlgorithm() = delete;
    ~MbapSpatAlgorithm() override = default;
    SG_DELETE_COPY_AND_MOVE(MbapSpatAlgorithm)
    //==============================================================================
    explicit MbapSpatAlgorithm(SpeakerSetup const & speakerSetup);
    //==============================================================================
    void updateSpatData(source_index_t sourceIndex, SourceData const & sourceData) noexcept override;
    void process(AudioConfig const & config,
                 SourceAudioBuffer & sourceBuffer,
                 SpeakerAudioBuffer & speakersBuffer,
                 juce::AudioBuffer<float> & stereoBuffer,
                 SourcePeaks const & sourcesPeaks,
                 SpeakersAudioConfig const * altSpeakerConfig) override;
    [[nodiscard]] juce::Array<Triplet> getTriplets() const noexcept override;
    [[nodiscard]] bool hasTriplets() const noexcept override { return false; }
    [[nodiscard]] tl::optional<Error> getError() const noexcept override { return tl::nullopt; }
    //==============================================================================
    static std::unique_ptr<AbstractSpatAlgorithm> make(SpeakerSetup const & speakerSetup);

private:
    JUCE_LEAK_DETECTOR(MbapSpatAlgorithm)
};

} // namespace gris
