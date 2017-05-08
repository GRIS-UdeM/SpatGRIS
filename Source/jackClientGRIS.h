/*
 This file is part of spatServerGRIS.
 
 Developers: Nicolas Masson
 
 spatServerGRIS is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.
 
 spatServerGRIS is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with spatServerGRIS.  If not, see <http://www.gnu.org/licenses/>.
 */


#ifndef jackClientGris_h
#define jackClientGris_h

#include <stdlib.h>
#include <vector>
#include <stdio.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "../JuceLibraryCode/JuceHeader.h"

#include <jack/jack.h>
#include <jack/transport.h>
#include <jack/types.h>
#include <jack/session.h>

using namespace std;

struct Client {
    String          name;
    unsigned int    portStart     = 1;
    unsigned int    portEnd       = 32;
    unsigned int    portAvailable = 0;
    bool            connected     = false;
};


struct SourceIn {
    unsigned int id;
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
    
    float azimuth = 0.0f;
    float zenith = 0.0f;
    float radius = 0.0f;
    
    float aziSpan = 0.0f;
    float zenSpan = 0.0f;
    
    bool  isMuted = false;
    bool  isSolo = false;
    float gain;//Not Implemented
};

struct SpeakerOut {
    unsigned int id;
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
    
    float azimuth = 0.0f;
    float zenith = 0.0f;
    float radius = 0.0f;
    
    bool  isMuted = false;
    bool  isSolo = false;
    float gain;//Not Implemented
};

//Mode Spat
typedef enum {
    FreeBasic = 0,
    VBap,
    DBap,
    HRTF
} ModeSpatEnum;
static const StringArray ModeSpatString = {"Free basic", "VBap", "DBap", "HRTF"};

//Settings Jack Server
static const StringArray BufferSize = {"32", "64", "128", "256", "512", "1024", "2048"};
static const StringArray RateValues = {"44100", "48000", "88200", "96000"};

static const char* ClientName =     "jackClientGris";
static const char* DriverNameSys =  "coreaudio";
static const char* ClientNameSys =  "system";

//Limit SpatServer In/Out
static unsigned int const MaxInputs  = 256;
static unsigned int const MaxOutputs = 256;


class jackClientGris {
public:

    //Jack var
    jack_client_t *client;

    vector<jack_port_t *> inputsPort;
    vector<jack_port_t *> outputsPort;

    //Noise Sound
    vector<double> sineNoise;
    int left_phase;
    int right_phase;
    
    //Mute Solo Vu meter
    float levelsIn[MaxInputs];
    float levelsOut[MaxOutputs];
    
    bool muteIn[MaxInputs];
    bool muteOut[MaxOutputs];
    
    bool soloIn[MaxInputs+1];
    bool soloOut[MaxOutputs+1];
    
    float masterGainOut;
    //------------------------
    vector<Client> listClient;
    SourceIn   listSourceIn   [MaxInputs];
    SpeakerOut listSpeakerOut [MaxOutputs];
    
    bool         processBlockOn;
    ModeSpatEnum modeSelected;
    
    bool noiseSound;
    bool autoConnection;
    bool overload;
    
    unsigned int sampleRate;
    unsigned int bufferSize;
    unsigned int numberInputs;
    unsigned int numberOutputs;

    
    //---------------------------------
    jackClientGris(unsigned int bufferS = 1024);
    virtual ~jackClientGris();
    
    bool  isReady() { return clientReady; }
    float getCpuUsed() const { return jack_cpu_load(client); }
    float getLevelsIn(int index) const { return levelsIn[index]; }
    float getLevelsOut(int index) const { return levelsOut[index]; }
    
    
    void addRemoveInput(int number);
    void clearOutput();
    bool addOutput();
    void removeOutput(int number);

    
    void autoConnectClient();
    void connectionClient(String name, bool connect = true);
    void updateClientPortAvailable();
    
    string getClientName(const char * port);
    unsigned int getPortStartClient(String nameClient);
    
    //Recording param =========================
    void prepareToRecord(int minuteR = 1);
    void startRecord(){ this->recording = true; }
    void stopRecort(){ this->recording = false; }
    vector<jack_default_audio_sample_t> buffersToRecord [MaxOutputs];
    unsigned int indexRecord = 1;
    unsigned int endIndexRecord = 1;
    bool recording;
    
private:
    
    
    bool clientReady;
    void connectedGristoSystem();
    
};


#endif
