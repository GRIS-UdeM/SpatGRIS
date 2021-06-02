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

#include "StereoSpatAlgorithm.hpp"

//==============================================================================
void StereoSpatAlgorithm::updateSpatData(SourceData const & sourceData, SourceSpatData & spatData) const noexcept
{
    jassert(sourceData.vector);
    using fast = juce::dsp::FastMathApproximations;

    static auto const TO_GAIN = [](radians_t const angle) { return fast::sin(angle.get()) / 2.0f + 0.5f; };

    auto & gains{ spatData.gains };

    gains[output_patch_t{ 1 }] = TO_GAIN(sourceData.vector->azimuth - HALF_PI);
    gains[output_patch_t{ 2 }] = TO_GAIN(sourceData.vector->azimuth + HALF_PI);
}

//==============================================================================
juce::Array<Triplet> StereoSpatAlgorithm::getTriplets() const noexcept
{
    jassertfalse;
    return juce::Array<Triplet>{};
}

//==============================================================================
StereoSpatAlgorithm::StereoSpatAlgorithm(SpeakerSetup const & speakerSetup,
                                         SourcesData const & sources,
                                         SpatData & spatData)
{
    fixDirectOutsIntoPlace(sources, speakerSetup, spatData);
}
