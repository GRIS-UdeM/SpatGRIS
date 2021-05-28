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

#include "LbapSpatAlgorithm.hpp"
#include "StereoSpatAlgorithm.hpp"
#include "VbapSpatAlgorithm.hpp"

//==============================================================================
std::unique_ptr<AbstractSpatAlgorithm> AbstractSpatAlgorithm::make(SpatMode const spatMode,
                                                                   tl::optional<StereoMode> const stereoMode,
                                                                   SpeakersData const & speakers)
{
    static auto const FROM_SPAT_MODE
        = [](SpatMode const spatMode, SpeakersData const & speakers) -> std::unique_ptr<AbstractSpatAlgorithm> {
        switch (spatMode) {
        case SpatMode::vbap:
            return std::make_unique<VbapSpatAlgorithm>(speakers);
        case SpatMode::lbap:
            return std::make_unique<LbapSpatAlgorithm>(speakers);
        }
        jassertfalse;
        return nullptr;
    };

    if (stereoMode) {
        switch (*stereoMode) {
        case StereoMode::hrtf: {
            auto const xml{ juce::XmlDocument{ BINAURAL_SPEAKER_SETUP_FILE }.getDocumentElement() };
            jassert(xml);
            auto const setup{ SpeakerSetup::fromXml(*xml) };
            jassert(setup);
            return FROM_SPAT_MODE(spatMode, setup->speakers);
        }
        case StereoMode::stereo:
            return std::make_unique<StereoSpatAlgorithm>();
        }
        jassertfalse;
    }

    return FROM_SPAT_MODE(spatMode, speakers);
}
