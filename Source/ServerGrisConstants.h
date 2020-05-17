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

extern const char *DeviceName;
extern const char *ClientName;
extern const char *DriverNameSys;
extern const char *ClientNameSys;
extern const char *ClientNameIgnor;

extern const juce::String SplashScreenFilePath;
extern const juce::String DefaultPresetFilePath;
extern const juce::String DefaultPresetDirectoryPath;
extern const juce::String DefaultSpeakerSetupFilePath;
extern const juce::String BinauralSpeakerSetupFilePath;
extern const juce::String StereoSpeakerSetupFilePath;
extern const juce::String ServerGrisManualFilePath;
extern const juce::String ServerGrisIconSmallFilePath;

extern const juce::String HRTFFolder0Path;
extern const juce::String HRTFFolder40Path;
extern const juce::String HRTFFolder80Path;

extern const juce::StringArray ModeSpatString;

extern const bool UseOSNativeDialogBox;

extern const juce::StringArray BufferSizes;
extern const juce::StringArray RateValues;
extern const juce::StringArray FileFormats;
extern const juce::StringArray FileConfigs;
extern const juce::StringArray AttenuationDBs;
extern const juce::StringArray AttenuationCutoffs;

extern const unsigned int VuMeterWidthInPixels;

#endif /* SERVERGRISCONSTANTS_H */