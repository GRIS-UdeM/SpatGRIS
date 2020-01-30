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

extern const String SplashScreenFilePath;
extern const String DefaultPresetFilePath;
extern const String DefaultPresetDirectoryPath;
extern const String DefaultSpeakerSetupFilePath;
extern const String BinauralSpeakerSetupFilePath;
extern const String StereoSpeakerSetupFilePath;
extern const String ServerGrisManualFilePath;
extern const String ServerGrisIconSmallFilePath;

extern const String HRTFFolder0Path;
extern const String HRTFFolder40Path;
extern const String HRTFFolder80Path;

extern const StringArray ModeSpatString;

extern const bool UseOSNativeDialogBox;

extern const StringArray BufferSizes;
extern const StringArray RateValues;
extern const StringArray FileFormats;
extern const StringArray FileConfigs;
extern const StringArray AttenuationDBs;
extern const StringArray AttenuationCutoffs;

#endif /* SERVERGRISCONSTANTS_H */