#include "sg_DefaultFiles.hpp"

namespace gris
{

bool FileTemplate::operator<(FileTemplate const& other) const noexcept
{
    return name.compareNatural (other.name) >= 0 ? false : true;
}

tl::optional<FileTemplate const&> commandIdToTemplate (juce::CommandID commandId)
{
    auto const find = [&](juce::Array<FileTemplate> const& templates) -> tl::optional<FileTemplate const&> {
        auto const* it { std::find_if (templates.begin (), templates.end (), [&](FileTemplate const& templateInfo) {
            return templateInfo.commandId == commandId;
        }) };
        if (it == templates.end ()) {
            return tl::nullopt;
        }
        return *it;
        };

    auto const domeTemplate { find (SPEAKER_SETUP_TEMPLATES.dome) };
    if (domeTemplate) {
        return domeTemplate;
    }

    auto const cubeTemplate { find (SPEAKER_SETUP_TEMPLATES.cube) };
    if (cubeTemplate) {
        return cubeTemplate;
    }

    auto const domeProjectTemplate { find (PROJECT_TEMPLATES.dome) };
    if (domeProjectTemplate) {
        return domeProjectTemplate;
    }

    auto const cubeProjectTemplate { find (PROJECT_TEMPLATES.cube) };
    if (cubeProjectTemplate) {
        return cubeProjectTemplate;
    }

    auto const hybridProjectTemplate { find (PROJECT_TEMPLATES.hybrid) };
    if (hybridProjectTemplate) {
        return hybridProjectTemplate;
    }

    return tl::nullopt;
}

juce::Array<FileTemplate> extract (juce::File const& dir, juce::CommandID& commandId)
{
    jassert (dir.isDirectory ());
    juce::Array<FileTemplate> result {};

    for (auto const& file : dir.findChildFiles (juce::File::TypesOfFileToFind::findFiles, false)) {
        FileTemplate setup { file.getFileNameWithoutExtension (), commandId++, file };
        result.add (setup);
    }

    result.sort ();

    return result;
}

//==============================================================================

static constexpr auto SPEAKER_SETUP_TEMPLATES_COMMANDS_OFFSET = 2000;

SpeakerSetupTemplates getSpeakerSetupTemplates ()
{
    auto const domeDir { SPEAKER_TEMPLATES_DIR.getChildFile ("DOME") };
    auto const cubeDir { SPEAKER_TEMPLATES_DIR.getChildFile ("CUBE") };
    auto commandId { SPEAKER_SETUP_TEMPLATES_COMMANDS_OFFSET };

    return SpeakerSetupTemplates { extract (domeDir, commandId), extract (cubeDir, commandId) };
}

SpeakerSetupTemplates const SPEAKER_SETUP_TEMPLATES { getSpeakerSetupTemplates () };

//==============================================================================

static constexpr auto PROJECT_TEMPLATES_COMMANDS_OFFSET = 3000;

ProjectTemplates getProjectTemplates ()
{
    auto const domeDir { PROJECT_TEMPLATES_DIR.getChildFile ("DOME") };
    auto const cubeDir { PROJECT_TEMPLATES_DIR.getChildFile ("CUBE") };
    auto const hybridDir { PROJECT_TEMPLATES_DIR.getChildFile ("HYBRID") };
    auto commandId { PROJECT_TEMPLATES_COMMANDS_OFFSET };

    return ProjectTemplates { extract (domeDir, commandId), extract (cubeDir, commandId), extract (hybridDir, commandId) };
}

ProjectTemplates const PROJECT_TEMPLATES { getProjectTemplates () };

} // namespace gris
