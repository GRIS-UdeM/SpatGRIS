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

#ifndef SERVERGRISCONSTANTS_H
#define SERVERGRISCONSTANTS_H

#include "../JuceLibraryCode/JuceHeader.h"

extern const char * DeviceName;
extern const char * ClientName;
extern const char * driverNameSys;
extern const char * ClientNameSys;
extern const char * ClientNameIgnor;

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

extern const juce::StringArray ModeSpatString;

extern const bool USE_OS_NATIVE_DIALOG_BOX;

extern const juce::StringArray BufferSizes;
extern const juce::StringArray RateValues;
extern const juce::StringArray FileFormats;
extern const juce::StringArray FileConfigs;
extern const juce::StringArray AttenuationDBs;
extern const juce::StringArray AttenuationCutoffs;

extern const unsigned int VuMeterWidthInPixels;

#endif /* SERVERGRISCONSTANTS_H */