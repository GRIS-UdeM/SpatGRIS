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

#include "AbstractSpatAlgorithm.hpp"

#include "HrtfSpatAlgorithm.hpp"
#include "LbapSpatAlgorithm.hpp"
#include "StereoSpatAlgorithm.hpp"
#include "VbapSpatAlgorithm.hpp"
//#include "OwnedMap.hpp"
//#include "StrongArray.hpp"

//==============================================================================
void AbstractSpatAlgorithm::fixDirectOutsIntoPlace(SourcesData const & sources,
                                                   SpeakerSetup const & speakerSetup,
                                                   SpatData & spatData) const noexcept
{
    auto const getFakeSourceData = [&](SourceData const & source, SpeakerData const & speaker) -> SourceData {
        auto fakeSourceData{ source };
        switch (speakerSetup.spatMode) {
        case SpatMode::vbap:
            fakeSourceData.vector = speaker.vector.normalized();
            fakeSourceData.position = fakeSourceData.vector->toCartesian();
            break;
        case SpatMode::lbap:
            fakeSourceData.vector = speaker.vector;
            fakeSourceData.position = speaker.position;
            break;
        default:
            jassertfalse;
        }
        return fakeSourceData;
    };

    for (auto const & source : sources) {
        auto const & directOut{ source.value->directOut };
        if (!directOut) {
            continue;
        }

        auto const & speaker{ speakerSetup.speakers[*directOut] };

        auto & exchanger{ spatData[source.key] };
        auto * sourceSpatData{ exchanger.acquire() };

        updateSpatData(getFakeSourceData(*source.value, speaker), sourceSpatData->get());
        exchanger.setMostRecent(sourceSpatData);
    }
}

//==============================================================================
std::unique_ptr<AbstractSpatAlgorithm> AbstractSpatAlgorithm::make(SpeakerSetup const & speakerSetup,
                                                                   tl::optional<StereoMode> const stereoMode,
                                                                   SourcesData const & sources,
                                                                   SpatData & spatData)
{
    if (stereoMode) {
        switch (*stereoMode) {
        case StereoMode::hrtf:
            return std::make_unique<HrtfSpatAlgorithm>(speakerSetup, sources, spatData);
        case StereoMode::stereo:
            return std::make_unique<StereoSpatAlgorithm>(speakerSetup, sources, spatData);
        }
        jassertfalse;
    }

    switch (speakerSetup.spatMode) {
    case SpatMode::vbap:
        return std::make_unique<VbapSpatAlgorithm>(speakerSetup.speakers);
    case SpatMode::lbap:
        return std::make_unique<LbapSpatAlgorithm>(speakerSetup.speakers);
    }

    jassertfalse;
    return {};
}
