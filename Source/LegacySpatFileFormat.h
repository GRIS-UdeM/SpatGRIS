#pragma once

#include "LogicStrucs.hpp"

tl::optional<std::pair<SpeakerSetup, SpatMode>> readLegacySpeakerSetup(juce::XmlElement const & xml);
tl::optional<SpatGrisProjectData> readLegacyProjectFile(juce::XmlElement const & xml);