#pragma once

#include <JuceHeader.h>

#include "../../submodules/AlgoGRIS/tl/optional.hpp"

namespace gris
{

#if defined(__linux__) || defined(WIN32)
juce::File const CURRENT_WORKING_DIR { juce::File::getCurrentWorkingDirectory () };
auto const RESOURCES_DIR { CURRENT_WORKING_DIR.getChildFile ("Resources") };
#elif defined(__APPLE__)
juce::File const CURRENT_WORKING_DIR = juce::File::getSpecialLocation (juce::File::currentApplicationFile);
auto const RESOURCES_DIR { CURRENT_WORKING_DIR.getChildFile ("Contents").getChildFile ("Resources") };
#else
static_assert(false, "What are you building this on?");
#endif

#ifdef __APPLE__
static auto const TEMPLATES_DIR { RESOURCES_DIR };
#else
static auto const TEMPLATES_DIR { RESOURCES_DIR.getChildFile ("templates") };
#endif

inline juce::File const SPEAKER_TEMPLATES_DIR{ TEMPLATES_DIR.getChildFile("Speaker setups") };
inline juce::File const PROJECT_TEMPLATES_DIR{ TEMPLATES_DIR.getChildFile("Projects") };
inline juce::File const SPLASH_SCREEN_FILE{ RESOURCES_DIR.getChildFile("splash_screen.png") };
inline juce::File const DEFAULT_PROJECT_DIRECTORY{ RESOURCES_DIR.getChildFile("default_preset/") };
inline juce::File const DEFAULT_PROJECT_FILE{ DEFAULT_PROJECT_DIRECTORY.getChildFile("default_preset.xml") };
inline juce::File const DEFAULT_SPEAKER_SETUP_FILE{ DEFAULT_PROJECT_DIRECTORY.getChildFile("default_speaker_setup.xml") };
inline juce::File const BINAURAL_SPEAKER_SETUP_FILE{ DEFAULT_PROJECT_DIRECTORY.getChildFile("BINAURAL_SPEAKER_SETUP.xml") };
inline juce::File const STEREO_SPEAKER_SETUP_FILE{ DEFAULT_PROJECT_DIRECTORY.getChildFile("STEREO_SPEAKER_SETUP.xml") };
inline juce::File const DEFAULT_CUBE_PROJECT{ RESOURCES_DIR.getChildFile("templates/Projects/CUBE/default_project18(8X2-Subs2).xml") };
inline juce::File const DEFAULT_CUBE_SPEAKER_SETUP{ RESOURCES_DIR.getChildFile("templates/Speaker setups/CUBE/Cube_default_speaker_setup.xml") };
inline juce::File const MANUAL_FILE_EN{ RESOURCES_DIR.getChildFile("SpatGRIS_3.3.7_Manual_EN.pdf") };
inline juce::File const MANUAL_FILE_FR{ RESOURCES_DIR.getChildFile("SpatGRIS_3.3.7_Manuel_FR.pdf") };
inline juce::File const ICON_SMALL_FILE{ RESOURCES_DIR.getChildFile("ServerGRIS_icon_splash_small.png") };
inline juce::File const HRTF_FOLDER_0{ RESOURCES_DIR.getChildFile("hrtf_compact/elev" + juce::String(0) + "/") };
inline juce::File const HRTF_FOLDER_40{ RESOURCES_DIR.getChildFile("hrtf_compact/elev" + juce::String(40) + "/") };
inline juce::File const HRTF_FOLDER_80{ RESOURCES_DIR.getChildFile("hrtf_compact/elev" + juce::String(80) + "/") };

//==============================================================================
struct FileTemplate
{
    juce::String name {};
    juce::CommandID commandId {};
    juce::File path {};

    bool operator<(FileTemplate const& other) const noexcept;
};

tl::optional<FileTemplate const&> commandIdToTemplate (juce::CommandID commandId);

juce::Array<FileTemplate> extract (juce::File const& dir, juce::CommandID& commandId);

//==============================================================================
struct SpeakerSetupTemplates
{
    juce::Array<FileTemplate> dome {};
    juce::Array<FileTemplate> cube {};
};

SpeakerSetupTemplates getSpeakerSetupTemplates ();

extern SpeakerSetupTemplates const SPEAKER_SETUP_TEMPLATES;

//==============================================================================

struct ProjectTemplates
{
    juce::Array<FileTemplate> dome {};
    juce::Array<FileTemplate> cube {};
    juce::Array<FileTemplate> hybrid {};
};

extern ProjectTemplates const PROJECT_TEMPLATES;

ProjectTemplates getProjectTemplates ();

} // namespace gris
