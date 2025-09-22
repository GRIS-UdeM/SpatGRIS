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

#include "Data/sg_Macros.hpp"

#include <JuceHeader.h>

namespace gris
{
//==============================================================================
class MinSizedComponent
    : public juce::Component
    , public juce::SettableTooltipClient
{
public:
    MinSizedComponent() = default;
    ~MinSizedComponent() override = default;
    SG_DELETE_COPY_AND_MOVE(MinSizedComponent)
    //==============================================================================
    [[nodiscard]] virtual int getMinWidth() const noexcept = 0;
    [[nodiscard]] virtual int getMinHeight() const noexcept = 0;

private:
    //==============================================================================
    JUCE_LEAK_DETECTOR(MinSizedComponent)
};

} // namespace gris
