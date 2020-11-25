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

#include "macros.h"

DISABLE_WARNINGS
#include <JuceHeader.h>
ENABLE_WARNINGS

extern char const * const DEVICE_NAME;
extern char const * CLIENT_NAME;
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

extern const bool USE_OS_NATIVE_DIALOG_BOX;

extern const juce::StringArray BUFFER_SIZES;
extern const juce::StringArray RATE_VALUES;
extern const juce::StringArray FILE_FORMATS;
extern const juce::StringArray FILE_CONFIGS;
extern const juce::StringArray ATTENUATION_DB;
extern const juce::StringArray ATTENUATION_CUTOFFS;

static constexpr int VU_METER_WIDTH_IN_PIXELS = 22;
