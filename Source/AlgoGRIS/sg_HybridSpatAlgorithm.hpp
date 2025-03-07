/*
 This file is part of SpatGRIS.

 Developers: Samuel B�land, Olivier B�langer, Nicolas Masson

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

#include "sg_MbapSpatAlgorithm.hpp"
#include "sg_VbapSpatAlgorithm.hpp"

namespace gris
{
//==============================================================================
/** A spatialization algorithm that uses both Vbap (dome) and Mbap (cube).
 *
 * The selection of the algorithm is done on a per-source basis.
 */
class HybridSpatAlgorithm final : public AbstractSpatAlgorithm
{
    std::unique_ptr<AbstractSpatAlgorithm> mVbap{};
    std::unique_ptr<AbstractSpatAlgorithm> mMbap{};

public:
    //==============================================================================
    HybridSpatAlgorithm() = default;
    ~HybridSpatAlgorithm() override = default;
    SG_DELETE_COPY_AND_MOVE(HybridSpatAlgorithm)
    //==============================================================================
    /** Note: do not use this function directly. Use HybridSpatAlgorithm::make() instead. */
    explicit HybridSpatAlgorithm(SpeakerSetup const & speakerSetup);
    //==============================================================================
    void updateSpatData(source_index_t sourceIndex, SourceData const & sourceData) noexcept override;
    void process(AudioConfig const & config,
                 SourceAudioBuffer & sourcesBuffer,
                 SpeakerAudioBuffer & speakersBuffer,
                 juce::AudioBuffer<float> & stereoBuffer,
                 SourcePeaks const & sourcePeaks,
                 SpeakersAudioConfig const * altSpeakerConfig) override;
    [[nodiscard]] juce::Array<Triplet> getTriplets() const noexcept override;
    [[nodiscard]] bool hasTriplets() const noexcept override;
    [[nodiscard]] tl::optional<Error> getError() const noexcept override;
    //==============================================================================
    /** Instantiates an HybridSpatAlgorithm. Make sure to check getError() as this might fail. */
    static std::unique_ptr<AbstractSpatAlgorithm> make(SpeakerSetup const & speakerSetup);

private:
    //==============================================================================
    JUCE_LEAK_DETECTOR(HybridSpatAlgorithm)
};

} // namespace gris
