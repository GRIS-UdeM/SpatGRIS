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

#include "sg_constants.hpp"

#include "BinaryData.h"

namespace gris
{
SpatGrisVersion const SPAT_GRIS_VERSION{ SpatGrisVersion::fromString(JUCE_STRINGIFY(JUCE_APP_VERSION)) };

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

juce::File const SPEAKER_TEMPLATES_DIR{ TEMPLATES_DIR.getChildFile("Speaker setups") };
juce::File const PROJECT_TEMPLATES_DIR{ TEMPLATES_DIR.getChildFile("Projects") };
juce::File const SPLASH_SCREEN_FILE{ RESOURCES_DIR.getChildFile("splash_screen.png") };
juce::File const DEFAULT_PROJECT_DIRECTORY{ RESOURCES_DIR.getChildFile("default_preset/") };
juce::File const DEFAULT_PROJECT_FILE{ DEFAULT_PROJECT_DIRECTORY.getChildFile("default_preset.xml") };
juce::File const DEFAULT_SPEAKER_SETUP_FILE{ DEFAULT_PROJECT_DIRECTORY.getChildFile("default_speaker_setup.xml") };
juce::File const BINAURAL_SPEAKER_SETUP_FILE{ DEFAULT_PROJECT_DIRECTORY.getChildFile("BINAURAL_SPEAKER_SETUP.xml") };
juce::File const STEREO_SPEAKER_SETUP_FILE{ DEFAULT_PROJECT_DIRECTORY.getChildFile("STEREO_SPEAKER_SETUP.xml") };
juce::File const MANUAL_FILE{ RESOURCES_DIR.getChildFile("SpatGRIS_3.1_Manual.pdf") };
juce::File const ICON_SMALL_FILE{ RESOURCES_DIR.getChildFile("ServerGRIS_icon_splash_small.png") };
juce::File const HRTF_FOLDER_0{ RESOURCES_DIR.getChildFile("hrtf_compact/elev" + juce::String(0) + "/") };
juce::File const HRTF_FOLDER_40{ RESOURCES_DIR.getChildFile("hrtf_compact/elev" + juce::String(40) + "/") };
juce::File const HRTF_FOLDER_80{ RESOURCES_DIR.getChildFile("hrtf_compact/elev" + juce::String(80) + "/") };

juce::Colour const DEFAULT_SOURCE_COLOR{ narrow<juce::uint8>(255), 0, 0 };

static constexpr auto SPEAKER_SETUP_TEMPLATES_COMMANDS_OFFSET = 2000;
static constexpr auto PROJECT_TEMPLATES_COMMANDS_OFFSET = 3000;

//==============================================================================
static juce::Array<FileTemplate> extract(juce::File const & dir, juce::CommandID & commandId)
{
    jassert(dir.isDirectory());
    juce::Array<FileTemplate> result{};

    for (auto const & file : dir.findChildFiles(juce::File::TypesOfFileToFind::findFiles, false)) {
        FileTemplate setup{ file.getFileNameWithoutExtension(), commandId++, file };
        result.add(setup);
    }

    result.sort();

    return result;
}

//==============================================================================
static SpeakerSetupTemplates getSpeakerSetupTemplates()
{
    auto const domeDir{ SPEAKER_TEMPLATES_DIR.getChildFile("DOME") };
    auto const cubeDir{ SPEAKER_TEMPLATES_DIR.getChildFile("CUBE") };
    auto commandId{ SPEAKER_SETUP_TEMPLATES_COMMANDS_OFFSET };

    return SpeakerSetupTemplates{ extract(domeDir, commandId), extract(cubeDir, commandId) };
}

//==============================================================================
static ProjectTemplates getProjectTemplates()
{
    auto commandId{ PROJECT_TEMPLATES_COMMANDS_OFFSET };

    return extract(PROJECT_TEMPLATES_DIR, commandId);
}

SpeakerSetupTemplates const SPEAKER_SETUP_TEMPLATES{ getSpeakerSetupTemplates() };
ProjectTemplates const PROJECT_TEMPLATES = getProjectTemplates();

//==============================================================================
juce::StringArray const RECORDING_FORMAT_STRINGS{ "WAV",
                                                  "AIFF"
#ifdef __APPLE__
                                                  ,
                                                  "CAF"
#endif
};
juce::StringArray const RECORDING_FILE_TYPE_STRINGS{ "Mono Files", "Interleaved" };
juce::Array<int> const ATTENUATION_DB_VALUES{ 0, -12, -24, -36, -48, -60, -72 };
juce::StringArray const ATTENUATION_DB_STRINGS{ "0", "-12", "-24", "-36", "-48", "-60", "-72" };
juce::Array<int> const ATTENUATION_FREQUENCY_VALUES{ 125, 250, 500, 1000, 2000, 4000, 8000, 16000 };
juce::StringArray const ATTENUATION_FREQUENCY_STRINGS{ "125", "250", "500", "1000", "2000", "4000", "8000", "16000 " };

//==============================================================================
template<typename T>
static juce::Array<T> stringToStronglyTypedFloat(juce::StringArray const & strings)
{
    static_assert(std::is_base_of_v<StrongFloatBase, T>);

    juce::Array<T> result{};
    result.ensureStorageAllocated(strings.size());
    for (auto const & string : strings) {
        result.add(T{ string.getFloatValue() });
    }
    return result;
}

//==============================================================================
static bool fancyStringCmp(juce::String const & a, juce::String const & b)
{
    static juce::StringArray const DEFAULT_FILES{ "Cube_default_speaker_setup",
                                                  "Dome_default_speaker_setup",
                                                  "default_project18(8X2-Subs2)" };

    static auto const IS_NUMERIC = [](char const c) { return c >= '0' && c <= '9'; };
    static auto const IS_NOT_NUMERIC = [](char const c) { return !IS_NUMERIC(c); };

    static auto const STARTS_WITH_A_NUMBER = [](std::string const & string) {
        if (string.empty()) {
            return false;
        }
        return IS_NUMERIC(string.front());
    };

    static auto const SPLIT = [](std::string const & string, auto const & predicate) {
        auto const end{ std::find_if_not(string.begin(), string.end(), predicate) };
        auto const length{ narrow<size_t>(end - string.begin()) };
        auto const extracted{ juce::String{ string.substr(0, length) } };
        auto const left{ juce::String{ string.substr(length) } };
        return std::make_pair(extracted, left);
    };

    auto const a_{ a.toStdString() };
    auto const b_{ b.toStdString() };

    if (a.isEmpty() && b.isEmpty()) {
        return false;
    }

    if (DEFAULT_FILES.contains(a)) {
        return true;
    }

    if (DEFAULT_FILES.contains(b)) {
        return false;
    }

    auto const aStartsWithNumber{ STARTS_WITH_A_NUMBER(a_) };
    auto const bStartsWithNumber{ STARTS_WITH_A_NUMBER(b_) };

    if (aStartsWithNumber != bStartsWithNumber) {
        return aStartsWithNumber;
    }

    if (aStartsWithNumber) {
        auto const [aNumber, aLeft] = SPLIT(a_, IS_NUMERIC);
        auto const [bNumber, bLeft] = SPLIT(b_, IS_NUMERIC);

        if (aNumber.getIntValue() == bNumber.getIntValue()) {
            return fancyStringCmp(aLeft, bLeft);
        }

        return aNumber.getIntValue() < bNumber.getIntValue();
    }

    auto const [aString, aLeft] = SPLIT(a_, IS_NOT_NUMERIC);
    auto const [bString, bLeft] = SPLIT(b_, IS_NOT_NUMERIC);

    if (aString == bString) {
        return fancyStringCmp(aLeft, bLeft);
    }

    return aString < bString;
}

//==============================================================================
bool FileTemplate::operator<(FileTemplate const & other) const noexcept
{
    return fancyStringCmp(name, other.name);
}

//==============================================================================
tl::optional<FileTemplate const &> commandIdToTemplate(juce::CommandID commandId)
{
    auto const find = [&](juce::Array<FileTemplate> const & templates) -> tl::optional<FileTemplate const &> {
        auto const * it{ std::find_if(templates.begin(), templates.end(), [&](FileTemplate const & templateInfo) {
            return templateInfo.commandId == commandId;
        }) };
        if (it == templates.end()) {
            return tl::nullopt;
        }
        return *it;
    };

    auto const domeTemplate{ find(SPEAKER_SETUP_TEMPLATES.dome) };

    if (domeTemplate) {
        return domeTemplate;
    }

    auto const cubeTemplate{ find(SPEAKER_SETUP_TEMPLATES.cube) };
    if (cubeTemplate) {
        return cubeTemplate;
    }

    auto const projectTemplates{ find(PROJECT_TEMPLATES) };
    if (projectTemplates) {
        return projectTemplates;
    }

    return tl::nullopt;
}

//==============================================================================
tl::optional<int> attenuationDbToComboBoxIndex(dbfs_t const attenuation)
{
    static auto const ALLOWED_VALUES = stringToStronglyTypedFloat<dbfs_t>(ATTENUATION_DB_STRINGS);

    auto const index{ ALLOWED_VALUES.indexOf(attenuation) };
    if (index < 0) {
        return tl::nullopt;
    }
    return index + 1;
}

//==============================================================================
tl::optional<int> attenuationFreqToComboBoxIndex(hz_t const freq)
{
    static auto const ALLOWED_VALUES = stringToStronglyTypedFloat<hz_t>(ATTENUATION_FREQUENCY_STRINGS);

    auto const index{ ALLOWED_VALUES.indexOf(freq) };
    if (index < 0) {
        return tl::nullopt;
    }
    return index + 1;
}

} // namespace gris
