#include "ServerGrisConstants.h"

const char *DeviceName = "GRIS";
const char *ClientName = "ServerGRIS";
const char *ClientNameSys = "system";
const char *ClientNameIgnor = "JAR::57";

#ifdef __linux__
const char *DriverNameSys = "alsa";
const bool UseOSNativeDialogBox = false;
String CURRENT_WORKING_DIR = File::getCurrentWorkingDirectory().getFullPathName();
String RESOURCES_DIR = String("/../../Resources/");
#else
const char *DriverNameSys = "coreaudio";
const bool UseOSNativeDialogBox = true;
String CURRENT_WORKING_DIR = File::getSpecialLocation(File::currentApplicationFile).getFullPathName();
String RESOURCES_DIR = String("/Contents/Resources/");
#endif

const String SplashScreenFilePath = CURRENT_WORKING_DIR + RESOURCES_DIR + "splash_screen.png";
const String DefaultPresetFilePath = CURRENT_WORKING_DIR + RESOURCES_DIR + "default_preset/default_preset.xml";
const String DefaultPresetDirectoryPath = CURRENT_WORKING_DIR + RESOURCES_DIR + "default_preset/";
const String DefaultSpeakerSetupFilePath = CURRENT_WORKING_DIR + RESOURCES_DIR + "default_preset/default_speaker_setup.xml";
const String BinauralSpeakerSetupFilePath = CURRENT_WORKING_DIR + RESOURCES_DIR + "default_preset/BINAURAL_SPEAKER_SETUP.xml";
const String StereoSpeakerSetupFilePath = CURRENT_WORKING_DIR + RESOURCES_DIR + "default_preset/STEREO_SPEAKER_SETUP.xml";
const String ServerGrisManualFilePath = CURRENT_WORKING_DIR + RESOURCES_DIR + "ServerGRIS_1.0_Manual.pdf";
const String ServerGrisIconSmallFilePath = CURRENT_WORKING_DIR + RESOURCES_DIR + "ServerGRIS_icon_small.png";
const String HRTFFolder0Path = CURRENT_WORKING_DIR + RESOURCES_DIR + "hrtf_compact/elev" + String(0) + "/";
const String HRTFFolder40Path = CURRENT_WORKING_DIR + RESOURCES_DIR + "hrtf_compact/elev" + String(40) + "/";
const String HRTFFolder80Path = CURRENT_WORKING_DIR + RESOURCES_DIR + "hrtf_compact/elev" + String(80) + "/";

const StringArray ModeSpatString = {"DOME",  "CUBE", "BINAURAL", "STEREO"};

// Settings Jack Server
const StringArray BufferSizes = {"32", "64", "128", "256", "512", "1024", "2048"};
const StringArray RateValues = {"44100", "48000", "88200", "96000"};
const StringArray FileFormats = {"WAV", "AIFF"};
const StringArray FileConfigs = {"Multiple Mono Files", "Single Interleaved"};

