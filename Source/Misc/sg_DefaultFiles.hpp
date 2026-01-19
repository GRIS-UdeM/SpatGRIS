#pragma once

#include <JuceHeader.h>

#include <tl/optional.hpp>

namespace gris
{

#if defined(__linux__) || defined(WIN32)
juce::File const CURRENT_WORKING_DIR{ juce::File::getCurrentWorkingDirectory() };
auto const RESOURCES_DIR{ CURRENT_WORKING_DIR.getChildFile("Resources") };
#elif defined(__APPLE__)
juce::File const CURRENT_WORKING_DIR = juce::File::getSpecialLocation(juce::File::currentApplicationFile);
auto const RESOURCES_DIR{ CURRENT_WORKING_DIR.getChildFile("Contents").getChildFile("Resources") };
#else
static_assert(false, "What are you building this on?");
#endif

#ifdef __APPLE__
static auto const TEMPLATES_DIR{ RESOURCES_DIR };
#else
static auto const TEMPLATES_DIR{ RESOURCES_DIR.getChildFile("templates") };
#endif

extern juce::File const SPEAKER_TEMPLATES_DIR;
extern juce::File const PROJECT_TEMPLATES_DIR;
extern juce::File const CURRENT_WORKING_DIR;
extern juce::File const SPLASH_SCREEN_FILE;
extern juce::File const DEFAULT_PROJECT_FILE;
extern juce::File const DEFAULT_PROJECT_DIRECTORY;
extern juce::File const DEFAULT_SPEAKER_SETUP_FILE;
extern juce::File const BINAURAL_SPEAKER_SETUP_FILE;
extern juce::File const STEREO_SPEAKER_SETUP_FILE;
extern juce::File const DEFAULT_CUBE_PROJECT;
extern juce::File const DEFAULT_CUBE_SPEAKER_SETUP;
extern juce::File const MANUAL_FILE_EN;
extern juce::File const MANUAL_FILE_FR;
extern juce::File const ICON_SMALL_FILE;

//==============================================================================
struct FileTemplate {
    juce::String name{};
    juce::CommandID commandId{};
    juce::File path{};

    bool operator<(FileTemplate const & other) const noexcept;
};

tl::optional<FileTemplate const &> commandIdToTemplate(juce::CommandID commandId);

juce::Array<FileTemplate> extract(juce::File const & dir, juce::CommandID & commandId);

//==============================================================================
struct SpeakerSetupTemplates {
    juce::Array<FileTemplate> dome{};
    juce::Array<FileTemplate> cube{};
    juce::Array<FileTemplate> domeSurround{};
};

SpeakerSetupTemplates getSpeakerSetupTemplates();

extern SpeakerSetupTemplates const SPEAKER_SETUP_TEMPLATES;

//==============================================================================

struct ProjectTemplates {
    juce::Array<FileTemplate> dome{};
    juce::Array<FileTemplate> cube{};
    juce::Array<FileTemplate> hybrid{};
};

extern ProjectTemplates const PROJECT_TEMPLATES;

ProjectTemplates getProjectTemplates();

} // namespace gris
