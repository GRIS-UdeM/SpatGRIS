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

#include "AudioStructs.hpp"
#include "LogicStrucs.hpp"
#include "SpatMode.hpp"
#include "Triplet.hpp"

//==============================================================================
class AbstractSpatAlgorithm
{
public:
    AbstractSpatAlgorithm() = default;
    virtual ~AbstractSpatAlgorithm() = default;
    //==============================================================================
    AbstractSpatAlgorithm(AbstractSpatAlgorithm const &) = delete;
    AbstractSpatAlgorithm(AbstractSpatAlgorithm &&) = delete;
    AbstractSpatAlgorithm & operator=(AbstractSpatAlgorithm const &) = delete;
    AbstractSpatAlgorithm & operator=(AbstractSpatAlgorithm &&) = delete;
    //==============================================================================
    virtual void computeSpeakerGains(SourceData const & source, SpeakersSpatGains & gains) const noexcept = 0;
    [[nodiscard]] virtual juce::Array<Triplet> getTriplets() const noexcept = 0;
    [[nodiscard]] virtual bool hasTriplets() const noexcept = 0;
    //==============================================================================
    [[nodiscard]] static std::unique_ptr<AbstractSpatAlgorithm>
        make(SpatMode spatMode, tl::optional<StereoMode> stereoMode, SpeakersData const & speakers);

private:
    JUCE_LEAK_DETECTOR(AbstractSpatAlgorithm)
};
