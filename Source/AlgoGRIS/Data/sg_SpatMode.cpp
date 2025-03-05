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

#include "sg_SpatMode.hpp"
#include "sg_constants.hpp"

namespace gris
{
//==============================================================================
juce::StringArray const SPAT_MODE_STRINGS{ "Dome", "Cube", "Hybrid" };
juce::StringArray const SPAT_MODE_TOOLTIPS{ "Equidistant speaker dome implemented using the VBAP algorithm",
                                            "Free-form speaker setup implemented using the MBAP algorithm",
                                            "By-source selection of Dome or Cube spatialization" };

#ifdef USE_DOPPLER
juce::StringArray const STEREO_MODE_STRINGS{ "Binaural", "Stereo", "Doppler" };
juce::StringArray const STEREO_TOOLTIPS{ "HRTF transfer", "Dumb Left/Right panning", "Doppler shifted sources" };
#else
juce::StringArray const STEREO_MODE_STRINGS{ "Binaural", "Stereo" };
juce::StringArray const STEREO_TOOLTIPS{ "HRTF transfer", "Dumb Left/Right panning" };
#endif

//==============================================================================
template<typename T>
static juce::String const & enumToString(T const value, juce::StringArray const & stringArray) noexcept
{
    auto const index{ static_cast<int>(value) };
    jassert(index >= 0 && index < stringArray.size());
    return stringArray[index];
}

//==============================================================================
template<typename T>
static tl::optional<T> stringToEnum(juce::String const & string, juce::StringArray const & stringArray) noexcept
{
    auto const index{ stringArray.indexOf(string, true) };
    if (index < 0) {
        return tl::nullopt;
    }
    return static_cast<T>(index);
}

//==============================================================================
juce::String const & spatModeToString(SpatMode const mode)
{
    return enumToString(mode, SPAT_MODE_STRINGS);
}

//==============================================================================
juce::String const & spatModeToTooltip(SpatMode const mode)
{
    return enumToString(mode, SPAT_MODE_TOOLTIPS);
}

//==============================================================================
tl::optional<SpatMode> stringToSpatMode(juce::String const & string)
{
    return stringToEnum<SpatMode>(string, SPAT_MODE_STRINGS);
}

//==============================================================================
juce::String const & stereoModeToString(StereoMode const mode)
{
    return enumToString(mode, STEREO_MODE_STRINGS);
}

//==============================================================================
tl::optional<StereoMode> stringToStereoMode(juce::String const & string)
{
    return stringToEnum<StereoMode>(string, STEREO_MODE_STRINGS);
}

} // namespace gris
