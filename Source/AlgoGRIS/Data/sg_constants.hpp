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

#pragma once

#include "../tl/optional.hpp"
#include "StrongTypes/sg_Dbfs.hpp"
#include "StrongTypes/sg_Hz.hpp"
#include "StrongTypes/sg_OutputPatch.hpp"
#include "StrongTypes/sg_SourceIndex.hpp"
#include <JuceHeader.h>

namespace gris
{
//==============================================================================
struct SpatGrisVersion {
    int major;
    int minor;
    int patch;
    //==============================================================================
    [[nodiscard]] constexpr int compare(SpatGrisVersion const & other) const noexcept
    {
        auto const majorDiff{ major - other.major };
        if (majorDiff != 0) {
            return majorDiff;
        }
        auto const minorDiff{ minor - other.minor };
        if (minorDiff != 0) {
            return minorDiff;
        }
        return patch - other.patch;
    }
    //==============================================================================
    [[nodiscard]] juce::String toString() const noexcept
    {
        return juce::String{ major } + '.' + juce::String{ minor } + '.' + juce::String{ patch };
    }
    //==============================================================================
    static SpatGrisVersion fromString(juce::String const & string)
    {
        SpatGrisVersion result{};
        result.major = string.upToFirstOccurrenceOf(".", false, true).getIntValue();
        result.minor
            = string.fromFirstOccurrenceOf(".", false, true).upToLastOccurrenceOf(".", false, true).getIntValue();
        result.patch = string.fromLastOccurrenceOf(".", false, true).getIntValue();
        return result;
    }
};

extern const SpatGrisVersion SPAT_GRIS_VERSION;

constexpr auto MAX_NUM_SOURCES = 256;
constexpr auto MAX_NUM_SPEAKERS = 256;
constexpr auto MBAP_EXTENDED_RADIUS = 1.6666667f;
constexpr dbfs_t DEFAULT_PINK_NOISE_DB{ -20.0f };
constexpr auto DEFAULT_SAMPLE_RATE{ 48000.0 };
constexpr auto DEFAULT_BUFFER_SIZE{ 512 };
constexpr auto SQRT3{ 1.7320508075688772935274463415059f };

constexpr auto SPAT_MODE_BUTTONS_RADIO_GROUP_ID = 1;
constexpr auto PREPARE_TO_RECORD_WINDOW_FILE_FORMAT_GROUP_ID = 2;
constexpr auto PREPARE_TO_RECORD_WINDOW_FILE_TYPE_GROUP_ID = 3;

constexpr juce::Range<source_index_t> LEGAL_SOURCE_INDEX_RANGE{ source_index_t{ 1 },
                                                                source_index_t{ MAX_NUM_SOURCES + 1 } };
constexpr juce::Range<output_patch_t> LEGAL_OUTPUT_PATCH_RANGE{ output_patch_t{ 1 },
                                                                output_patch_t{ MAX_NUM_SPEAKERS + 1 } };
constexpr juce::Range<dbfs_t> LEGAL_MASTER_GAIN_RANGE{ dbfs_t{ -60.0f }, dbfs_t{ 12.0f } };
constexpr juce::Range<float> LEGAL_GAIN_INTERPOLATION_RANGE{ 0.0f, 1.0f };
constexpr juce::Range<dbfs_t> LEGAL_PINK_NOISE_GAIN_RANGE{ dbfs_t{ -60.0f }, dbfs_t{ 0.0f } };

constexpr auto SLICES_WIDTH = 25;
constexpr auto SLICES_ID_BUTTON_HEIGHT = 17;

extern juce::File const SPEAKER_TEMPLATES_DIR;
extern juce::File const PROJECT_TEMPLATES_DIR;
extern juce::File const CURRENT_WORKING_DIR;
extern juce::File const SPLASH_SCREEN_FILE;
extern juce::File const DEFAULT_PROJECT_FILE;
extern juce::File const DEFAULT_PROJECT_DIRECTORY;
extern juce::File const DEFAULT_SPEAKER_SETUP_FILE;
extern juce::File const BINAURAL_SPEAKER_SETUP_FILE;
extern juce::File const STEREO_SPEAKER_SETUP_FILE;
extern juce::File const MANUAL_FILE_EN;
extern juce::File const MANUAL_FILE_FR;
extern juce::File const ICON_SMALL_FILE;
extern juce::File const HRTF_FOLDER_0;
extern juce::File const HRTF_FOLDER_40;
extern juce::File const HRTF_FOLDER_80;

extern juce::Colour const DEFAULT_SOURCE_COLOR;

//==============================================================================
struct FileTemplate {
    juce::String name{};
    juce::CommandID commandId{};
    juce::File path{};

    bool operator<(FileTemplate const & other) const noexcept;
};

//==============================================================================
struct SpeakerSetupTemplates {
    juce::Array<FileTemplate> dome{};
    juce::Array<FileTemplate> cube{};
};

struct ProjectTemplates {
    juce::Array<FileTemplate> dome{};
    juce::Array<FileTemplate> cube{};
    juce::Array<FileTemplate> hybrid{};
};

extern SpeakerSetupTemplates const SPEAKER_SETUP_TEMPLATES;
extern ProjectTemplates const PROJECT_TEMPLATES;
tl::optional<FileTemplate const &> commandIdToTemplate(juce::CommandID commandId);

//==============================================================================
extern juce::StringArray const RECORDING_FORMAT_STRINGS;
extern juce::StringArray const RECORDING_FILE_TYPE_STRINGS;
extern juce::StringArray const ATTENUATION_DB_STRINGS;
extern juce::StringArray const ATTENUATION_FREQUENCY_STRINGS;

extern juce::Array<int> const ATTENUATION_DB_VALUES;
extern juce::Array<int> const ATTENUATION_FREQUENCY_VALUES;

//==============================================================================
[[nodiscard]] tl::optional<int> attenuationDbToComboBoxIndex(dbfs_t attenuation);
[[nodiscard]] tl::optional<int> attenuationFreqToComboBoxIndex(hz_t freq);

} // namespace gris
