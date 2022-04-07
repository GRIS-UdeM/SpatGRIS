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
#include "sg_StrongArray.hpp"
#include "sg_TaggedAudioBuffer.hpp"
#include "sg_lbap.hpp"

namespace gris
{
struct LbapSpatData {
    SpeakersSpatGains gains{};
    float lbapSourceDistance{};
};

using LbapSpatDataQueue = AtomicUpdater<LbapSpatData>;

struct LbapSourceData {
    LbapSpatDataQueue dataQueue{};
    LbapSpatDataQueue::Token * currentData{};
    LbapSourceAttenuationState attenuationState{};
    SpeakersSpatGains lastGains{};
};

//==============================================================================
class LbapSpatAlgorithm final : public AbstractSpatAlgorithm
{
    LbapField mField{};
    StrongArray<source_index_t, LbapSourceData, MAX_NUM_SOURCES> mData{};

public:
    //==============================================================================
    LbapSpatAlgorithm() = delete;
    ~LbapSpatAlgorithm() override = default;
    SG_DELETE_COPY_AND_MOVE(LbapSpatAlgorithm)
    //==============================================================================
    explicit LbapSpatAlgorithm(SpeakersData const & speakers);
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
    JUCE_LEAK_DETECTOR(LbapSpatAlgorithm)
};

} // namespace gris