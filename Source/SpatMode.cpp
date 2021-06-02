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

#include "SpatMode.hpp"

#include "Constants.hpp"

//==============================================================================
juce::StringArray const SPAT_MODE_STRINGS{ "Dome", "Cube", "Binaural", "Stereo" };
juce::StringArray const SPAT_MODE_TOOLTIPS{
    "Equidistant speaker dome implemented using the VBAP algorithm",
    "Free-form speaker setup implemented using the LBAP algorithm",
};
juce::StringArray const STEREO_MODE_STRINGS{ "Binaural", "Stereo" };
juce::StringArray const STEREO_TOOLTIPS{ "HRTF transfer", "Dumb Left/Right panning" };

//==============================================================================
juce::String const & spatModeToString(SpatMode const mode)
{
    auto const index{ static_cast<int>(mode) };
    jassert(index >= 0 && index < SPAT_MODE_STRINGS.size());
    return SPAT_MODE_STRINGS[index];
}

//==============================================================================
juce::String const & spatModeToTooltip(SpatMode mode)
{
    auto const index{ static_cast<int>(mode) };
    jassert(index >= 0 && index < SPAT_MODE_TOOLTIPS.size());
    return SPAT_MODE_TOOLTIPS[index];
}

//==============================================================================
tl::optional<SpatMode> stringToSpatMode(juce::String const & string)
{
    auto const index{ SPAT_MODE_STRINGS.indexOf(string, true) };
    if (index < 0) {
        return tl::nullopt;
    }
    return static_cast<SpatMode>(index);
}

//==============================================================================
juce::String const & stereoModeToString(StereoMode const mode)
{
    auto const index{ static_cast<int>(mode) };
    jassert(index >= 0 && index < STEREO_MODE_STRINGS.size());
    return STEREO_MODE_STRINGS[index];
}

//==============================================================================
tl::optional<StereoMode> stringToStereoMode(juce::String const & string)
{
    auto const index{ STEREO_MODE_STRINGS.indexOf(string, true) };
    if (index < 0) {
        return tl::nullopt;
    }
    return static_cast<StereoMode>(index);
}
