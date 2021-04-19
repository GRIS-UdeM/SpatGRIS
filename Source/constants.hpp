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

#include "StrongTypes.hpp"
#include "lib/tl/optional.hpp"

constexpr auto MAX_INPUTS = 256;
constexpr auto MAX_OUTPUTS = 256;
constexpr auto VU_METER_WIDTH_IN_PIXELS = 22;
constexpr auto VU_METER_HEIGHT = 140;
constexpr auto LBAP_EXTENDED_RADIUS = 1.6666667f;
constexpr auto HRTF_NUM_SAMPLES = 128;
constexpr dbfs_t DEFAULT_PINK_NOISE_DB{ -20.0f };

constexpr juce::Range<source_index_t> LEGAL_SOURCE_INDEX_RANGE{ source_index_t{ 1 }, source_index_t{ MAX_INPUTS } };
constexpr juce::Range<output_patch_t> LEGAL_OUTPUT_PATCH_RANGE{ output_patch_t{ 1 }, output_patch_t{ MAX_OUTPUTS } };
constexpr juce::Range<dbfs_t> LEGAL_MASTER_GAIN_RANGE{ dbfs_t{ -60.0f }, dbfs_t{ 12.0f } };
constexpr juce::Range<float> LEGAL_GAIN_INTERPOLATION_RANGE{ 0.0f, 1.0f };
constexpr juce::Range<dbfs_t> LEGAL_PINK_NOISE_GAIN_RANGE{ dbfs_t{ -60.0f }, dbfs_t{ 0.0f } };

extern char const * const DEVICE_NAME;
extern char const * const CLIENT_NAME;
extern char const * const SYS_DRIVER_NAME;
extern char const * const SYS_CLIENT_NAME;
extern char const * const CLIENT_NAME_IGNORE;

extern juce::File const SPLASH_SCREEN_FILE;
extern juce::File const DEFAULT_PROJECT_FILE;
extern juce::File const DEFAULT_PRESET_DIRECTORY;
extern juce::File const DEFAULT_SPEAKER_SETUP_FILE;
extern juce::File const BINAURAL_SPEAKER_SETUP_FILE;
extern juce::File const STEREO_SPEAKER_SETUP_FILE;
extern juce::File const SERVER_GRIS_MANUAL_FILE;
extern juce::File const SERVER_GRIS_ICON_SMALL_FILE;
extern juce::File const HRTF_FOLDER_0;
extern juce::File const HRTF_FOLDER_40;
extern juce::File const HRTF_FOLDER_80;

//==============================================================================
extern juce::StringArray const RECORDING_FORMAT_STRINGS;
extern juce::StringArray const RECORDING_CONFIG_STRINGS;
extern juce::StringArray const ATTENUATION_DB_STRINGS;
extern juce::StringArray const ATTENUATION_FREQUENCY_STRINGS;

//==============================================================================
[[nodiscard]] tl::optional<int> attenuationDbToComboBoxIndex(dbfs_t attenuation);
[[nodiscard]] tl::optional<int> attenuationFreqToComboBoxIndex(hz_t const freq);