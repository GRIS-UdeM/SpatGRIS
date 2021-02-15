/*
 This file is part of SpatGRIS2.

 Developers: Samuel Béland, Olivier Bélanger, Nicolas Masson

 SpatGRIS2 is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 SpatGRIS2 is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with SpatGRIS2.  If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

#include <optional>

#include "macros.h"
DISABLE_WARNINGS
#include <JuceHeader.h>
ENABLE_WARNINGS

static constexpr auto MAX_INPUTS = 256;
static constexpr auto MAX_OUTPUTS = 256;
static constexpr auto VU_METER_WIDTH_IN_PIXELS = 22;

extern char const * const DEVICE_NAME;
extern char const * const CLIENT_NAME;
extern char const * const SYS_DRIVER_NAME;
extern char const * const SYS_CLIENT_NAME;
extern char const * const CLIENT_NAME_IGNORE;

extern const juce::File SPLASH_SCREEN_FILE;
extern const juce::File DEFAULT_PRESET_FILE;
extern const juce::File DEFAULT_PRESET_DIRECTORY;
extern const juce::File DEFAULT_SPEAKER_SETUP_FILE;
extern const juce::File BINAURAL_SPEAKER_SETUP_FILE;
extern const juce::File STEREO_SPEAKER_SETUP_FILE;
extern const juce::File SERVER_GRIS_MANUAL_FILE;
extern const juce::File SERVER_GRIS_ICON_SMALL_FILE;

extern const juce::File HRTF_FOLDER_0;
extern const juce::File HRTF_FOLDER_40;
extern const juce::File HRTF_FOLDER_80;

extern const juce::StringArray MODE_SPAT_STRING;

extern const juce::StringArray FILE_FORMATS;
extern const juce::StringArray FILE_CONFIGS;
extern const juce::StringArray ATTENUATION_DB;
extern const juce::StringArray ATTENUATION_CUTOFFS;

namespace user_properties_tags
{
extern const juce::String DEVICE_TYPE;
extern const juce::String INPUT_DEVICE;
extern const juce::String OUTPUT_DEVICE;
extern const juce::String BUFFER_SIZE;
extern const juce::String SAMPLE_RATE;
extern const juce::String OSC_INPUT_PORT;
extern const juce::String FILE_FORMAT;
extern const juce::String FILE_CONFIG;
extern const juce::String ATTENUATION_DB;
extern const juce::String ATTENUATION_HZ;

extern const juce::String LAST_VBAP_SPEAKER_SETUP;
extern const juce::String LAST_OPEN_PRESET;
extern const juce::String LAST_OPEN_SPEAKER_SETUP;
extern const juce::String LAST_RECORDING_DIRECTORY;
extern const juce::String SASH_POSITION;
} // namespace user_properties_tags

enum class RecordingFormat { wav, aiff };
enum class RecordingConfig { interleaved, mono };

juce::String recordingFormatToString(RecordingFormat format);
std::optional<RecordingFormat> stringToRecordingFormat(juce::String const & string);
juce::String recordingConfigToString(RecordingConfig config);
std::optional<RecordingConfig> stringToRecordingConfig(juce::String const & string);
