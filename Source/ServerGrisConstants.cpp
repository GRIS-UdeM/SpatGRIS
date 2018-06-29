#include "ServerGrisConstants.h"

const char *DeviceName = "GRIS";
const char *ClientName = "ServerGRIS";

#ifdef __linux__
const char *DriverNameSys = "alsa";
const String SplashScreenFilePath = File::getCurrentWorkingDirectory().getFullPathName() +
                                    ("/../../Resources/splash_screen.png");
const String DefaultPresetFilePath = File::getCurrentWorkingDirectory().getFullPathName() +
                                     ("/../../Resources/default_preset/default_preset.xml");
const String DefaultPresetDirectoryPath = File::getCurrentWorkingDirectory().getFullPathName() +
                                          ("/../../Resources/default_preset/");
const String DefaultSpeakerSetupFilePath = File::getCurrentWorkingDirectory().getFullPathName() +
                                           ("/../../Resources/default_preset/default_speaker_setup.xml");
const String BinauralSpeakerSetupFilePath = File::getCurrentWorkingDirectory().getFullPathName() +
                                            ("/../../Resources/default_preset/BINAURAL_SPEAKER_SETUP.xml");
const String ServerGrisManualFilePath = File::getCurrentWorkingDirectory().getFullPathName() +
                                        ("/../../Resources/ServerGRIS_1.0_Manual.pdf");
const bool UseOSNativeDialogBox = false;
#else
const char *DriverNameSys = "coreaudio";
const String SplashScreenFilePath = File::getSpecialLocation(File::currentApplicationFile).getFullPathName() +
                                    ("/Contents/Resources/splash_screen.png");
const String DefaultPresetFilePath = File::getSpecialLocation(File::currentApplicationFile).getFullPathName() +
                                     ("/Contents/Resources/default_preset/default_preset.xml");
const String DefaultPresetDirectoryPath = File::getSpecialLocation(File::currentApplicationFile).getFullPathName() +
                                          ("/Contents/Resources/default_preset/");
const String DefaultSpeakerSetupFilePath = File::getSpecialLocation(File::currentApplicationFile).getFullPathName() +
                                           ("/Contents/Resources/default_preset/default_speaker_setup.xml");
const String BinauralSpeakerSetupFilePath = File::getSpecialLocation(File::currentApplicationFile).getFullPathName() +
                                            ("/Contents/Resources/default_preset/BINAURAL_SPEAKER_SETUP.xml");
const String ServerGrisManualFilePath = File::getSpecialLocation(File::currentApplicationFile).getFullPathName() +
                                        ("/Contents/Resources/ServerGRIS_1.0_Manual.pdf");
const bool UseOSNativeDialogBox = true;
#endif

const char *ClientNameSys = "system";

const char *ClientNameIgnor = "JAR::57";

const StringArray ModeSpatString = {"VBAP",  "LBAP", "BINAURAL", "STEREO"};
