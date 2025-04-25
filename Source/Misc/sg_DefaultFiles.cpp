#include "sg_DefaultFiles.hpp"

namespace gris
{

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

bool FileTemplate::operator<(FileTemplate const& other) const noexcept
{
    return name.compareNatural (other.name) >= 0 ? false : true;
}

SpeakerSetupTemplates const SPEAKER_SETUP_TEMPLATES { getSpeakerSetupTemplates () };
ProjectTemplates const PROJECT_TEMPLATES { getProjectTemplates () };

} // namespace gris
