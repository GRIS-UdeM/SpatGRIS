/*
 This file is part of SpatGRIS2.

 Developers: Samuel B�land, Olivier B�langer, Nicolas Masson

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

#include "JackClient.h"

#include <array>
#include <cstdarg>

#include "AudioManager.h"
#include "MainComponent.h"
#include "Speaker.h"
#include "constants.hpp"

extern "C" {
#include "spat/vbap.h"
}

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
    auto * jackCli = static_cast<JackClient *>(arg);
    return jackCli->processAudio(nFrames);
}

//==============================================================================
// Jack callback functions.
void session_callback(jack_session_event_t * event, void * /*arg*/)
{
    char returnValue[100];
    jack_client_log("session notification\n");
    jack_client_log("path %s, uuid %s, type: %s\n",
                    event->session_dir,
                    event->client_uuid,
                    event->type == JackSessionSave ? "save" : "quit");

    snprintf(returnValue, 100, "jack_simple_session_client %s", event->client_uuid);
    // event->command_line = strdup(returnValue);
}

//==============================================================================
int graphOrderCallback(void * arg)
{
    auto * jackCli = static_cast<JackClient *>(arg);
    jack_client_log("graph_order_callback...\n");
    jackCli->updateClientPortAvailable(true);
    jack_client_log("... done!\n");
    return 0;
}

//==============================================================================
void client_registration_callback(const char * const name, int const registered, void * arg)
{
    jack_client_log("Jack client registration : %s : ", name);
    if (!strcmp(name, CLIENT_NAME_IGNORE)) {
        jack_client_log("ignored\n");
        return;
    }

    auto * jackCli = static_cast<JackClient *>(arg);
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
    auto * jackCli = static_cast<JackClient *>(arg);
    jackCli->portConnectCallback(a, b, connect);
}

//==============================================================================
// Load samples from a wav file into a float array.
static juce::AudioBuffer<float> getSamplesFromWavFile(juce::File const & file)
{
    if (!file.existsAsFile()) {
        auto const error{ file.getFullPathName() + "\n\nTry re-installing SpatGRIS2." };
        juce::AlertWindow::showMessageBox(juce::AlertWindow::AlertIconType::WarningIcon, "Missing file", error);
        std::exit(-1);
    }

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
JackClient::JackClient()
{
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
        std::memcpy(mVbapHrtfLeftImpulses[i].data(), stbuf.getReadPointer(leftChannel), 128);
        std::memcpy(mVbapHrtfRightImpulses[i].data(), stbuf.getReadPointer(rightChannel), 128);
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
        std::memcpy(mVbapHrtfLeftImpulses[i].data(), stbuf.getReadPointer(leftChannel), 128);
        std::memcpy(mVbapHrtfRightImpulses[i].data(), stbuf.getReadPointer(rightChannel), 128);
    }
    // Azimuth = 80
    for (int i = 0; i < 2; i++) {
        auto const file{ HRTF_FOLDER_80.getChildFile("H80e090a.wav") };
        auto const stbuf{ getSamplesFromWavFile(file) };
        auto const leftChannel{ 1 - i };
        auto const rightChannel{ i };
        std::memcpy(mVbapHrtfLeftImpulses[i].data(), stbuf.getReadPointer(leftChannel), 128);
        std::memcpy(mVbapHrtfRightImpulses[i].data(), stbuf.getReadPointer(rightChannel), 128);
    }

    // Initialize STEREO data.
    // Initialize LBAP data.
    mLbapSpeakerField = lbap_field_init();

    mInterMaster = 0.8f;

    // open a client connection to the JACK server. Start server if it is not running.
    jack_options_t options = JackNullOption;
    jack_status_t status;

    jack_client_log("\nStart Jack Client\n");
    jack_client_log("=================\n");

    mClient = jackClientOpen(CLIENT_NAME, options, &status, SYS_DRIVER_NAME);
    if (mClient == NULL) {
        jack_client_log("\nTry again...\n");
        options = JackServerName;
        mClient = jackClientOpen(CLIENT_NAME, options, &status, SYS_DRIVER_NAME);
        if (mClient == NULL) {
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
        CLIENT_NAME = jack_get_client_name(mClient);
        jack_client_log("\n Chosen name already existed, new unique name `%s' assigned\n", CLIENT_NAME);
    }

    // Register Jack callbacks and ports.
    jack_set_process_callback(mClient, process_audio, this);
    jack_set_port_connect_callback(mClient, port_connect_callback, this);

    // Initialize pink noise
    srand(static_cast<unsigned int>(time(nullptr)));

    // Print available inputs ports.
    auto ** ports = jack_get_ports(mClient, nullptr, NULL, JackPortIsInput);
    if (ports == nullptr) {
        jack_client_log("\n No input ports!\n");
        return;
    }
    mNumberInputs = 0;
    jack_client_log("\nInput ports:\n\n");
    while (ports[mNumberInputs]) {
        jack_client_log("%s\n", ports[mNumberInputs]);
        mNumberInputs += 1;
    }
    jack_free(ports);
    jack_client_log("\nNumber of input ports: %d\n\n", mNumberInputs);

    // Print available outputs ports.
    ports = jack_get_ports(mClient, nullptr, nullptr, JackPortIsOutput);
    if (ports == nullptr) {
        jack_client_log("\n No output ports!\n");
        return;
    }
    mNumberOutputs = 0;
    jack_client_log("Output ports:\n\n");
    while (ports[mNumberOutputs]) {
        jack_client_log("%s\n", ports[mNumberOutputs]);
        mNumberOutputs += 1;
    }
    jack_free(ports);
    jack_client_log("\nNumber of output ports: %d\n\n", mNumberOutputs);

    jack_client_log("\nJack Client Run\n");
    jack_client_log("=============== \n");

    mClientReady = true;
}

//==============================================================================
void JackClient::resetHrtf()
{
    std::fill(std::begin(mHrtfCount), std::end(mHrtfCount), 0u);
    static constexpr std::array<float, 128> EMPTY_HRTF_INPUT{};
    std::fill(std::begin(mHrtfInputTmp), std::end(mHrtfInputTmp), EMPTY_HRTF_INPUT);
}

//==============================================================================
void JackClient::clientRegistrationCallback(char const * const name, int const regist)
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
void JackClient::portConnectCallback(jack_port_id_t const a, jack_port_id_t const b, int const connect) const
{
    jack_client_log("Jack port : ");
    if (connect) {
        // Stop Auto connection with system.
        // TODO : crashes happening once in a while here. Probably due to some race conditions between AudioManager
        // ports and the audio callbacks.
        if (!mAutoConnection) {
            std::string nameClient = jack_port_name(jack_port_by_id(mClient, a));
            std::string tempN = jack_port_short_name(jack_port_by_id(mClient, a));
            nameClient = nameClient.substr(0, nameClient.size() - (tempN.size() + 1));
        }
        jack_client_log("Connect ");
    } else {
        jack_client_log("Disconnect ");
    }
    jack_client_log("%d <> %d \n", a, b);
}

//==============================================================================
void JackClient::prepareToRecord()
{
    int num_of_channels;
    if (mOutputsPort.size() < 1) {
        return;
    }

    mIsRecording = false;
    mIndexRecord = 0;
    mOutputFileNames.clear();

    juce::String channelName;
    juce::File fileS = juce::File(mRecordPath);
    juce::String fname = fileS.getFileNameWithoutExtension();
    juce::String extF = fileS.getFileExtension();
    juce::String parent = fileS.getParentDirectory().getFullPathName();

    auto * currentAudioDevice{ AudioManager::getInstance().getAudioDeviceManager().getCurrentAudioDevice() };
    jassert(currentAudioDevice);
    auto const sampleRate{ static_cast<unsigned>(std::round(currentAudioDevice->getCurrentSampleRate())) };

    if (mModeSelected == ModeSpatEnum::VBAP || mModeSelected == ModeSpatEnum::LBAP) {
        num_of_channels = (int)mOutputsPort.size();
        for (int i = 0; i < num_of_channels; ++i) {
            if (intVectorContains(mOutputPatches, i + 1)) {
                channelName = parent + "/" + fname + "_" + juce::String(i + 1).paddedLeft('0', 3) + extF;
                juce::File fileC = juce::File(channelName);
                mRecorders[i].startRecording(fileC, sampleRate, extF);
                mOutputFileNames.add(fileC);
            }
        }
    } else if (mModeSelected == ModeSpatEnum::VBAP_HRTF || mModeSelected == ModeSpatEnum::STEREO) {
        num_of_channels = 2;
        for (int i = 0; i < num_of_channels; ++i) {
            channelName = parent + "/" + fname + "_" + juce::String(i + 1).paddedLeft('0', 3) + extF;
            juce::File fileC = juce::File(channelName);
            mRecorders[i].startRecording(fileC, sampleRate, extF);
            mOutputFileNames.add(fileC);
        }
    }
}

//==============================================================================
void JackClient::startRecord()
{
    mIndexRecord = 0;
    mIsRecording = true;
}

//==============================================================================
void JackClient::addRemoveInput(unsigned int number)
{
    if (number < mInputsPort.size()) {
        while (number < mInputsPort.size()) {
            jack_port_unregister(mClient, mInputsPort.back());
            mInputsPort.pop_back();
        }
    } else {
        while (number > mInputsPort.size()) {
            juce::String nameIn = "input";
            nameIn += juce::String(mInputsPort.size() + 1);
            jack_port_t * newPort = jack_port_register(mClient,
                                                       nameIn.toStdString().c_str(),
                                                       JACK_DEFAULT_AUDIO_TYPE,
                                                       JackPortIsInput,
                                                       0);
            mInputsPort.push_back(newPort);
        }
    }
    connectedGrisToSystem();
}

//==============================================================================
void JackClient::clearOutput()
{
    int outS = (int)mOutputsPort.size();
    for (int i = 0; i < outS; i++) {
        jack_port_unregister(mClient, mOutputsPort.back());
        mOutputsPort.pop_back();
    }
}

//==============================================================================
bool JackClient::addOutput(unsigned int outputPatch)
{
    if (outputPatch > mMaxOutputPatch)
        mMaxOutputPatch = outputPatch;
    juce::String nameOut = "output";
    nameOut += juce::String(mOutputsPort.size() + 1);

    jack_port_t * newPort = jack_port_register(mClient, nameOut.toUTF8(), JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);
    mOutputsPort.push_back(newPort);
    connectedGrisToSystem();
    return true;
}

//==============================================================================
void JackClient::removeOutput(int number)
{
    jack_port_unregister(mClient, mOutputsPort.at(number));
    mOutputsPort.erase(mOutputsPort.begin() + number);
}

//==============================================================================
std::vector<int> JackClient::getDirectOutOutputPatches() const
{
    std::vector<int> directOutOutputPatches;
    for (auto const & it : mSpeakersOut) {
        if (it.directOut && it.outputPatch != 0)
            directOutOutputPatches.push_back(it.outputPatch);
    }
    return directOutOutputPatches;
}

//==============================================================================
void JackClient::muteSoloVuMeterIn(jack_default_audio_sample_t ** ins,
                                   jack_nframes_t const nFrames,
                                   unsigned const sizeInputs)
{
    for (unsigned int i = 0; i < sizeInputs; ++i) {
        if (mSourcesIn[i].isMuted) { // Mute
            memset(ins[i], 0, sizeof(jack_default_audio_sample_t) * nFrames);
        } else if (mSoloIn) { // Solo
            if (!mSourcesIn[i].isSolo) {
                memset(ins[i], 0, sizeof(jack_default_audio_sample_t) * nFrames);
            }
        }

        // VuMeter
        auto maxGain = 0.0f;
        for (unsigned int j = 1; j < nFrames; j++) {
            auto const absGain = fabsf(ins[i][j]);
            if (absGain > maxGain)
                maxGain = absGain;
        }
        mLevelsIn[i] = maxGain;
    }
}

//==============================================================================
void JackClient::muteSoloVuMeterGainOut(jack_default_audio_sample_t ** outs,
                                        jack_nframes_t const nFrames,
                                        unsigned const sizeOutputs,
                                        float const gain)
{
    unsigned int num_of_channels = 2;
    double inval = 0.0, val = 0.0;

    if (mModeSelected == ModeSpatEnum::VBAP || mModeSelected == ModeSpatEnum::LBAP) {
        num_of_channels = sizeOutputs;
    }

    for (unsigned int i = 0; i < sizeOutputs; ++i) {
        if (mSpeakersOut[i].isMuted) { // Mute
            memset(outs[i], 0, sizeof(jack_default_audio_sample_t) * nFrames);
        } else if (mSoloOut) { // Solo
            if (!mSpeakersOut[i].isSolo) {
                memset(outs[i], 0, sizeof(jack_default_audio_sample_t) * nFrames);
            }
        }

        // Speaker independent gain.
        auto const speakerGain = mSpeakersOut[i].gain;
        for (unsigned int f = 0; f < nFrames; ++f) {
            outs[i][f] *= speakerGain * gain;
        }

        // Speaker independent crossover filter.
        if (mSpeakersOut[i].hpActive) {
            SpeakerOut so = mSpeakersOut[i];
            for (unsigned int f = 0; f < nFrames; ++f) {
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
        for (unsigned int j = 1; j < nFrames; j++) {
            float absGain = fabsf(outs[i][j]);
            if (absGain > maxGain)
                maxGain = absGain;
        }
        mLevelsOut[i] = maxGain;

        // Record buffer.
        if (mIsRecording) {
            if (num_of_channels == sizeOutputs && i < num_of_channels) {
                if (intVectorContains(mOutputPatches, i + 1)) {
                    mRecorders[i].recordSamples(&outs[i], (int)nFrames);
                }
            } else if (num_of_channels == 2 && i < num_of_channels) {
                mRecorders[i].recordSamples(&outs[i], (int)nFrames);
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
        mIndexRecord += nFrames;
    }
}

//==============================================================================
void JackClient::addNoiseSound(jack_default_audio_sample_t ** outs,
                               jack_nframes_t const nFrames,
                               unsigned const sizeOutputs)
{
    float rnd;
    float val;
    float const fac = 1.0f / (static_cast<float>(RAND_MAX) / 2.0f);
    for (unsigned int nF = 0; nF < nFrames; ++nF) {
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
void JackClient::processVbap(jack_default_audio_sample_t ** ins,
                             jack_default_audio_sample_t ** outs,
                             jack_nframes_t const nFrames,
                             unsigned const sizeInputs,
                             unsigned const sizeOutputs)
{
    auto interpolationG{ mInterMaster == 0.0f ? 0.99f : std::pow(mInterMaster, 0.1f) * 0.0099f + 0.99f };

    for (unsigned i{}; i < sizeInputs; ++i) {
        if (mVbapSourcesToUpdate[i] == 1) {
            updateSourceVbap(static_cast<int>(i));
            mVbapSourcesToUpdate[i] = 0;
        }
    }

    for (unsigned o{}; o < sizeOutputs; ++o) {
        std::fill(outs[o], outs[o] + nFrames, 0.0f);
        for (unsigned i{}; i < sizeInputs; ++i) {
            if (!mSourcesIn[i].directOut && mSourcesIn[i].paramVBap != nullptr) {
                auto const ioGain{ mSourcesIn[i].paramVBap->gains[o] };
                auto y{ mSourcesIn[i].paramVBap->y[o] };
                if (mInterMaster == 0.0f) {
                    interpolationG = (ioGain - y) / nFrames;
                    for (unsigned f{}; f < nFrames; ++f) {
                        y += interpolationG;
                        outs[o][f] += ins[i][f] * y;
                    }
                } else {
                    for (unsigned f{}; f < nFrames; ++f) {
                        y = ioGain + (y - ioGain) * interpolationG;
                        if (y < 0.0000000000001f) {
                            y = 0.0;
                        } else {
                            outs[o][f] += ins[i][f] * y;
                        }
                    }
                }
                mSourcesIn[i].paramVBap->y[o] = y;
            } else if (static_cast<unsigned int>(mSourcesIn[i].directOut - 1) == o) {
                std::transform(outs[o], outs[o] + nFrames, ins[i], outs[o], std::plus());
            }
        }
    }
}

//==============================================================================
void JackClient::processLbap(jack_default_audio_sample_t ** ins,
                             jack_default_audio_sample_t ** outs,
                             jack_nframes_t const nFrames,
                             unsigned const sizeInputs,
                             unsigned const sizeOutputs)
{
    static auto constexpr MAX_N_FRAMES{ 2048u };
    jassert(nFrames <= MAX_N_FRAMES);
    std::array<float, MAX_N_FRAMES> filteredInputSignal{};

    auto interpolationG{ mInterMaster == 0.0f ? 0.99f : std::pow(mInterMaster, 0.1f) * 0.0099f + 0.99f };

    // TODO : is this necessary ?
    // std::for_each(outs, outs + sizeOutputs, [nFrames](float * it) { std::fill(it, it + nFrames, 0.0f); });

    for (unsigned i{}; i < sizeInputs; ++i) {
        auto & sourceIn{ mSourcesIn[i] };
        if (!sourceIn.directOut) {
            lbap_pos pos;
            lbap_pos_init_from_radians(&pos, sourceIn.radAzimuth, sourceIn.radElevation, sourceIn.radius);
            pos.radspan = sourceIn.azimuthSpan;
            pos.elespan = sourceIn.zenithSpan;
            auto distance{ sourceIn.radius };
            if (!lbap_pos_compare(&pos, &sourceIn.lbapLastPos)) {
                lbap_field_compute(mLbapSpeakerField, &pos, sourceIn.lbapGains.data());
                lbap_pos_copy(&sourceIn.lbapLastPos, &pos);
            }

            float distanceGain;
            float distanceCoefficient;
            // Energy lost with distance, radius is in the range 0 - 2.6 (>1 is beyond HP circle).
            if (distance < 1.0f) {
                distanceGain = 1.0f;
                distanceCoefficient = 0.0f;
            } else {
                distance = std::min((distance - 1.0f) * 1.25f, 1.0f);
                distanceGain = (1.0f - distance) * (1.0f - mAttenuationLinearGain) + mAttenuationLinearGain;
                distanceCoefficient = distance * mAttenuationLowpassCoefficient;
            }
            auto const diffGain{ (distanceGain - mLastAttenuationGain[i]) / static_cast<float>(nFrames) };
            auto const diffCoefficient
                = (distanceCoefficient - mLastAttenuationCoefficient[i]) / static_cast<float>(nFrames);
            auto filterInY{ mAttenuationLowpassY[i] };
            auto filterInZ{ mAttenuationLowpassZ[i] };
            auto lastCoefficient{ mLastAttenuationCoefficient[i] };
            auto lastGain{ mLastAttenuationGain[i] };
            for (unsigned k{}; k < nFrames; ++k) {
                lastCoefficient += diffCoefficient;
                lastGain += diffGain;
                filterInY = ins[i][k] + (filterInY - ins[i][k]) * lastCoefficient;
                filterInZ = filterInY + (filterInZ - filterInY) * lastCoefficient;
                filteredInputSignal[k] = filterInZ * lastGain;
            }
            mAttenuationLowpassY[i] = filterInY;
            mAttenuationLowpassZ[i] = filterInZ;
            mLastAttenuationGain[i] = distanceGain;
            mLastAttenuationCoefficient[i] = distanceCoefficient;
            //==============================================================================
            for (unsigned o{}; o < sizeOutputs; ++o) {
                auto const gain{ mSourcesIn[i].lbapGains[o] };
                auto y{ mSourcesIn[i].lbapY[o] };
                if (mInterMaster == 0.0f) {
                    interpolationG = (gain - y) / nFrames;
                    for (unsigned f{}; f < nFrames; ++f) {
                        y += interpolationG;
                        outs[o][f] += filteredInputSignal[f] * y;
                    }
                } else {
                    // TODO : this is where 98% of the process takes place. Is it just a ramp? Optimize.
                    for (unsigned f{}; f < nFrames; ++f) {
                        y = (y - gain) * interpolationG + gain;
                        if (y < 0.0000000000001f) {
                            y = 0.0f;
                        } else {
                            outs[o][f] += filteredInputSignal[f] * y;
                        }
                    }
                }
                mSourcesIn[i].lbapY[o] = y;
            }
        } else {
            for (unsigned o{}; o < sizeOutputs; ++o) {
                if (static_cast<unsigned>(mSourcesIn[i].directOut - 1) == o) {
                    for (unsigned f{}; f < nFrames; ++f) {
                        outs[o][f] += ins[i][f];
                    }
                }
            }
        }
    }
}

//==============================================================================
void JackClient::processVBapHrtf(jack_default_audio_sample_t ** ins,
                                 jack_default_audio_sample_t ** outs,
                                 jack_nframes_t const nFrames,
                                 unsigned const sizeInputs,
                                 [[maybe_unused]] unsigned const sizeOutputs)
{
    // TODO : not shure this is necessary.
    // std::for_each(outs, outs + sizeOutputs, [nFrames](float * channel) {
    //    std::fill(channel, channel + nFrames, 0.0f);
    //});

    for (unsigned i{}; i < sizeInputs; ++i) {
        if (mVbapSourcesToUpdate[i] == 1) {
            updateSourceVbap(static_cast<int>(i));
            mVbapSourcesToUpdate[i] = 0;
        }
    }

    auto interpolationG{ mInterMaster == 0.0f ? 0.99f : std::pow(mInterMaster, 0.1f) * 0.0099f + 0.99f };
    for (unsigned o{}; o < 16; ++o) {
        std::array<float, 2048> vbapOuts{};
        for (unsigned i{}; i < sizeInputs; ++i) {
            if (!mSourcesIn[i].directOut && mSourcesIn[i].paramVBap != nullptr) {
                auto const ioGain{ mSourcesIn[i].paramVBap->gains[o] };
                auto y{ mSourcesIn[i].paramVBap->y[o] };
                if (mInterMaster == 0.0f) {
                    interpolationG = (ioGain - y) / nFrames;
                    for (unsigned f{}; f < nFrames; ++f) {
                        y += interpolationG;
                        vbapOuts[f] += ins[i][f] * y;
                    }
                } else {
                    for (unsigned f{}; f < nFrames; ++f) {
                        y = ioGain + (y - ioGain) * interpolationG;
                        if (y < 0.0000000000001f) {
                            y = 0.0f;
                        } else {
                            vbapOuts[f] += ins[i][f] * y;
                        }
                    }
                }
                mSourcesIn[i].paramVBap->y[o] = y;
            }
        }

        for (unsigned f{}; f < nFrames; f++) {
            auto tmpCount{ static_cast<int>(mHrtfCount[o]) };
            for (unsigned k{}; k < 128; ++k) {
                if (tmpCount < 0) {
                    tmpCount += 128;
                }
                auto const sig{ mHrtfInputTmp[o][tmpCount] };
                outs[0][f] += sig * mVbapHrtfLeftImpulses[o][k];
                outs[1][f] += sig * mVbapHrtfRightImpulses[o][k];
                --tmpCount;
            }
            mHrtfCount[o]++;
            if (mHrtfCount[o] >= 128) {
                mHrtfCount[o] = 0;
            }
            mHrtfInputTmp[o][mHrtfCount[o]] = vbapOuts[f];
        }
    }

    // Add direct outs to the now stereo signal.
    for (unsigned i{}; i < sizeInputs; ++i) {
        if (mSourcesIn[i].directOut != 0) {
            if (mSourcesIn[i].directOut % 2 == 1) {
                for (unsigned f{}; f < nFrames; ++f) {
                    outs[0][f] += ins[i][f];
                }
            } else {
                for (unsigned f{}; f < nFrames; ++f) {
                    outs[1][f] += ins[i][f];
                }
            }
        }
    }
}

//==============================================================================
void JackClient::processStereo(jack_default_audio_sample_t ** ins,
                               jack_default_audio_sample_t ** outs,
                               jack_nframes_t const nFrames,
                               unsigned const sizeInputs,
                               unsigned const sizeOutputs)
{
    static auto constexpr FACTOR{ juce::MathConstants<float>::pi / 360.0f };

    auto const interpolationG{ std::pow(mInterMaster, 0.1f) * 0.0099f + 0.99f };
    auto const gain{ std::pow(10.0f, (static_cast<float>(sizeInputs) - 1.0f) * -0.1f * 0.05f) };

    for (unsigned i{}; i < sizeOutputs; ++i) {
        memset(outs[i], 0, sizeof(jack_default_audio_sample_t) * nFrames);
    }

    for (unsigned i{}; i < sizeInputs; ++i) {
        if (!mSourcesIn[i].directOut) {
            auto const azimuth{ mSourcesIn[i].azimuth };
            auto lastAzimuth{ mLastAzimuth[i] };
            for (unsigned f{}; f < nFrames; ++f) {
                // Removes the chirp at 180->-180 degrees azimuth boundary.
                if (std::abs(lastAzimuth - azimuth) > 300.0f) {
                    lastAzimuth = azimuth;
                }
                lastAzimuth = azimuth + (lastAzimuth - azimuth) * interpolationG;
                float scaled;
                if (lastAzimuth < -90.0f) {
                    scaled = -90.0f - (lastAzimuth + 90.0f);
                } else if (lastAzimuth > 90) {
                    scaled = 90.0f - (lastAzimuth - 90.0f);
                } else {
                    scaled = lastAzimuth;
                }
                scaled = (scaled + 90) * FACTOR;
                outs[0][f] += ins[i][f] * std::cos(scaled);
                outs[1][f] += ins[i][f] * std::sin(scaled);
            }
            mLastAzimuth[i] = lastAzimuth;

        } else if (mSourcesIn[i].directOut % 2 == 1) {
            for (unsigned f{}; f < nFrames; ++f) {
                outs[0][f] += ins[i][f];
            }
        } else {
            for (unsigned f{}; f < nFrames; ++f) {
                outs[1][f] += ins[i][f];
            }
        }
    }
    // Apply gain compensation.
    for (unsigned f{}; f < nFrames; ++f) {
        outs[0][f] *= gain;
        outs[1][f] *= gain;
    }
}

//==============================================================================
int JackClient::processAudio(jack_nframes_t const nFrames)
{
    // Return if the user is editing the speaker setup.
    if (!mProcessBlockOn) {
        for (unsigned int i = 0; i < mOutputsPort.size(); ++i) {
            memset(((jack_default_audio_sample_t *)jack_port_get_buffer(mOutputsPort[i], nFrames)),
                   0,
                   sizeof(jack_default_audio_sample_t) * nFrames);
            mLevelsOut[i] = 0.0f;
        }
        return 0;
    }

    const unsigned int sizeInputs = static_cast<unsigned int>(mInputsPort.size());
    const unsigned int sizeOutputs = static_cast<unsigned int>(mOutputsPort.size());

    jack_default_audio_sample_t * ins[MAX_INPUTS];
    jack_default_audio_sample_t * outs[MAX_OUTPUTS];

    for (unsigned int i = 0; i < sizeInputs; i++) {
        ins[i] = (jack_default_audio_sample_t *)jack_port_get_buffer(mInputsPort[i], nFrames);
    }
    for (unsigned int i = 0; i < sizeOutputs; i++) {
        outs[i] = (jack_default_audio_sample_t *)jack_port_get_buffer(mOutputsPort[i], nFrames);
    }

    muteSoloVuMeterIn(ins, nFrames, sizeInputs);

    switch (mModeSelected) {
    case ModeSpatEnum::VBAP:
        processVbap(ins, outs, nFrames, sizeInputs, sizeOutputs);
        break;
    case ModeSpatEnum::LBAP:
        processLbap(ins, outs, nFrames, sizeInputs, sizeOutputs);
        break;
    case ModeSpatEnum::VBAP_HRTF:
        processVBapHrtf(ins, outs, nFrames, sizeInputs, sizeOutputs);
        break;
    case ModeSpatEnum::STEREO:
        processStereo(ins, outs, nFrames, sizeInputs, sizeOutputs);
        break;
    default:
        jassertfalse;
        break;
    }

    if (mPinkNoiseActive) {
        addNoiseSound(outs, nFrames, sizeOutputs);
    }

    muteSoloVuMeterGainOut(outs, nFrames, sizeOutputs, mMasterGainOut);

    mIsOverloaded = false;

    return 0;
}

//==============================================================================
void JackClient::connectedGrisToSystem()
{
    juce::String nameOut;
    clearOutput();
    for (unsigned int i = 0; i < mMaxOutputPatch; i++) {
        nameOut = "output";
        nameOut += juce::String(mOutputsPort.size() + 1);
        jack_port_t * newPort
            = jack_port_register(mClient, nameOut.toUTF8(), JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);
        mOutputsPort.push_back(newPort);
    }

    const char ** portsOut = jack_get_ports(mClient, NULL, JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput);
    const char ** portsIn = jack_get_ports(mClient, NULL, JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput);

    int i = 0;
    int j = 0;

    // DisConnect JackClientGris to system.
    while (portsOut[i]) {
        if (getClientName(portsOut[i]) == CLIENT_NAME) { // jackClient
            j = 0;
            while (portsIn[j]) {
                if (getClientName(portsIn[j]) == SYS_CLIENT_NAME && // system
                    jack_port_connected_to(jack_port_by_name(mClient, portsOut[i]), portsIn[j])) {
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
                    jack_connect(mClient, portsOut[i], portsIn[j]);
                    j += 1;
                    break;
                }
                j += 1;
            }
        }
        i += 1;
    }

    // Build output patch list.
    mOutputPatches.clear();
    for (i = 0; i < mOutputsPort.size(); i++) {
        if (mSpeakersOut[i].outputPatch != 0) {
            mOutputPatches.push_back(mSpeakersOut[i].outputPatch);
        }
    }

    jack_free(portsIn);
    jack_free(portsOut);
}

//==============================================================================
bool JackClient::initSpeakersTriplet(std::vector<Speaker *> const & listSpk, int dimensions, bool needToComputeVbap)
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
        mParamVBap
            = init_vbap_from_speakers(lss, (int)listSpk.size(), dimensions, outputPatches, mMaxOutputPatch, NULL);
        if (mParamVBap == NULL) {
            return false;
        }
    }

    for (unsigned int i = 0; i < MAX_INPUTS; i++) {
        mSourcesIn[i].paramVBap = copy_vbap_data(mParamVBap);
    }

    int ** triplets;
    int num = vbap_get_triplets(mSourcesIn[0].paramVBap, &triplets);
    mVbapTriplets.clear();
    for (int i = 0; i < num; i++) {
        std::vector<int> row;
        for (j = 0; j < 3; j++) {
            row.push_back(triplets[i][j]);
        }
        mVbapTriplets.push_back(row);
    }

    for (int i = 0; i < num; i++) {
        free(triplets[i]);
    }
    free(triplets);

    connectedGrisToSystem();

    return true;
}

//==============================================================================
bool JackClient::lbapSetupSpeakerField(std::vector<Speaker *> const & listSpk)
{
    int j;
    if (listSpk.size() <= 0) {
        return false;
    }

    float azimuth[MAX_OUTPUTS];
    float elevation[MAX_OUTPUTS];
    float radius[MAX_OUTPUTS];
    int outputPatch[MAX_OUTPUTS];

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

    lbap_field_reset(mLbapSpeakerField);
    lbap_field_setup(mLbapSpeakerField, speakers, (int)listSpk.size());

    free(speakers);

    connectedGrisToSystem();

    return true;
}

//==============================================================================
void JackClient::setAttenuationDb(float const value)
{
    mAttenuationLinearGain = value;
}

//==============================================================================
void JackClient::setAttenuationHz(float const value)
{
    mAttenuationLowpassCoefficient = value;
}

//==============================================================================
void JackClient::updateSourceVbap(int const idS)
{
    if (mVbapDimensions == 3) {
        if (mSourcesIn[idS].paramVBap != nullptr) {
            vbap2_flip_y_z(mSourcesIn[idS].azimuth,
                           mSourcesIn[idS].zenith,
                           mSourcesIn[idS].azimuthSpan,
                           mSourcesIn[idS].zenithSpan,
                           mSourcesIn[idS].paramVBap);
        }
    } else if (mVbapDimensions == 2) {
        if (mSourcesIn[idS].paramVBap != nullptr) {
            vbap2(mSourcesIn[idS].azimuth, 0.0, mSourcesIn[idS].azimuthSpan, 0.0, mSourcesIn[idS].paramVBap);
        }
    }
}

//==============================================================================
void JackClient::connectionClient(juce::String const & name, bool connect)
{
    const char ** portsOut = jack_get_ports(mClient, NULL, JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput);
    const char ** portsIn = jack_get_ports(mClient, NULL, JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput);

    int i = 0;
    int j = 0;
    int startJ = 0;
    int endJ = 0;
    bool conn = false;
    updateClientPortAvailable(false);

    // Disconnect client.
    while (portsOut[i]) {
        if (getClientName(portsOut[i]) == name) {
            j = 0;
            while (portsIn[j]) {
                if (getClientName(portsIn[j]) == CLIENT_NAME && // jackClient
                    jack_port_connected_to(jack_port_by_name(mClient, portsOut[i]), portsIn[j])) {
                }
                j += 1;
            }
        }
        i += 1;
    }

    for (auto && cli : mClients) {
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
    mAutoConnection = true;

    for (auto && cli : mClients) {
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
                            jack_connect(mClient, portsOut[i], portsIn[j]);
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

    mAutoConnection = false;

    jack_free(portsIn);
    jack_free(portsOut);
}

//==============================================================================
std::string JackClient::getClientName(const char * port) const
{
    if (port) {
        jack_port_t * tt = jack_port_by_name(mClient, port);
        if (tt) {
            std::string nameClient = jack_port_name(tt);
            std::string tempN = jack_port_short_name(tt);
            return nameClient.substr(0, nameClient.size() - (tempN.size() + 1));
        }
    }
    return "";
}

//==============================================================================
void JackClient::updateClientPortAvailable(bool fromJack)
{
    const char ** portsOut = jack_get_ports(mClient, NULL, JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput);
    int i = 0;

    for (auto && cli : mClients) {
        cli.portAvailable = 0;
    }

    while (portsOut[i]) {
        std::string nameCli = getClientName(portsOut[i]);
        if (nameCli != CLIENT_NAME && nameCli != SYS_CLIENT_NAME) {
            for (auto && cli : mClients) {
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
    for (auto && cli : mClients) {
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
            if (mClients.size() > 1) {
                unsigned int pos = 0;
                bool somethingBad = false;
                for (unsigned int c = 0; c < mClients.size(); c++) {
                    if (mClients[c].name == cli.name) {
                        pos = c;
                        break;
                    }
                }
                if (pos == 0) {
                    somethingBad = false;
                } else if (pos >= mClients.size()) {
                    somethingBad = true; // Never supposed to get here.
                } else {
                    if ((cli.portStart - 1) != mClients[pos - 1].portEnd) {
                        int numPorts = cli.portEnd - cli.portStart;
                        cli.portStart = mClients[pos - 1].portEnd + 1;
                        cli.portEnd = cli.portStart + numPorts;
                    }
                    for (unsigned int k = 0; k < pos; k++) {
                        struct Client clicmp = mClients[k];
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
        if (cli.portStart > mInputsPort.size()) {
            // cout << "Not enough inputs, client can't connect!" << " " << cli.portStart << " " <<
            // inputsPort.size() << endl;
        }
    }

    jack_free(portsOut);
}

//==============================================================================
JackClient::~JackClient()
{
    // TODO: paramVBap and mSourcesIn->paramVBap are never deallocated.

    lbap_field_free(mLbapSpeakerField);

    jack_deactivate(mClient);
    for (unsigned int i = 0; i < mInputsPort.size(); i++) {
        jack_port_unregister(mClient, mInputsPort[i]);
    }

    for (unsigned int i = 0; i < mOutputsPort.size(); i++) {
        jack_port_unregister(mClient, mOutputsPort[i]);
    }
}
