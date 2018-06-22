#include "ServerGrisConstants.h"

const char *DeviceName = "GRIS";
const char *ClientName = "ServerGRIS";

#ifdef __linux__
const char *DriverNameSys = "alsa";
#else
const char *DriverNameSys = "coreaudio";
#endif

const char *ClientNameSys = "system";

const char *ClientNameIgnor = "JAR::57";

const StringArray ModeSpatString = {"VBAP",  "LBAP", "BINAURAL", "STEREO"};
