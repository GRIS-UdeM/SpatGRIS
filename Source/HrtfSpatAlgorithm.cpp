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
HrtfSpatAlgorithm::HrtfSpatAlgorithm(SpatMode const spatMode)
{
    auto const xml{ juce::XmlDocument{ BINAURAL_SPEAKER_SETUP_FILE }.getDocumentElement() };
    auto const speakerSetup{ SpeakerSetup::fromXml(*xml) };
    jassert(speakerSetup);

    switch (spatMode) {
    case SpatMode::vbap:
        mInnerSpatAlgorithm = std::make_unique<VbapSpatAlgorithm>(speakerSetup->speakers);
        return;
    case SpatMode::lbap:
        mInnerSpatAlgorithm = std::make_unique<LbapSpatAlgorithm>(speakerSetup->speakers);
        return;
    }
    jassertfalse;
}

//==============================================================================
void HrtfSpatAlgorithm::computeSpeakerGains(SourceData const & source, SpeakersSpatGains & gains) const noexcept
{
    mInnerSpatAlgorithm->computeSpeakerGains(source, gains);
}

//==============================================================================
juce::Array<Triplet> HrtfSpatAlgorithm::getTriplets() const noexcept
{
    return mInnerSpatAlgorithm->getTriplets();
}

//==============================================================================
bool HrtfSpatAlgorithm::hasTriplets() const noexcept
{
    return mInnerSpatAlgorithm->hasTriplets();
}
