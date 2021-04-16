#include "SpatMode.hpp"

#include "constants.hpp"

//==============================================================================
juce::StringArray const SPAT_MODE_STRINGS{ "DOME", "CUBE", "BINAURAL", "STEREO" };

//==============================================================================
juce::String spatModeToString(SpatMode const mode)
{
    auto const index{ static_cast<int>(mode) };
    jassert(index >= 0 && index < SPAT_MODE_STRINGS.size());
    return SPAT_MODE_STRINGS[index];
}

//==============================================================================
tl::optional<SpatMode> stringToSpatMode(juce::String const & string)
{
    auto const index{ SPAT_MODE_STRINGS.indexOf(string) };
    if (index < 0) {
        return tl::nullopt;
    }
    return static_cast<SpatMode>(index);
}
