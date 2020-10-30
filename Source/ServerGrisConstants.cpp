/*
 This file is part of SpatGRIS2.

 Developers: Olivier Belanger

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

const char * DeviceName = "GRIS";
const char * ClientName = "SpatGRIS2";
const char * ClientNameSys = "system";
const char * ClientNameIgnor = "JAR::57";

#ifdef __linux__
const char * DriverNameSys = "alsa";
const bool UseOSNativeDialogBox = false;
juce::String CURRENT_WORKING_DIR = juce::File::getCurrentWorkingDirectory().getFullPathName();
juce::String RESOURCES_DIR = juce::String("/../../Resources/");
#elif defined WIN32
const char * driverNameSys = "coreaudio";
const bool USE_OS_NATIVE_DIALOG_BOX{ true };
auto const CURRENT_WORKING_DIR{ juce::File::getCurrentWorkingDirectory() };
auto const RESOURCES_DIR{ CURRENT_WORKING_DIR.getChildFile("Resources") };
#elif defined __APPLE__
const char * driverNameSys = "coreaudio";
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

const juce::StringArray ModeSpatString = { "DOME", "CUBE", "BINAURAL", "STEREO" };

// Settings Jack Server
const juce::StringArray BufferSizes = { "32", "64", "128", "256", "512", "1024", "2048" };
const juce::StringArray RateValues = { "44100", "48000", "88200", "96000" };
const juce::StringArray FileFormats = { "WAV", "AIFF" };
const juce::StringArray FileConfigs = { "Multiple Mono Files", "Single Interleaved" };
const juce::StringArray AttenuationDBs = { "0", "-12", "-24", "-36", "-48", "-60", "-72" };
const juce::StringArray AttenuationCutoffs = { "125", "250", "500", "1000", "2000", "4000", "8000", "16000" };

const unsigned int VuMeterWidthInPixels = 22;
