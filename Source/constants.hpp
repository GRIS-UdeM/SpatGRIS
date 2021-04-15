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
constexpr auto LBAP_EXTENDED_RADIUS = 1.6666667f;
constexpr auto HRTF_NUM_SAMPLES = 128;
constexpr dbfs_t MIN_PINK_NOISE_DB{ -60.0f };
constexpr dbfs_t MAX_PINK_NOISE_DB{ 0.0f };
constexpr dbfs_t DEFAULT_PINK_NOISE_DB{ -20.0f };

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
extern juce::StringArray const MODE_SPAT_STRING;

extern juce::StringArray const RECORDING_FORMAT_STRINGS;
extern juce::StringArray const RECORDING_CONFIG_STRINGS;
extern juce::StringArray const ATTENUATION_DB_STRINGS;
extern juce::StringArray const ATTENUATION_FREQUENCY_STRINGS;

[[nodiscard]] tl::optional<int> attenuationDbToComboBoxIndex(dbfs_t attenuation);
[[nodiscard]] tl::optional<int> attenuationFreqToComboBoxIndex(hz_t const freq);