/*
 This file is part of SpatGRIS2.

 Developers: Samuel B�land, Olivier B�langer, Nicolas Masson

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

#include "ServerGrisConstants.h"

char const * const DEVICE_NAME = "GRIS";
char const * CLIENT_NAME = "SpatGRIS2";
char const * const SYS_CLIENT_NAME = "system";
char const * const CLIENT_NAME_IGNORE = "JAR::57";

#ifdef __linux__
char const * const SYS_DRIVER_NAME = "alsa";
const bool USE_OS_NATIVE_DIALOG_BOX = false;
auto const CURRENT_WORKING_DIR{ juce::File::getCurrentWorkingDirectory() };
auto const RESOURCES_DIR{ CURRENT_WORKING_DIR.getChildFile("Resources") };
#elif defined WIN32
char const * const SYS_DRIVER_NAME = "coreaudio";
const bool USE_OS_NATIVE_DIALOG_BOX{ true };
auto const CURRENT_WORKING_DIR{ juce::File::getCurrentWorkingDirectory() };
auto const RESOURCES_DIR{ CURRENT_WORKING_DIR.getChildFile("Resources") };
#elif defined __APPLE__
const char * const SYS_DRIVER_NAME = "coreaudio";
const bool USE_OS_NATIVE_DIALOG_BOX = true;
juce::File CURRENT_WORKING_DIR = juce::File::getSpecialLocation(juce::File::currentApplicationFile);
juce::File RESOURCES_DIR{ CURRENT_WORKING_DIR.getChildFile("Contents").getChildFile("Resources") };
#else
static_assert(false, "What are you building this on?");
#endif

juce::File const SPLASH_SCREEN_FILE{ RESOURCES_DIR.getChildFile("splash_screen.png") };
juce::File const DEFAULT_PRESET_DIRECTORY{ RESOURCES_DIR.getChildFile("default_preset/") };
juce::File const DEFAULT_PRESET_FILE{ DEFAULT_PRESET_DIRECTORY.getChildFile("default_preset.xml") };
juce::File const DEFAULT_SPEAKER_SETUP_FILE{ DEFAULT_PRESET_DIRECTORY.getChildFile("default_speaker_setup.xml") };
juce::File const BINAURAL_SPEAKER_SETUP_FILE{ DEFAULT_PRESET_DIRECTORY.getChildFile("BINAURAL_SPEAKER_SETUP.xml") };
juce::File const STEREO_SPEAKER_SETUP_FILE{ DEFAULT_PRESET_DIRECTORY.getChildFile("STEREO_SPEAKER_SETUP.xml") };
juce::File const SERVER_GRIS_MANUAL_FILE{ RESOURCES_DIR.getChildFile("SpatGRIS2_2.0_Manual.pdf") };
juce::File const SERVER_GRIS_ICON_SMALL_FILE{ RESOURCES_DIR.getChildFile("ServerGRIS_icon_small.png") };
juce::File const HRTF_FOLDER_0{ RESOURCES_DIR.getChildFile("hrtf_compact/elev" + juce::String(0) + "/") };
juce::File const HRTF_FOLDER_40{ RESOURCES_DIR.getChildFile("hrtf_compact/elev" + juce::String(40) + "/") };
juce::File const HRTF_FOLDER_80{ RESOURCES_DIR.getChildFile("hrtf_compact/elev" + juce::String(80) + "/") };

const juce::StringArray MODE_SPAT_STRING = { "DOME", "CUBE", "BINAURAL", "STEREO" };

// Settings Jack Server
const juce::StringArray BUFFER_SIZES = { "32", "64", "128", "256", "512", "1024", "2048" };
const juce::StringArray RATE_VALUES = { "44100", "48000", "88200", "96000" };
const juce::StringArray FILE_FORMATS = { "WAV", "AIFF" };
const juce::StringArray FILE_CONFIGS = { "Multiple Mono Files", "Single Interleaved" };
const juce::StringArray ATTENUATION_DB = { "0", "-12", "-24", "-36", "-48", "-60", "-72" };
const juce::StringArray ATTENUATION_CUTOFFS = { "125", "250", "500", "1000", "2000", "4000", "8000", "16000" };
