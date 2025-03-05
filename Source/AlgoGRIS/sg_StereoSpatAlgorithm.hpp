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

namespace gris
{
using StereoSpeakerGains = std::array<float, 2>;
using StereoGainsUpdater = AtomicUpdater<StereoSpeakerGains>;

struct StereoSourceData {
    StereoGainsUpdater gainsUpdater{};
    StereoGainsUpdater::Token * currentGains{};
    StereoSpeakerGains lastGains{};
};

using StereoSourcesData = StrongArray<source_index_t, StereoSourceData, MAX_NUM_SOURCES>;

//==============================================================================
class StereoSpatAlgorithm final : public AbstractSpatAlgorithm
{
    std::unique_ptr<AbstractSpatAlgorithm> mInnerAlgorithm{};
    StereoSourcesData mData{};

public:
    //==============================================================================
    StereoSpatAlgorithm(SpeakerSetup const & speakerSetup,
                        SpatMode const & projectSpatMode,
                        SourcesData const & sources);
    ~StereoSpatAlgorithm() override = default;
    SG_DELETE_COPY_AND_MOVE(StereoSpatAlgorithm)
    //==============================================================================
    void updateSpatData(source_index_t sourceIndex, SourceData const & sourceData) noexcept override;
    void process(AudioConfig const & config,
                 SourceAudioBuffer & sourcesBuffer,
                 SpeakerAudioBuffer & speakersBuffer,
                 juce::AudioBuffer<float> & stereoBuffer,
                 SourcePeaks const & sourcePeaks,
                 SpeakersAudioConfig const * altSpeakerConfig) override;
    [[nodiscard]] juce::Array<Triplet> getTriplets() const noexcept override;
    [[nodiscard]] bool hasTriplets() const noexcept override { return false; }
    [[nodiscard]] tl::optional<Error> getError() const noexcept override { return tl::nullopt; }
    //==============================================================================
    static std::unique_ptr<AbstractSpatAlgorithm>
        make(SpeakerSetup const & speakerSetup, SpatMode const & projectSpatMode, SourcesData const & sources);

private:
    JUCE_LEAK_DETECTOR(StereoSpatAlgorithm)
};

} // namespace gris
