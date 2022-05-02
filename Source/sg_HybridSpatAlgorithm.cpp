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

#include "sg_HybridSpatAlgorithm.hpp"

namespace gris
{
//==============================================================================
HybridSpatAlgorithm::HybridSpatAlgorithm(SpeakersData const & speakersData)
    : mVbap(std::make_unique<VbapSpatAlgorithm>(speakersData))
    , mLbap(std::make_unique<LbapSpatAlgorithm>(speakersData))
{
}

//==============================================================================
void HybridSpatAlgorithm::updateSpatData(source_index_t const sourceIndex, SourceData const & sourceData) noexcept
{
    if (!sourceData.position.has_value()) {
        // resetting a position should reset both algorithms
        mVbap->updateSpatData(sourceIndex, sourceData);
        mLbap->updateSpatData(sourceIndex, sourceData);
        return;
    }

    // Valid position: only send to the right algorithm
    switch (sourceData.hybridSpatMode) {
    case SpatMode::vbap:
        mVbap->updateSpatData(sourceIndex, sourceData);
        return;
    case SpatMode::lbap:
        mLbap->updateSpatData(sourceIndex, sourceData);
        return;
    case SpatMode::hybrid:
    case SpatMode::invalid:
        break;
    }
    jassertfalse;
}

//==============================================================================
void HybridSpatAlgorithm::process(AudioConfig const & config,
                                  SourceAudioBuffer & sourcesBuffer,
                                  SpeakerAudioBuffer & speakersBuffer,
                                  juce::AudioBuffer<float> & stereoBuffer,
                                  SourcePeaks const & sourcePeaks,
                                  SpeakersAudioConfig const * altSpeakerConfig)
{
    mVbap->process(config, sourcesBuffer, speakersBuffer, stereoBuffer, sourcePeaks, altSpeakerConfig);
    mLbap->process(config, sourcesBuffer, speakersBuffer, stereoBuffer, sourcePeaks, altSpeakerConfig);
}

//==============================================================================
juce::Array<Triplet> HybridSpatAlgorithm::getTriplets() const noexcept
{
    return mVbap->getTriplets();
}

//==============================================================================
bool HybridSpatAlgorithm::hasTriplets() const noexcept
{
    return true;
}

//==============================================================================
tl::optional<AbstractSpatAlgorithm::Error> HybridSpatAlgorithm::getError() const noexcept
{
    return mVbap->getError().disjunction(mLbap->getError());
}

//==============================================================================
std::unique_ptr<AbstractSpatAlgorithm> HybridSpatAlgorithm::make(SpeakerSetup const & speakerSetup)
{
    return std::make_unique<HybridSpatAlgorithm>(speakerSetup.speakers);
}

} // namespace gris