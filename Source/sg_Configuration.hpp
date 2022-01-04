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

#include "sg_LogicStrucs.hpp"

#include <JuceHeader.h>

namespace gris
{
class Configuration
{
    struct XmlTags {
        static juce::String const MAIN_TAG;
    };

    juce::ApplicationProperties mApplicationProperties{};
    juce::PropertiesFile * mUserSettings{};

public:
    //==============================================================================
    Configuration();
    ~Configuration();
    SG_DELETE_COPY_AND_MOVE(Configuration)
    //==============================================================================
    void save(AppData const & appData) const;
    [[nodiscard]] AppData load() const;

private:
    //==============================================================================
    JUCE_LEAK_DETECTOR(Configuration)
};

} // namespace gris