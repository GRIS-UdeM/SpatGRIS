/*
 This file is part of SpatGRIS2.

 Developers: Samuel Béland, Olivier Bélanger, Nicolas Masson

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

#include "JackClientGRIS.h"

#include <array>
#include <cstdarg>

#include "MainComponent.h"
#include "ServerGrisConstants.h"
#include "Speaker.h"
#include "spat/vbap.h"

static bool jackClientLogPrint = true;

//==============================================================================
// Utilities.
static bool intVectorContains(std::vector<int> const & vec, int const value)
{
    return std::find(vec.begin(), vec.end(), value) != vec.end();
}

//==============================================================================
static void jack_client_log(const char * format, ...)
{
    if (jackClientLogPrint) {
        char buffer[256];
        va_list args;
        va_start(args, format);
        vsprintf(buffer, format, args);
        va_end(args);
        printf("%s", buffer);
    }
}

// Jack processing callback.
static int process_audio(jack_nframes_t const nFrames, void * arg)
{
    auto * jackCli = static_cast<JackClientGris *>(arg);
    return jackCli->processAudio(nFrames);
}

//==============================================================================
// Jack callback functions.
void session_callback(jack_session_event_t * event, void * arg)
{
    auto * jackCli = static_cast<JackClientGris *>(arg);

    char returnValue[100];
    jack_client_log("session notification\n");
    jack_client_log("path %s, uuid %s, type: %s\n",
                    event->session_dir,
                    event->client_uuid,
                    event->type == JackSessionSave ? "save" : "quit");

    snprintf(returnValue, 100, "jack_simple_session_client %s", event->client_uuid);
    event->command_line = strdup(returnValue);

    jack_session_reply(jackCli->getClient(), event);

    jack_session_event_free(event);
}

//==============================================================================
int graphOrderCallback(void * arg)
{
    auto * jackCli = static_cast<JackClientGris *>(arg);
    jack_client_log("graph_order_callback...\n");
    jackCli->updateClientPortAvailable(true);
    jack_client_log("... done!\n");
    return 0;
}

//==============================================================================
int xRunCallback(void * /*arg*/)
{
    jassertfalse;
    /*auto * jackCli = static_cast<JackClientGris*>(arg);
    jackCli->mIsOverloaded = true;
    jack_client_log("Jack buffer overrun!!!\n");*/
    return 0;
}

//==============================================================================
void jackShutdown(void * /*arg*/)
{
    juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon,
                                           "FATAL ERROR",
                                           "Please check :\n - Buffer Size\n - Sample Rate\n - Inputs/Outputs");
    jack_client_log("FATAL ERROR: Jack shutdown!\n");
    exit(1);
}

//==============================================================================
void client_registration_callback(const char * const name, int const registered, void * arg)
{
    jack_client_log("Jack client registration : %s : ", name);
    if (!strcmp(name, CLIENT_NAME_IGNORE)) {
        jack_client_log("ignored\n");
        return;
    }

    auto * jackCli = static_cast<JackClientGris *>(arg);
    jackCli->clientRegistrationCallback(name, registered);
}

//==============================================================================
void port_registration_callback(jack_port_id_t const a, int const registered, void * /*arg*/)
{
    jack_client_log("Jack port : %d : ", a);
    if (registered) {
        jack_client_log("registered\n");
    } else {
        jack_client_log("deleted\n");
    }
}

//==============================================================================
void port_connect_callback(jack_port_id_t const a, jack_port_id_t const b, int const connect, void * arg)
{
    auto * jackCli = static_cast<JackClientGris *>(arg);
    jackCli->portConnectCallback(a, b, connect);
}

//==============================================================================
// Load samples from a wav file into a float array.
static juce::AudioBuffer<float> getSamplesFromWavFile(juce::File const & file)
{
    jassert(file.existsAsFile());

    auto const factor{ std::pow(2.0f, 31.0f) };

    juce::WavAudioFormat wavAudioFormat{};
    std::unique_ptr<juce::AudioFormatReader> audioFormatReader{
        wavAudioFormat.createReaderFor(file.createInputStream().release(), true)
    };
    jassert(audioFormatReader);
    std::array<int *, 2> wavData{};
    wavData[0] = new int[audioFormatReader->lengthInSamples];
    wavData[1] = new int[audioFormatReader->lengthInSamples];
    audioFormatReader->read(wavData.data(), 2, 0, static_cast<int>(audioFormatReader->lengthInSamples), false);
    juce::AudioBuffer<float> samples{ 2, static_cast<int>(audioFormatReader->lengthInSamples) };
    for (int i{}; i < 2; ++i) {
        for (int j{}; j < audioFormatReader->lengthInSamples; ++j) {
            samples.setSample(i, j, wavData[i][j] / factor);
        }
    }

    for (auto * it : wavData) {
        delete[] it;
    }
    return samples;
}

//==============================================================================
// JackClientGris class definition.
JackClientGris::JackClientGris()
{
    // Initialize variables.
    this->mPinkNoiseActive = false;
    this->mClientReady = false;
    this->mAutoConnection = false;
    this->mIsOverloaded = false;
    this->mMasterGainOut = 1.0f;
    this->mPinkNoiseGain = 0.1f;
    this->mProcessBlockOn = true;
    this->mModeSelected = VBAP;
    this->mIsRecording = false;

    this->mAttenuationLinearGain[0] = 0.01584893;       // -36 dB
    this->mAttenuationLowpassCoefficient[0] = 0.867208; // 1000 Hz
    for (unsigned int i = 0; i < MaxInputs; ++i) {
        this->mVbapSourcesToUpdate[i] = 0;
        this->mAttenuationLowpassY[i] = 0.0f;
        this->mAttenuationLowpassZ[i] = 0.0f;
        this->mLastAttenuationGain[i] = 0.0f;
        this->mLastAttenuationCoefficient[i] = 0.0f;
    }

    // Initialize impulse responses for VBAP+HRTF (BINAURAL mode).
    // Azimuth = 0
    juce::String names0[8] = { "H0e025a.wav", "H0e020a.wav", "H0e065a.wav", "H0e110a.wav",
                               "H0e155a.wav", "H0e160a.wav", "H0e115a.wav", "H0e070a.wav" };
    int reverse0[8] = { 1, 0, 0, 0, 0, 1, 1, 1 };
    for (int i = 0; i < 8; i++) {
        auto const file{ HRTF_FOLDER_0.getChildFile(names0[i]) };
        auto const stbuf{ getSamplesFromWavFile(file) };
        auto const leftChannel{ reverse0[i] };
        auto const rightChannel{ 1 - reverse0[i] };
        std::memcpy(mVbapHrtfLeftImpulses[i], stbuf.getReadPointer(leftChannel), 128);
        std::memcpy(mVbapHrtfRightImpulses[i], stbuf.getReadPointer(rightChannel), 128);
    }
    // Azimuth = 40
    juce::String names40[6]
        = { "H40e032a.wav", "H40e026a.wav", "H40e084a.wav", "H40e148a.wav", "H40e154a.wav", "H40e090a.wav" };
    int reverse40[6] = { 1, 0, 0, 0, 1, 1 };
    for (int i = 0; i < 6; i++) {
        auto const file{ HRTF_FOLDER_40.getChildFile(names40[i]) };
        auto const stbuf{ getSamplesFromWavFile(file) };
        auto const leftChannel{ reverse40[i] };
        auto const rightChannel{ 1 - reverse40[i] };
        std::memcpy(mVbapHrtfLeftImpulses[i], stbuf.getReadPointer(leftChannel), 128);
        std::memcpy(mVbapHrtfRightImpulses[i], stbuf.getReadPointer(rightChannel), 128);
    }
    // Azimuth = 80
    for (int i = 0; i < 2; i++) {
        auto const file{ HRTF_FOLDER_80.getChildFile("H80e090a.wav") };
        auto const stbuf{ getSamplesFromWavFile(file) };
        auto const leftChannel{ 1 - i };
        auto const rightChannel{ i };
        std::memcpy(mVbapHrtfLeftImpulses[i], stbuf.getReadPointer(leftChannel), 128);
        std::memcpy(mVbapHrtfRightImpulses[i], stbuf.getReadPointer(rightChannel), 128);
    }

    this->resetHrtf();

    // Initialize STEREO data.
    for (unsigned int i = 0; i < MaxInputs; ++i) {
        this->mLastAzimuth[i] = 0.0f;
    }

    // Initialize LBAP data.
    this->mLbapSpeakerField = lbap_field_init();
    for (unsigned int i = 0; i < MaxInputs; i++) {
        this->mSourcesIn[i].lbap_last_pos.azi = -1;
        this->mSourcesIn[i].lbap_last_pos.ele = -1;
        this->mSourcesIn[i].lbap_last_pos.rad = -1;
        for (unsigned int o = 0; o < MaxOutputs; o++) {
            this->mSourcesIn[i].lbap_gains[o] = this->mSourcesIn[i].lbap_y[o] = 0.0;
        }
    }

    // Initialize highpass filter delay samples.
    for (unsigned int i = 0; i < MaxOutputs; i++) {
        this->mCrossoverHighpassX1[i] = 0.0;
        this->mCrossoverHighpassX2[i] = 0.0;
        this->mCrossoverHighpassX3[i] = 0.0;
        this->mCrossoverHighpassX4[i] = 0.0;
        this->mCrossoverHighpassY1[i] = 0.0;
        this->mCrossoverHighpassY2[i] = 0.0;
        this->mCrossoverHighpassY3[i] = 0.0;
        this->mCrossoverHighpassY4[i] = 0.0;
    }

    this->mClients = std::vector<Client>();

    this->mSoloIn = false;
    this->mSoloOut = false;

    this->mInputsPort = std::vector<jack_port_t *>();
    this->mOutputsPort = std::vector<jack_port_t *>();
    this->mInterMaster = 0.8f;
    this->mMaxOutputPatch = 0;

    // open a client connection to the JACK server. Start server if it is not running.
    jack_options_t options = JackNullOption;
    jack_status_t status;

    jack_client_log("\nStart Jack Client\n");
    jack_client_log("=================\n");

    this->mClient = jackClientOpen(CLIENT_NAME, options, &status, SYS_DRIVER_NAME);
    if (this->mClient == NULL) {
        jack_client_log("\nTry again...\n");
        options = JackServerName;
        this->mClient = jackClientOpen(CLIENT_NAME, options, &status, SYS_DRIVER_NAME);
        if (this->mClient == NULL) {
            jack_client_log("\n\n jack_client_open() failed, status = 0x%2.0x\n", status);
            if (status & JackServerFailed) {
                jack_client_log("\n\n Unable to connect to JACK server\n");
            }
        }
    }
    if (status & JackServerStarted) {
        jack_client_log("\n jackdmp wasn't running so it was started\n");
    }
    if (status & JackNameNotUnique) {
        CLIENT_NAME = jack_get_client_name(this->mClient);
        jack_client_log("\n Chosen name already existed, new unique name `%s' assigned\n", CLIENT_NAME);
    }

    // Register Jack callbacks and ports.
    jack_on_shutdown(this->mClient, jackShutdown, this);
    jack_set_process_callback(this->mClient, process_audio, this);
    jack_set_client_registration_callback(this->mClient, client_registration_callback, this);
    jack_set_session_callback(this->mClient, session_callback, this);
    jack_set_port_connect_callback(this->mClient, port_connect_callback, this);
    jack_set_port_registration_callback(this->mClient, port_registration_callback, this);
    jack_set_graph_order_callback(this->mClient, graphOrderCallback, this);
    jack_set_xrun_callback(this->mClient, xRunCallback, this);

    mSampleRate = jack_get_sample_rate(this->mClient);
    mBufferSize = jack_get_buffer_size(this->mClient);

    jack_client_log("\nJack engine sample rate: %d \n", mSampleRate);
    jack_client_log("Jack engine buffer size: %d \n", mBufferSize);

    // Initialize pink noise
    srand((unsigned int)time(NULL));
    this->mPinkNoiseC0 = this->mPinkNoiseC1 = this->mPinkNoiseC2 = this->mPinkNoiseC3 = this->mPinkNoiseC4
        = this->mPinkNoiseC5 = this->mPinkNoiseC6 = 0.0;

    // Print available inputs ports.
    const char ** ports = jack_get_ports(this->mClient, NULL, NULL, JackPortIsInput);
    if (ports == NULL) {
        jack_client_log("\n No input ports!\n");
        return;
    }
    this->mNumberInputs = 0;
    jack_client_log("\nInput ports:\n\n");
    while (ports[this->mNumberInputs]) {
        jack_client_log("%s\n", ports[this->mNumberInputs]);
        this->mNumberInputs += 1;
    }
    jack_free(ports);
    jack_client_log("\nNumber of input ports: %d\n\n", this->mNumberInputs);

    // Print available outputs ports.
    ports = jack_get_ports(mClient, NULL, NULL, JackPortIsOutput);
    if (ports == NULL) {
        jack_client_log("\n No output ports!\n");
        return;
    }
    this->mNumberOutputs = 0;
    jack_client_log("Output ports:\n\n");
    while (ports[this->mNumberOutputs]) {
        jack_client_log("%s\n", ports[this->mNumberOutputs]);
        this->mNumberOutputs += 1;
    }
    jack_free(ports);
    jack_client_log("\nNumber of output ports: %d\n\n", this->mNumberOutputs);

    // Activate client and connect the ports.
    // Playback ports are "input" to the backend, and capture ports are "output" from it.
    if (jack_activate(this->mClient)) {
        jack_client_log("\n\n Jack cannot activate client.");
        return;
    }

    jack_client_log("\nJack Client Run\n");
    jack_client_log("=============== \n");

    this->mClientReady = true;
}

//==============================================================================
void JackClientGris::resetHrtf()
{
    for (unsigned int i = 0; i < 16; i++) {
        this->mHrtfCount[i] = 0;
        for (int j = 0; j < 128; j++) {
            this->mHrtfInputTmp[i][j] = 0.0f;
        }
    }
}

//==============================================================================
void JackClientGris::clientRegistrationCallback(char const * name, int regist)
{
    mClientsLock.lock();
    if (regist) {
        Client cli;
        cli.name = name;
        mClients.push_back(cli);
        jack_client_log("registered\n");
    } else {
        for (std::vector<Client>::iterator iter = mClients.begin(); iter != mClients.end(); ++iter) {
            if (iter->name == juce::String(name)) {
                mClients.erase(iter);
                jack_client_log("deleted\n");
                break;
            }
        }
    }
    mClientsLock.unlock();
}

//==============================================================================
void JackClientGris::portConnectCallback(jack_port_id_t const a, jack_port_id_t const b, int const connect) const
{
    jack_client_log("Jack port : ");
    if (connect) {
        // Stop Auto connection with system.
        if (!mAutoConnection) {
            std::string nameClient = jack_port_name(jack_port_by_id(mClient, a));
            std::string tempN = jack_port_short_name(jack_port_by_id(mClient, a));
            nameClient = nameClient.substr(0, nameClient.size() - (tempN.size() + 1));
            if ((nameClient != CLIENT_NAME && nameClient != SYS_CLIENT_NAME) || nameClient == SYS_CLIENT_NAME) {
                jack_disconnect(mClient,
                                jack_port_name(jack_port_by_id(mClient, a)),
                                jack_port_name(jack_port_by_id(mClient, b)));
            }
        }
        jack_client_log("Connect ");
    } else {
        jack_client_log("Disconnect ");
    }
    jack_client_log("%d <> %d \n", a, b);
}

//==============================================================================
void JackClientGris::prepareToRecord()
{
    int num_of_channels;
    if (this->mOutputsPort.size() < 1) {
        return;
    }

    this->mIsRecording = false;
    this->mIndexRecord = 0;
    this->mOutputFileNames.clear();

    juce::String channelName;
    juce::File fileS = juce::File(this->mRecordPath);
    juce::String fname = fileS.getFileNameWithoutExtension();
    juce::String extF = fileS.getFileExtension();
    juce::String parent = fileS.getParentDirectory().getFullPathName();

    if (this->mModeSelected == VBAP || this->mModeSelected == LBAP) {
        num_of_channels = (int)this->mOutputsPort.size();
        for (int i = 0; i < num_of_channels; ++i) {
            if (intVectorContains(this->mOutputPatches, i + 1)) {
                channelName = parent + "/" + fname + "_" + juce::String(i + 1).paddedLeft('0', 3) + extF;
                juce::File fileC = juce::File(channelName);
                this->mRecorders[i].startRecording(fileC, this->mSampleRate, extF);
                this->mOutputFileNames.add(fileC);
            }
        }
    } else if (this->mModeSelected == VBAP_HRTF || this->mModeSelected == STEREO) {
        num_of_channels = 2;
        for (int i = 0; i < num_of_channels; ++i) {
            channelName = parent + "/" + fname + "_" + juce::String(i + 1).paddedLeft('0', 3) + extF;
            juce::File fileC = juce::File(channelName);
            this->mRecorders[i].startRecording(fileC, this->mSampleRate, extF);
            this->mOutputFileNames.add(fileC);
        }
    }
}

//==============================================================================
void JackClientGris::startRecord()
{
    this->mIndexRecord = 0;
    this->mIsRecording = true;
}

//==============================================================================
void JackClientGris::addRemoveInput(unsigned int number)
{
    if (number < this->mInputsPort.size()) {
        while (number < this->mInputsPort.size()) {
            jack_port_unregister(mClient, this->mInputsPort.back());
            this->mInputsPort.pop_back();
        }
    } else {
        while (number > this->mInputsPort.size()) {
            juce::String nameIn = "input";
            nameIn += juce::String(this->mInputsPort.size() + 1);
            jack_port_t * newPort = jack_port_register(this->mClient,
                                                       nameIn.toStdString().c_str(),
                                                       JACK_DEFAULT_AUDIO_TYPE,
                                                       JackPortIsInput,
                                                       0);
            this->mInputsPort.push_back(newPort);
        }
    }
    connectedGrisToSystem();
}

//==============================================================================
void JackClientGris::clearOutput()
{
    int outS = (int)this->mOutputsPort.size();
    for (int i = 0; i < outS; i++) {
        jack_port_unregister(mClient, this->mOutputsPort.back());
        this->mOutputsPort.pop_back();
    }
}

//==============================================================================
bool JackClientGris::addOutput(unsigned int outputPatch)
{
    if (outputPatch > this->mMaxOutputPatch)
        this->mMaxOutputPatch = outputPatch;
    juce::String nameOut = "output";
    nameOut += juce::String(this->mOutputsPort.size() + 1);

    jack_port_t * newPort
        = jack_port_register(this->mClient, nameOut.toUTF8(), JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);
    this->mOutputsPort.push_back(newPort);
    connectedGrisToSystem();
    return true;
}

//==============================================================================
void JackClientGris::removeOutput(int number)
{
    jack_port_unregister(mClient, this->mOutputsPort.at(number));
    this->mOutputsPort.erase(this->mOutputsPort.begin() + number);
}

//==============================================================================
std::vector<int> JackClientGris::getDirectOutOutputPatches() const
{
    std::vector<int> directOutOutputPatches;
    for (auto const & it : mSpeakersOut) {
        if (it.directOut && it.outputPatch != 0)
            directOutOutputPatches.push_back(it.outputPatch);
    }
    return directOutOutputPatches;
}

//==============================================================================
void JackClientGris::muteSoloVuMeterIn(jack_default_audio_sample_t ** ins,
                                       jack_nframes_t const nframes,
                                       unsigned const sizeInputs)
{
    for (unsigned int i = 0; i < sizeInputs; ++i) {
        if (mSourcesIn[i].isMuted) { // Mute
            memset(ins[i], 0, sizeof(jack_default_audio_sample_t) * nframes);
        } else if (mSoloIn) { // Solo
            if (!mSourcesIn[i].isSolo) {
                memset(ins[i], 0, sizeof(jack_default_audio_sample_t) * nframes);
            }
        }

        // VuMeter
        auto maxGain = 0.0f;
        for (unsigned int j = 1; j < nframes; j++) {
            auto const absGain = fabsf(ins[i][j]);
            if (absGain > maxGain)
                maxGain = absGain;
        }
        mLevelsIn[i] = maxGain;
    }
}

//==============================================================================
void JackClientGris::muteSoloVuMeterGainOut(jack_default_audio_sample_t ** outs,
                                            jack_nframes_t const nframes,
                                            unsigned const sizeOutputs,
                                            float const gain)
{
    unsigned int num_of_channels = 2;
    double inval = 0.0, val = 0.0;

    if (mModeSelected == VBAP || mModeSelected == LBAP) {
        num_of_channels = sizeOutputs;
    }

    for (unsigned int i = 0; i < sizeOutputs; ++i) {
        if (mSpeakersOut[i].isMuted) { // Mute
            memset(outs[i], 0, sizeof(jack_default_audio_sample_t) * nframes);
        } else if (mSoloOut) { // Solo
            if (!mSpeakersOut[i].isSolo) {
                memset(outs[i], 0, sizeof(jack_default_audio_sample_t) * nframes);
            }
        }

        // Speaker independent gain.
        auto const speakerGain = mSpeakersOut[i].gain;
        for (unsigned int f = 0; f < nframes; ++f) {
            outs[i][f] *= speakerGain * gain;
        }

        // Speaker independent crossover filter.
        if (mSpeakersOut[i].hpActive) {
            SpeakerOut so = mSpeakersOut[i];
            for (unsigned int f = 0; f < nframes; ++f) {
                inval = (double)outs[i][f];
                val = so.ha0 * inval + so.ha1 * mCrossoverHighpassX1[i] + so.ha2 * mCrossoverHighpassX2[i]
                      + so.ha1 * mCrossoverHighpassX3[i] + so.ha0 * mCrossoverHighpassX4[i]
                      - so.b1 * mCrossoverHighpassY1[i] - so.b2 * mCrossoverHighpassY2[i]
                      - so.b3 * mCrossoverHighpassY3[i] - so.b4 * mCrossoverHighpassY4[i];
                mCrossoverHighpassY4[i] = mCrossoverHighpassY3[i];
                mCrossoverHighpassY3[i] = mCrossoverHighpassY2[i];
                mCrossoverHighpassY2[i] = mCrossoverHighpassY1[i];
                mCrossoverHighpassY1[i] = val;
                mCrossoverHighpassX4[i] = mCrossoverHighpassX3[i];
                mCrossoverHighpassX3[i] = mCrossoverHighpassX2[i];
                mCrossoverHighpassX2[i] = mCrossoverHighpassX1[i];
                mCrossoverHighpassX1[i] = inval;
                outs[i][f] = (jack_default_audio_sample_t)val;
            }
        }

        // VuMeter
        float maxGain = 0.0f;
        for (unsigned int j = 1; j < nframes; j++) {
            float absGain = fabsf(outs[i][j]);
            if (absGain > maxGain)
                maxGain = absGain;
        }
        mLevelsOut[i] = maxGain;

        // Record buffer.
        if (mIsRecording) {
            if (num_of_channels == sizeOutputs && i < num_of_channels) {
                if (intVectorContains(mOutputPatches, i + 1)) {
                    mRecorders[i].recordSamples(&outs[i], (int)nframes);
                }
            } else if (num_of_channels == 2 && i < num_of_channels) {
                mRecorders[i].recordSamples(&outs[i], (int)nframes);
            }
        }
    }

    // Recording index.
    if (!mIsRecording && mIndexRecord > 0) {
        if (num_of_channels == sizeOutputs) {
            for (unsigned int i = 0; i < sizeOutputs; ++i) {
                if (intVectorContains(mOutputPatches, i + 1) && i < num_of_channels) {
                    mRecorders[i].stop();
                }
            }
        } else if (num_of_channels == 2) {
            mRecorders[0].stop();
            mRecorders[1].stop();
        }
        mIndexRecord = 0;
    } else if (mIsRecording) {
        mIndexRecord += nframes;
    }
}

//==============================================================================
void JackClientGris::addNoiseSound(jack_default_audio_sample_t ** outs,
                                   jack_nframes_t const nframes,
                                   unsigned const sizeOutputs)
{
    float rnd;
    float val;
    float const fac = 1.0f / (static_cast<float>(RAND_MAX) / 2.0f);
    for (unsigned int nF = 0; nF < nframes; ++nF) {
        rnd = rand() * fac - 1.0f;
        mPinkNoiseC0 = mPinkNoiseC0 * 0.99886f + rnd * 0.0555179f;
        mPinkNoiseC1 = mPinkNoiseC1 * 0.99332f + rnd * 0.0750759f;
        mPinkNoiseC2 = mPinkNoiseC2 * 0.96900f + rnd * 0.1538520f;
        mPinkNoiseC3 = mPinkNoiseC3 * 0.86650f + rnd * 0.3104856f;
        mPinkNoiseC4 = mPinkNoiseC4 * 0.55000f + rnd * 0.5329522f;
        mPinkNoiseC5 = mPinkNoiseC5 * -0.7616f - rnd * 0.0168980f;
        val = mPinkNoiseC0 + mPinkNoiseC1 + mPinkNoiseC2 + mPinkNoiseC3 + mPinkNoiseC4 + mPinkNoiseC5 + mPinkNoiseC6
              + rnd * 0.5362f;
        val *= 0.2f;
        val *= mPinkNoiseGain;
        mPinkNoiseC6 = rnd * 0.115926f;

        for (unsigned int i = 0; i < sizeOutputs; i++) {
            outs[i][nF] += val;
        }
    }
}

//==============================================================================
void JackClientGris::processVbap(jack_default_audio_sample_t ** ins,
                                 jack_default_audio_sample_t ** outs,
                                 jack_nframes_t const nframes,
                                 unsigned const sizeInputs,
                                 unsigned const sizeOutputs)
{
    unsigned int f, i, o, ilinear;
    float y, interpG = 0.99, iogain = 0.0;

    if (mInterMaster == 0.0) {
        ilinear = 1;
    } else {
        ilinear = 0;
        interpG = powf(mInterMaster, 0.1) * 0.0099 + 0.99;
    }

    for (i = 0; i < sizeInputs; ++i) {
        if (mVbapSourcesToUpdate[i] == 1) {
            updateSourceVbap(i);
            mVbapSourcesToUpdate[i] = 0;
        }
    }

    for (o = 0; o < sizeOutputs; ++o) {
        memset(outs[o], 0, sizeof(jack_default_audio_sample_t) * nframes);
        for (i = 0; i < sizeInputs; ++i) {
            if (!mSourcesIn[i].directOut && mSourcesIn[i].paramVBap != nullptr) {
                iogain = mSourcesIn[i].paramVBap->gains[o];
                y = mSourcesIn[i].paramVBap->y[o];
                if (ilinear) {
                    interpG = (iogain - y) / nframes;
                    for (f = 0; f < nframes; ++f) {
                        y += interpG;
                        outs[o][f] += ins[i][f] * y;
                    }
                } else {
                    for (f = 0; f < nframes; ++f) {
                        y = iogain + (y - iogain) * interpG;
                        if (y < 0.0000000000001f) {
                            y = 0.0;
                        } else {
                            outs[o][f] += ins[i][f] * y;
                        }
                    }
                }
                mSourcesIn[i].paramVBap->y[o] = y;
            } else if (static_cast<unsigned int>(mSourcesIn[i].directOut - 1) == o) {
                for (f = 0; f < nframes; ++f) {
                    outs[o][f] += ins[i][f];
                }
            }
        }
    }
}

//==============================================================================
void JackClientGris::processLbap(jack_default_audio_sample_t ** ins,
                                 jack_default_audio_sample_t ** outs,
                                 jack_nframes_t nframes,
                                 unsigned sizeInputs,
                                 unsigned sizeOutputs)
{
    unsigned int f, i, o, ilinear;
    float y, gain, distance, distgain, distcoef, interpG = 0.99;
    lbap_pos pos;

    float filteredInputSignal[2048];
    memset(filteredInputSignal, 0, sizeof(float) * nframes);

    if (mInterMaster == 0.0) {
        ilinear = 1;
    } else {
        ilinear = 0;
        interpG = powf(mInterMaster, 0.1) * 0.0099 + 0.99;
    }

    for (o = 0; o < sizeOutputs; ++o) {
        memset(outs[o], 0, sizeof(jack_default_audio_sample_t) * nframes);
    }

    for (i = 0; i < sizeInputs; ++i) {
        if (!mSourcesIn[i].directOut) {
            lbap_pos_init_from_radians(&pos, mSourcesIn[i].radazi, mSourcesIn[i].radele, mSourcesIn[i].radius);
            pos.radspan = mSourcesIn[i].aziSpan;
            pos.elespan = mSourcesIn[i].zenSpan;
            distance = mSourcesIn[i].radius;
            if (!lbap_pos_compare(&pos, &mSourcesIn[i].lbap_last_pos)) {
                lbap_field_compute(mLbapSpeakerField, &pos, mSourcesIn[i].lbap_gains);
                lbap_pos_copy(&mSourcesIn[i].lbap_last_pos, &pos);
            }

            // Energy lost with distance, radius is in the range 0 - 2.6 (>1 is beyond HP circle).
            if (distance < 1.0f) {
                distgain = 1.0f;
                distcoef = 0.0f;
            } else {
                distance -= 1.0f;
                distance *= 1.25f;
                if (distance > 1.0f) {
                    distance = 1.0f;
                }
                distgain = (1.0f - distance) * (1.0f - mAttenuationLinearGain[0]) + mAttenuationLinearGain[0];
                distcoef = distance * mAttenuationLowpassCoefficient[0];
            }
            float diffgain = (distgain - mLastAttenuationGain[i]) / nframes;
            float diffcoef = (distcoef - mLastAttenuationCoefficient[i]) / nframes;
            float filtInY = mAttenuationLowpassY[i];
            float filtInZ = mAttenuationLowpassZ[i];
            float lastcoef = mLastAttenuationCoefficient[i];
            float lastgain = mLastAttenuationGain[i];
            for (unsigned int k = 0; k < nframes; k++) {
                lastcoef += diffcoef;
                lastgain += diffgain;
                filtInY = ins[i][k] + (filtInY - ins[i][k]) * lastcoef;
                filtInZ = filtInY + (filtInZ - filtInY) * lastcoef;
                filteredInputSignal[k] = filtInZ * lastgain;
            }
            mAttenuationLowpassY[i] = filtInY;
            mAttenuationLowpassZ[i] = filtInZ;
            mLastAttenuationGain[i] = distgain;
            mLastAttenuationCoefficient[i] = distcoef;
            //==============================================================================

            for (o = 0; o < sizeOutputs; ++o) {
                gain = mSourcesIn[i].lbap_gains[o];
                y = mSourcesIn[i].lbap_y[o];
                if (ilinear) {
                    interpG = (gain - y) / nframes;
                    for (f = 0; f < nframes; ++f) {
                        y += interpG;
                        outs[o][f] += filteredInputSignal[f] * y;
                    }
                } else {
                    for (f = 0; f < nframes; ++f) {
                        y = gain + (y - gain) * interpG;
                        if (y < 0.0000000000001f) {
                            y = 0.0;
                        } else {
                            outs[o][f] += filteredInputSignal[f] * y;
                        }
                    }
                }
                mSourcesIn[i].lbap_y[o] = y;
            }
        } else {
            for (o = 0; o < sizeOutputs; ++o) {
                if (static_cast<unsigned int>(mSourcesIn[i].directOut - 1) == o) {
                    for (f = 0; f < nframes; ++f) {
                        outs[o][f] += ins[i][f];
                    }
                }
            }
        }
    }
}

//==============================================================================
void JackClientGris::processVBapHrtf(jack_default_audio_sample_t ** ins,
                                     jack_default_audio_sample_t ** outs,
                                     jack_nframes_t nframes,
                                     unsigned sizeInputs,
                                     unsigned sizeOutputs)
{
    for (unsigned int o = 0; o < sizeOutputs; ++o) {
        memset(outs[o], 0, sizeof(jack_default_audio_sample_t) * nframes);
    }

    unsigned int ilinear;
    float interpG = 0.99f;
    if (mInterMaster == 0.0) {
        ilinear = 1;
    } else {
        ilinear = 0;
        interpG = powf(mInterMaster, 0.1f) * 0.0099f + 0.99f;
    }

    for (unsigned int i = 0; i < sizeInputs; ++i) {
        if (mVbapSourcesToUpdate[i] == 1) {
            updateSourceVbap(i);
            mVbapSourcesToUpdate[i] = 0;
        }
    }

    constexpr unsigned int MAX_FRAME_COUNT = 2048;
    constexpr unsigned int MAX_OUTPUTS_COUNT = 16;
    std::array<std::array<float, MAX_FRAME_COUNT>, MAX_OUTPUTS_COUNT> vbapouts{};

    jassert(sizeOutputs == MAX_OUTPUTS_COUNT);

    // zero mem
    std::for_each(std::begin(vbapouts), std::end(vbapouts), [=](auto & buffer) {
        std::fill(std::begin(buffer), std::begin(buffer) + nframes, 0.0f);
    });

    for (unsigned int o{}; o < MAX_OUTPUTS_COUNT; ++o) {
        for (unsigned int i{}; i < sizeInputs; ++i) {
            if (!mSourcesIn[i].directOut && mSourcesIn[i].paramVBap != nullptr) {
                float iogain = mSourcesIn[i].paramVBap->gains[o];
                float y = mSourcesIn[i].paramVBap->y[o];
                if (ilinear) {
                    interpG = (iogain - y) / nframes;
                    for (unsigned int f{}; f < nframes; ++f) {
                        y += interpG;
                        vbapouts[o][f] += ins[i][f] * y;
                    }
                } else {
                    for (unsigned int f{}; f < nframes; ++f) {
                        y = iogain + (y - iogain) * interpG;
                        if (y < 0.0000000000001f) {
                            y = 0.0f;
                        } else {
                            vbapouts[o][f] += ins[i][f] * y;
                        }
                    }
                }
                mSourcesIn[i].paramVBap->y[o] = y;
            }
        }

        int tmp_count;
        for (unsigned int f = 0; f < nframes; ++f) {
            tmp_count = mHrtfCount[o];
            for (unsigned int k = 0; k < 128; ++k) {
                if (tmp_count < 0) {
                    tmp_count += 128;
                }
                float const sig = mHrtfInputTmp[o][tmp_count];
                outs[0][f] += sig * mVbapHrtfLeftImpulses[o][k];
                outs[1][f] += sig * mVbapHrtfRightImpulses[o][k];
                --tmp_count;
            }
            ++mHrtfCount[o];
            if (mHrtfCount[o] >= 128) {
                mHrtfCount[o] = 0;
            }
            mHrtfInputTmp[o][mHrtfCount[o]] = vbapouts[o][f];
        }
    }

    // Add direct outs to the now stereo signal.
    for (unsigned int i = 0; i < sizeInputs; ++i) {
        if (mSourcesIn[i].directOut != 0) {
            if ((mSourcesIn[i].directOut % 2) == 1) {
                for (unsigned int f = 0; f < nframes; ++f) {
                    outs[0][f] += ins[i][f];
                }
            } else {
                for (unsigned int f = 0; f < nframes; ++f) {
                    outs[1][f] += ins[i][f];
                }
            }
        }
    }
}

//==============================================================================
void JackClientGris::processStereo(jack_default_audio_sample_t ** ins,
                                   jack_default_audio_sample_t ** outs,
                                   jack_nframes_t const nframes,
                                   unsigned const sizeInputs,
                                   unsigned const sizeOutputs)
{
    unsigned int f, i;
    float azi, last_azi, scaled;
    float factor = M_PI2 / 180.0f;
    float interpG = powf(mInterMaster, 0.1) * 0.0099 + 0.99;
    float gain = powf(10.0f, (sizeInputs - 1) * -0.1f * 0.05f);

    for (i = 0; i < sizeOutputs; ++i) {
        memset(outs[i], 0, sizeof(jack_default_audio_sample_t) * nframes);
    }

    for (i = 0; i < sizeInputs; ++i) {
        if (!mSourcesIn[i].directOut) {
            azi = mSourcesIn[i].azimuth;
            last_azi = mLastAzimuth[i];
            for (f = 0; f < nframes; ++f) {
                // Removes the chirp at 180->-180 degrees azimuth boundary.
                if (abs(last_azi - azi) > 300.0f) {
                    last_azi = azi;
                }
                last_azi = azi + (last_azi - azi) * interpG;
                if (last_azi < -90.0f) {
                    scaled = -90.0f - (last_azi + 90.0f);
                } else if (last_azi > 90) {
                    scaled = 90.0f - (last_azi - 90.0f);
                } else {
                    scaled = last_azi;
                }
                scaled = (scaled + 90) * factor;
                outs[0][f] += ins[i][f] * cosf(scaled);
                outs[1][f] += ins[i][f] * sinf(scaled);
            }
            mLastAzimuth[i] = last_azi;

        } else if ((mSourcesIn[i].directOut % 2) == 1) {
            for (f = 0; f < nframes; ++f) {
                outs[0][f] += ins[i][f];
            }
        } else {
            for (f = 0; f < nframes; ++f) {
                outs[1][f] += ins[i][f];
            }
        }
    }
    // Apply gain compensation.
    for (f = 0; f < nframes; ++f) {
        outs[0][f] *= gain;
        outs[1][f] *= gain;
    }
}

//==============================================================================
int JackClientGris::processAudio(jack_nframes_t const nframes)
{
    // Return if the user is editing the speaker setup.
    if (!mProcessBlockOn) {
        for (unsigned int i = 0; i < mOutputsPort.size(); ++i) {
            memset(((jack_default_audio_sample_t *)jack_port_get_buffer(mOutputsPort[i], nframes)),
                   0,
                   sizeof(jack_default_audio_sample_t) * nframes);
            mLevelsOut[i] = 0.0f;
        }
        return 0;
    }

    const unsigned int sizeInputs = static_cast<unsigned int>(mInputsPort.size());
    const unsigned int sizeOutputs = static_cast<unsigned int>(mOutputsPort.size());

    jack_default_audio_sample_t * ins[MaxInputs];
    jack_default_audio_sample_t * outs[MaxOutputs];

    for (unsigned int i = 0; i < sizeInputs; i++) {
        ins[i] = (jack_default_audio_sample_t *)jack_port_get_buffer(mInputsPort[i], nframes);
    }
    for (unsigned int i = 0; i < sizeOutputs; i++) {
        outs[i] = (jack_default_audio_sample_t *)jack_port_get_buffer(mOutputsPort[i], nframes);
    }

    muteSoloVuMeterIn(ins, nframes, sizeInputs);

    switch (mModeSelected) {
    case VBAP:
        processVbap(ins, outs, nframes, sizeInputs, sizeOutputs);
        break;
    case LBAP:
        processLbap(ins, outs, nframes, sizeInputs, sizeOutputs);
        break;
    case VBAP_HRTF:
        processVBapHrtf(ins, outs, nframes, sizeInputs, sizeOutputs);
        break;
    case STEREO:
        processStereo(ins, outs, nframes, sizeInputs, sizeOutputs);
        break;
    default:
        jassertfalse;
        break;
    }

    if (mPinkNoiseActive) {
        addNoiseSound(outs, nframes, sizeOutputs);
    }

    muteSoloVuMeterGainOut(outs, nframes, sizeOutputs, mMasterGainOut);

    mIsOverloaded = false;

    return 0;
}

//==============================================================================
void JackClientGris::connectedGrisToSystem()
{
    juce::String nameOut;
    this->clearOutput();
    for (unsigned int i = 0; i < this->mMaxOutputPatch; i++) {
        nameOut = "output";
        nameOut += juce::String(this->mOutputsPort.size() + 1);
        jack_port_t * newPort
            = jack_port_register(this->mClient, nameOut.toUTF8(), JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);
        this->mOutputsPort.push_back(newPort);
    }

    const char ** portsOut = jack_get_ports(this->mClient, NULL, JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput);
    const char ** portsIn = jack_get_ports(this->mClient, NULL, JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput);

    int i = 0;
    int j = 0;

    // DisConnect JackClientGris to system.
    while (portsOut[i]) {
        if (getClientName(portsOut[i]) == CLIENT_NAME) { // jackClient
            j = 0;
            while (portsIn[j]) {
                if (getClientName(portsIn[j]) == SYS_CLIENT_NAME && // system
                    jack_port_connected_to(jack_port_by_name(this->mClient, portsOut[i]), portsIn[j])) {
                    jack_disconnect(this->mClient, portsOut[i], portsIn[j]);
                }
                j += 1;
            }
        }
        i += 1;
    }

    i = 0;
    j = 0;

    // Connect JackClientGris to system.
    while (portsOut[i]) {
        if (getClientName(portsOut[i]) == CLIENT_NAME) { // jackClient
            while (portsIn[j]) {
                if (getClientName(portsIn[j]) == SYS_CLIENT_NAME) { // system
                    jack_connect(this->mClient, portsOut[i], portsIn[j]);
                    j += 1;
                    break;
                }
                j += 1;
            }
        }
        i += 1;
    }

    // Build output patch list.
    this->mOutputPatches.clear();
    for (unsigned int i = 0; i < this->mOutputsPort.size(); i++) {
        if (this->mSpeakersOut[i].outputPatch != 0) {
            this->mOutputPatches.push_back(this->mSpeakersOut[i].outputPatch);
        }
    }

    jack_free(portsIn);
    jack_free(portsOut);
}

//==============================================================================
bool JackClientGris::initSpeakersTriplet(std::vector<Speaker *> const & listSpk, int dimensions, bool needToComputeVbap)
{
    int j;
    if (listSpk.size() <= 0) {
        return false;
    }

    ls lss[MAX_LS_AMOUNT];
    int outputPatches[MAX_LS_AMOUNT];

    for (unsigned int i = 0; i < listSpk.size(); i++) {
        for (j = 0; j < MAX_LS_AMOUNT; j++) {
            if (listSpk[i]->getOutputPatch() == mSpeakersOut[j].outputPatch && !mSpeakersOut[j].directOut) {
                break;
            }
        }
        lss[i].coords.x = mSpeakersOut[j].x;
        lss[i].coords.y = mSpeakersOut[j].y;
        lss[i].coords.z = mSpeakersOut[j].z;
        lss[i].angles.azi = mSpeakersOut[j].azimuth;
        lss[i].angles.ele = mSpeakersOut[j].zenith;
        lss[i].angles.length = mSpeakersOut[j].radius; // Always 1.0 for VBAP.
        outputPatches[i] = mSpeakersOut[j].outputPatch;
    }

    if (needToComputeVbap) {
        this->mParamVBap
            = init_vbap_from_speakers(lss, (int)listSpk.size(), dimensions, outputPatches, this->mMaxOutputPatch, NULL);
        if (this->mParamVBap == NULL) {
            return false;
        }
    }

    for (unsigned int i = 0; i < MaxInputs; i++) {
        mSourcesIn[i].paramVBap = copy_vbap_data(this->mParamVBap);
    }

    int ** triplets;
    int num = vbap_get_triplets(mSourcesIn[0].paramVBap, &triplets);
    mVbapTriplets.clear();
    for (int i = 0; i < num; i++) {
        std::vector<int> row;
        for (int j = 0; j < 3; j++) {
            row.push_back(triplets[i][j]);
        }
        mVbapTriplets.push_back(row);
    }

    for (int i = 0; i < num; i++) {
        free(triplets[i]);
    }
    free(triplets);

    this->connectedGrisToSystem();

    return true;
}

//==============================================================================
bool JackClientGris::lbapSetupSpeakerField(std::vector<Speaker *> const & listSpk)
{
    int j;
    if (listSpk.size() <= 0) {
        return false;
    }

    float azimuth[MaxOutputs];
    float elevation[MaxOutputs];
    float radius[MaxOutputs];
    int outputPatch[MaxOutputs];

    for (unsigned int i = 0; i < listSpk.size(); i++) {
        for (j = 0; j < MAX_LS_AMOUNT; j++) {
            if (listSpk[i]->getOutputPatch() == mSpeakersOut[j].outputPatch && !mSpeakersOut[j].directOut) {
                break;
            }
        }
        azimuth[i] = mSpeakersOut[j].azimuth;
        elevation[i] = mSpeakersOut[j].zenith;
        radius[i] = mSpeakersOut[j].radius;
        outputPatch[i] = mSpeakersOut[j].outputPatch - 1;
    }

    lbap_speaker * speakers
        = lbap_speakers_from_positions(azimuth, elevation, radius, outputPatch, (int)listSpk.size());

    lbap_field_reset(this->mLbapSpeakerField);
    lbap_field_setup(this->mLbapSpeakerField, speakers, (int)listSpk.size());

    free(speakers);

    this->connectedGrisToSystem();

    return true;
}

//==============================================================================
void JackClientGris::setAttenuationDb(float value)
{
    this->mAttenuationLinearGain[0] = value;
}

//==============================================================================
void JackClientGris::setAttenuationHz(float value)
{
    this->mAttenuationLowpassCoefficient[0] = value;
}

//==============================================================================
void JackClientGris::updateSourceVbap(int idS)
{
    if (this->mVbapDimensions == 3) {
        if (mSourcesIn[idS].paramVBap != nullptr) {
            vbap2_flip_y_z(mSourcesIn[idS].azimuth,
                           mSourcesIn[idS].zenith,
                           mSourcesIn[idS].aziSpan,
                           mSourcesIn[idS].zenSpan,
                           mSourcesIn[idS].paramVBap);
        }
    } else if (this->mVbapDimensions == 2) {
        if (mSourcesIn[idS].paramVBap != nullptr) {
            vbap2(mSourcesIn[idS].azimuth, 0.0, mSourcesIn[idS].aziSpan, 0.0, mSourcesIn[idS].paramVBap);
        }
    }
}

//==============================================================================
void JackClientGris::connectionClient(juce::String name, bool connect)
{
    const char ** portsOut = jack_get_ports(this->mClient, NULL, JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput);
    const char ** portsIn = jack_get_ports(this->mClient, NULL, JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput);

    int i = 0;
    int j = 0;
    int startJ = 0;
    int endJ = 0;
    bool conn = false;
    this->updateClientPortAvailable(false);

    // Disconnect client.
    while (portsOut[i]) {
        if (getClientName(portsOut[i]) == name) {
            j = 0;
            while (portsIn[j]) {
                if (getClientName(portsIn[j]) == CLIENT_NAME && // jackClient
                    jack_port_connected_to(jack_port_by_name(this->mClient, portsOut[i]), portsIn[j])) {
                    jack_disconnect(this->mClient, portsOut[i], portsIn[j]);
                }
                j += 1;
            }
        }
        i += 1;
    }

    for (auto && cli : this->mClients) {
        if (cli.name == name) {
            // cli.connected = false;
            cli.connected = true; // since there is no checkbox anymore, lets set this to true by default.
        }
    }

    connectedGrisToSystem();

    if (!connect) {
        return;
    }

    // Connect other client to JackClientGris
    this->mAutoConnection = true;

    for (auto && cli : this->mClients) {
        i = 0;
        j = 0;
        juce::String nameClient = cli.name;
        startJ = cli.portStart - 1;
        endJ = cli.portEnd;

        while (portsOut[i]) {
            if (nameClient == name && nameClient.compare(getClientName(portsOut[i])) == 0) {
                while (portsIn[j]) {
                    if (getClientName(portsIn[j]) == CLIENT_NAME) {
                        if (j >= startJ && j < endJ) {
                            jack_connect(this->mClient, portsOut[i], portsIn[j]);
                            conn = true;
                            j += 1;
                            break;
                        } else {
                            j += 1;
                        }
                    } else {
                        j += 1;
                        startJ += 1;
                        endJ += 1;
                    }
                }
                cli.connected = conn;
            }
            i += 1;
        }
    }

    this->mAutoConnection = false;

    jack_free(portsIn);
    jack_free(portsOut);
}

//==============================================================================
std::string JackClientGris::getClientName(const char * port) const
{
    if (port) {
        jack_port_t * tt = jack_port_by_name(this->mClient, port);
        if (tt) {
            std::string nameClient = jack_port_name(tt);
            std::string tempN = jack_port_short_name(tt);
            return nameClient.substr(0, nameClient.size() - (tempN.size() + 1));
        }
    }
    return "";
}

//==============================================================================
void JackClientGris::updateClientPortAvailable(bool fromJack)
{
    const char ** portsOut = jack_get_ports(this->mClient, NULL, JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput);
    int i = 0;

    for (auto && cli : this->mClients) {
        cli.portAvailable = 0;
    }

    while (portsOut[i]) {
        std::string nameCli = getClientName(portsOut[i]);
        if (nameCli != CLIENT_NAME && nameCli != SYS_CLIENT_NAME) {
            for (auto && cli : this->mClients) {
                if (cli.name.compare(nameCli) == 0) {
                    cli.portAvailable += 1;
                }
            }
        }
        i++;
    }

    unsigned int start = 1;
    unsigned int end = 2;
    unsigned int defaultActivePorts = 64;
    for (auto && cli : this->mClients) {
        if (!fromJack) {
            cli.initialized = true;
            end = cli.activePorts;
        } else {
            end = cli.portAvailable < defaultActivePorts ? cli.portAvailable : defaultActivePorts;
        }
        if (cli.portStart == 0 || cli.portEnd == 0 || !cli.initialized) { // ports not initialized.
            cli.portStart = start;
            cli.portEnd = start + end - 1;
            start += end;
        } else if ((cli.portStart >= cli.portEnd)
                   || (cli.portEnd - cli.portStart > cli.portAvailable)) { // portStart bigger than portEnd.
            cli.portStart = start;
            cli.portEnd = start + cli.activePorts - 1;
            start += cli.activePorts;
        } else {
            if (this->mClients.size() > 1) {
                unsigned int pos = 0;
                bool somethingBad = false;
                for (unsigned int c = 0; c < this->mClients.size(); c++) {
                    if (this->mClients[c].name == cli.name) {
                        pos = c;
                        break;
                    }
                }
                if (pos == 0) {
                    somethingBad = false;
                } else if (pos >= this->mClients.size()) {
                    somethingBad = true; // Never supposed to get here.
                } else {
                    if ((cli.portStart - 1) != this->mClients[pos - 1].portEnd) {
                        int numPorts = cli.portEnd - cli.portStart;
                        cli.portStart = this->mClients[pos - 1].portEnd + 1;
                        cli.portEnd = cli.portStart + numPorts;
                    }
                    for (unsigned int k = 0; k < pos; k++) {
                        struct Client clicmp = this->mClients[k];
                        if (clicmp.name != cli.name && cli.portStart > clicmp.portStart
                            && cli.portStart < clicmp.portEnd) {
                            somethingBad = true;
                        } else if (clicmp.name != cli.name && cli.portEnd > clicmp.portStart
                                   && cli.portEnd < clicmp.portEnd) {
                            somethingBad = true;
                        }
                    }
                }

                if (somethingBad) { // ports overlap other client ports.
                    cli.portStart = start;
                    cli.portEnd = start + defaultActivePorts - 1;
                    start += defaultActivePorts;
                } else {
                    // If everything goes right, we keep portStart and portEnd for this client.
                    start = cli.portEnd + 1;
                }
            }
        }
        cli.activePorts = cli.portEnd - cli.portStart + 1;
        if (cli.portStart > this->mInputsPort.size()) {
            // cout << "Not enough inputs, client can't connect!" << " " << cli.portStart << " " <<
            // this->inputsPort.size() << endl;
        }
    }

    jack_free(portsOut);
}

//==============================================================================
JackClientGris::~JackClientGris()
{
    // TODO: this->paramVBap and this->mSourcesIn->paramVBap are never deallocated.

    lbap_field_free(this->mLbapSpeakerField);

    jack_deactivate(this->mClient);
    for (unsigned int i = 0; i < this->mInputsPort.size(); i++) {
        jack_port_unregister(this->mClient, this->mInputsPort[i]);
    }

    for (unsigned int i = 0; i < this->mOutputsPort.size(); i++) {
        jack_port_unregister(this->mClient, this->mOutputsPort[i]);
    }
    jack_client_close(this->mClient);
}
