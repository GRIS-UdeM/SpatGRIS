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

#pragma once

#include "sg_LbapSpatAlgorithm.hpp"
#include "sg_VbapSpatAlgorithm.hpp"

/** A spatialization algorithm that uses both Vbap (dome) and Lbap (cube).
 *
 * The selection of the algorithm is done on a per-source basis.
 */
class HybridSpatAlgorithm : public AbstractSpatAlgorithm
{
    std::unique_ptr<AbstractSpatAlgorithm> mVbap{};
    std::unique_ptr<AbstractSpatAlgorithm> mLbap{};

public:
    //==============================================================================
    HybridSpatAlgorithm() = default;
    ~HybridSpatAlgorithm() override = default;
    //==============================================================================
    HybridSpatAlgorithm(HybridSpatAlgorithm const &) = delete;
    HybridSpatAlgorithm(HybridSpatAlgorithm &&) = delete;
    HybridSpatAlgorithm & operator=(HybridSpatAlgorithm const &) = delete;
    HybridSpatAlgorithm & operator=(HybridSpatAlgorithm &&) = delete;
    //==============================================================================
    /** Note: do not use this function directly. Use HybridSpatAlgorithm::make() instead. */
    explicit HybridSpatAlgorithm(ColdSpeakersData const & speakersData);
    //==============================================================================
    void updateSpatData(source_index_t sourceIndex, ColdSourceData const & sourceData) noexcept override;
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
    static std::unique_ptr<AbstractSpatAlgorithm> make(ColdSpeakerSetup const & speakerSetup);

private:
    //==============================================================================
    JUCE_LEAK_DETECTOR(HybridSpatAlgorithm)
};