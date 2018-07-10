/*
 This file is part of ServerGris.
 
 Developers: Olivier Belanger, Nicolas Masson
 
 ServerGris is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.
 
 ServerGris is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with ServerGris.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef JACKCLIENTGRIS_H
#define JACKCLIENTGRIS_H

#include <stdlib.h>
#include <vector>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <unistd.h>

#ifdef __linux__
#include <mutex>
#endif

#include "../JuceLibraryCode/JuceHeader.h"

#include <jack/jack.h>
#include <jack/transport.h>
#include <jack/types.h>
#include <jack/session.h>

#include "vbap.h"
#include "lbap.h"

class Speaker;
using namespace std;

// Limits of ServerGris In/Out.
static unsigned int const MaxInputs  = 256;
static unsigned int const MaxOutputs = 256;

typedef struct {
    lbap_pos pos;
    float gains[MaxOutputs];
    float y[MaxOutputs];
} LBAP_DATA;
 
struct Client {
    String       name;
    unsigned int portStart     = 0;
    unsigned int portEnd       = 0;
    unsigned int portAvailable = 0;
    bool         initialized   = false;
    bool         connected     = false;
};

struct SourceIn {
    unsigned int id;
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
    
    float azimuth = 0.0f;
    float zenith = 0.0f;
    float radius = 0.0f;
    float depth = 1.0f;
    float aziSpan = 0.0f;
    float zenSpan = 0.0f;

    float lbap_gains[MaxOutputs];
    float lbap_y[MaxOutputs];
    lbap_pos lbap_last_pos;

    bool  isMuted = false;
    bool  isSolo = false;
    float gain;            // Not used yet.

    int directOut = 0;
    
    VBAP_DATA * paramVBap;
};

struct SpeakerOut {
    unsigned int id;
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
    
    float azimuth = 0.0f;
    float zenith = 0.0f;
    float radius = 0.0f;

    float gain = 1.0f;

    bool hpActive = false;
    double b1 = 0.0;
    double b2 = 0.0;
    double b3 = 0.0;
    double b4 = 0.0;
    double ha0 = 0.0;
    double ha1 = 0.0;
    double ha2 = 0.0;

    bool  isMuted = false;
    bool  isSolo = false;

    int outputPatch = 0;

    bool directOut = false;
};

// Spatialization modes.
typedef enum {
    VBAP = 0,
    LBAP,
    VBAP_HRTF,
    STEREO
} ModeSpatEnum;

// Audio recorder class used to write a monophonic soundfile on disk.
class AudioRecorder
{
public:
    AudioRecorder() : backgroundThread ("Audio Recorder Thread"), activeWriter (nullptr) {
        backgroundThread.startThread();
    }

    ~AudioRecorder() {
        stop();
    }

    //==============================================================================
    void startRecording(const File& file, unsigned int sampleRate, String extF) {
        stop();

        // Create an OutputStream to write to our destination file.
        file.deleteFile();
        ScopedPointer<FileOutputStream> fileStream(file.createOutputStream());

        AudioFormatWriter *writer;

        if (fileStream != nullptr) {
            // Now create a writer object that writes to our output stream...
            if (extF == ".wav") {
                WavAudioFormat wavFormat;
                writer = wavFormat.createWriterFor(fileStream, sampleRate, 1, 24, NULL, 0);
            } else {
                AiffAudioFormat aiffFormat;
                writer = aiffFormat.createWriterFor(fileStream, sampleRate, 1, 24, NULL, 0);
            }

            if (writer != nullptr) {
                fileStream.release(); // (passes responsibility for deleting the stream to the writer object that is now using it)

                // Now we'll create one of these helper objects which will act as a FIFO buffer, and will
                // write the data to disk on our background thread.
                threadedWriter = new AudioFormatWriter::ThreadedWriter (writer, backgroundThread, 32768);

                // And now, swap over our active writer pointer so that the audio callback will start using it..
                const ScopedLock sl (writerLock);
                activeWriter = threadedWriter;
            }
        }
    }

    void stop() {
        if (activeWriter == nullptr) { return; }

        // First, clear this pointer to stop the audio callback from using our writer object.
        {
            const ScopedLock sl (writerLock);
            activeWriter = nullptr;
        }

        // Now we can delete the writer object. It's done in this order because the deletion could
        // take a little time while remaining data gets flushed to disk, so it's best to avoid blocking
        // the audio callback while this happens.
        threadedWriter = nullptr;
    }

    void recordSamples(float **samples, int numSamples) {
        const ScopedLock sl (writerLock);
        activeWriter->write (samples, numSamples);
    }

private:
    TimeSliceThread backgroundThread; // the thread that will write our audio data to disk
    ScopedPointer<AudioFormatWriter::ThreadedWriter> threadedWriter; // the FIFO used to buffer the incoming data
    CriticalSection writerLock;
    AudioFormatWriter::ThreadedWriter* volatile activeWriter;
};


class jackClientGris {
public:
    // class variables.
    //-----------------
    unsigned int sampleRate;
    unsigned int bufferSize;
    unsigned int numberInputs;
    unsigned int numberOutputs;
    unsigned int maxOutputPatch;
    vector<int> outputPatches;

    // Jack variables.
    jack_client_t *client;
    vector<jack_port_t *> inputsPort;
    vector<jack_port_t *> outputsPort;

    // Interpolation and master gain values.
    float interMaster;
    float masterGainOut;

    // Global solo states.
    bool soloIn;
    bool soloOut;

    // Pink noise test sound.
    float c0;
    float c1;
    float c2;
    float c3;
    float c4;
    float c5;
    float c6;
    float pinkNoiseGain;
    bool pinkNoiseSound;

    // Crossover highpass filter.
    double x1[MaxOutputs];
    double x2[MaxOutputs];
    double x3[MaxOutputs];
    double x4[MaxOutputs];
    double y1[MaxOutputs];
    double y2[MaxOutputs];
    double y3[MaxOutputs];
    double y4[MaxOutputs];

    // Mute / Solo / VuMeter.
    float levelsIn[MaxInputs];
    float levelsOut[MaxOutputs];
    
    // Client list.
    vector<Client> listClient;
    mutex          lockListClient;

    // Source and output lists.
    SourceIn   listSourceIn   [MaxInputs];
    SpeakerOut listSpeakerOut [MaxOutputs];

    // Enable/disable jack process callback.
    bool         processBlockOn;

    // True when jack reports an xrun.
    bool overload;

    // Which spatialization mode is selected.
    ModeSpatEnum modeSelected;
    
    bool autoConnection; // not sure this one is necessary ?

    // VBAP data.
    unsigned int vbapDimensions;
    vector<vector<int>> vbap_triplets;
    int vbapSourcesToUpdate[MaxInputs];

    // BINAURAL data.
    unsigned int hrtf_count[16];
    float hrtf_input_tmp[16][128];
    float vbap_hrtf_left_impulses[16][128];
    float vbap_hrtf_right_impulses[16][128];

    // STEREO data.
    float last_azi[MaxInputs];

    // LBAP data.
    lbap_field *lbap_speaker_field;

    // Recording parameters.
    AudioRecorder recorder[MaxOutputs];
    unsigned int indexRecord = 0;
    bool recording;

    // Class methods.
    //---------------

    jackClientGris();
    virtual ~jackClientGris();

    // Audio Status.
    bool  isReady() { return clientReady; }
    float getCpuUsed() const { return jack_cpu_load(client); }
    float getLevelsIn(int index) const { return levelsIn[index]; }
    float getLevelsOut(int index) const { return levelsOut[index]; }

    // Manage Inputs / Outputs.
    void addRemoveInput(unsigned int number);
    void clearOutput();
    bool addOutput(unsigned int outputPatch);
    void removeOutput(int number);

    // Manage clients.
    void disconnectAllClient();
    void autoConnectClient();
    void connectionClient(String name, bool connect = true);
    void updateClientPortAvailable(bool fromJack);
    string getClientName(const char *port);
    unsigned int getPortStartClient(String nameClient);

    // Recording.
    void prepareToRecord();
    void startRecord() { this->indexRecord = 0; this->recording = true; }
    void stopRecord() { this->recording = false; };
    void setRecordFormat(int format) { this->recordFormat = format; };
    int getRecordFormat() { return this->recordFormat; };
    void setRecordingPath(String filePath) { this->recordPath = filePath; }
    String getRecordingPath() { return this->recordPath; }
    bool isSavingRun() { return this->recording; };

    // Initialize VBAP algorithm.
    bool initSpeakersTripplet(vector<Speaker *>  listSpk, int dimensions, bool needToComputeVbap);

    // Initialize LBAP algorithm.
    bool lbapSetupSpeakerField(vector<Speaker *>  listSpk);

    // Need to update a source VBAP data.
    void updateSourceVbap(int idS);

    // Reinit HRTF delay lines.
    void resetHRTF();
    
private:
    // Tells if an error occured while setting up the client.
    bool clientReady;

    // Private recording parameters.
    int recordFormat = 0;    // 0 = WAV, 1 = AIFF
    String recordPath = "";

    // This structure is used to compute the VBAP algorithm only once. Each source only gets a copy.
    VBAP_DATA *paramVBap;

    // Connect the server's outputs to the system's inputs.
    void connectedGristoSystem();
};

#endif /* JACKCLIENTGRIS_H */
