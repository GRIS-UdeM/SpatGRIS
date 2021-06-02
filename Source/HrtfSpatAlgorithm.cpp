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

#include "HrtfSpatAlgorithm.hpp"

#include "LbapSpatAlgorithm.hpp"
#include "VbapSpatAlgorithm.hpp"

//==============================================================================
HrtfSpatAlgorithm::HrtfSpatAlgorithm(SpeakerSetup const & speakerSetup,
                                     SourcesData const & sources,
                                     SpatData & spatData)
{
    auto const binauralXml{ juce::XmlDocument{ BINAURAL_SPEAKER_SETUP_FILE }.getDocumentElement() };
    jassert(binauralXml);
    auto const binauralSpeakerSetup{ SpeakerSetup::fromXml(*binauralXml) };
    jassert(binauralSpeakerSetup);
    auto const & binauralSpeakerData{ binauralSpeakerSetup->speakers };

    switch (speakerSetup.spatMode) {
    case SpatMode::vbap:
        mInnerAlgorithm = std::make_unique<VbapSpatAlgorithm>(binauralSpeakerData);
        break;
    case SpatMode::lbap:
        mInnerAlgorithm = std::make_unique<LbapSpatAlgorithm>(binauralSpeakerData);
        break;
    }

    jassert(mInnerAlgorithm);

    fixDirectOutsIntoPlace(sources, speakerSetup, spatData);
}

//==============================================================================
void HrtfSpatAlgorithm::updateSpatData(SourceData const & sourceData, SourceSpatData & spatData) const noexcept
{
    mInnerAlgorithm->updateSpatData(sourceData, spatData);
}

//==============================================================================
juce::Array<Triplet> HrtfSpatAlgorithm::getTriplets() const noexcept
{
    return mInnerAlgorithm->getTriplets();
}

//==============================================================================
bool HrtfSpatAlgorithm::hasTriplets() const noexcept
{
    return mInnerAlgorithm->hasTriplets();
}
