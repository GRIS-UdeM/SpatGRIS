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

#pragma once

static_assert(!USE_JACK);

#include <array>
#include <cstdint>
#include <mutex>
#include <vector>

#include "macros.h"

DISABLE_WARNINGS

#include <JuceHeader.h>

#include "JackMockup.h"

#include "spat/lbap.h"
#include "spat/vbap.h"
ENABLE_WARNINGS

#include "AudioRecorder.h"

class Speaker;

// Limits of SpatGRIS2 In/Out.
static unsigned int const MAX_INPUTS = 256;
static unsigned int const MAX_OUTPUTS = 256;

//==============================================================================
struct LbapData {
    lbap_pos pos;
    std::array<float, MAX_OUTPUTS> gains;
    std::array<float, MAX_OUTPUTS> y;
};

//==============================================================================
struct Client {
    juce::String name;
    unsigned int portStart = 0;
    unsigned int portEnd = 0;
    unsigned int portAvailable = 0;
    unsigned int activePorts = 0;
    bool initialized = false;
    bool connected = false;
};

//==============================================================================
struct SourceIn {
    unsigned int id;
    float x{};
    float y{};
    float z{};

    float radAzimuth{};
    float radElevation{};
    float azimuth{};
    float zenith{};
    float radius{ 1.0f };
    float azimuthSpan{};
    float zenithSpan{};

    std::array<float, MAX_OUTPUTS> lbapGains{};
    std::array<float, MAX_OUTPUTS> lbapY{};
    lbap_pos lbapLastPos{ -1, -1, -1, 0.0f, 0.0f, 0.0f };

    bool isMuted = false;
    bool isSolo = false;
    float gain{}; // Not used yet.

    int directOut{};

    VBAP_DATA * paramVBap{};
};

//==============================================================================
struct SpeakerOut {
    unsigned int id;
    float x{};
    float y{};
    float z{};

    float azimuth{};
    float zenith{};
    float radius{};

    float gain{ 1.0f };

    bool hpActive = false;
    double b1{};
    double b2{};
    double b3{};
    double b4{};
    double ha0{};
    double ha1{};
    double ha2{};

    bool isMuted = false;
    bool isSolo = false;

    int outputPatch{};

    bool directOut = false;
};

//==============================================================================
// Spatialization modes.
enum class ModeSpatEnum { VBAP = 0, LBAP, VBAP_HRTF, STEREO };

//==============================================================================
class JackClient
{
    // class variables.
    //-----------------
    unsigned int mNumberInputs{};
    unsigned int mNumberOutputs{};
    unsigned int mMaxOutputPatch{};

    std::vector<int> mOutputPatches{};

    // Jack variables.
    jack_client_t * mClient{};

    std::vector<jack_port_t *> mInputsPort{};
    std::vector<jack_port_t *> mOutputsPort{};

    // Interpolation and master gain values.
    float mInterMaster{ 0.8f };
    float mMasterGainOut{ 1.0f };

    // Global solo states.
    bool mSoloIn{ false };
    bool mSoloOut{ false };

    // Pink noise test sound.
    float mPinkNoiseC0{};
    float mPinkNoiseC1{};
    float mPinkNoiseC2{};
    float mPinkNoiseC3{};
    float mPinkNoiseC4{};
    float mPinkNoiseC5{};
    float mPinkNoiseC6{};
    float mPinkNoiseGain{ 0.1f };
    bool mPinkNoiseActive{ false };

    // Crossover highpass filter.
    std::array<double, MAX_OUTPUTS> mCrossoverHighpassX1{};
    std::array<double, MAX_OUTPUTS> mCrossoverHighpassX2{};
    std::array<double, MAX_OUTPUTS> mCrossoverHighpassX3{};
    std::array<double, MAX_OUTPUTS> mCrossoverHighpassX4{};
    std::array<double, MAX_OUTPUTS> mCrossoverHighpassY1{};
    std::array<double, MAX_OUTPUTS> mCrossoverHighpassY2{};
    std::array<double, MAX_OUTPUTS> mCrossoverHighpassY3{};
    std::array<double, MAX_OUTPUTS> mCrossoverHighpassY4{};

    // Mute / Solo / VuMeter.
    std::array<float, MAX_INPUTS> mLevelsIn{};
    std::array<float, MAX_OUTPUTS> mLevelsOut{};

    // Client list.
    std::vector<Client> mClients{};
    std::mutex mClientsLock{};

    // Source and output lists.
    std::array<SourceIn, MAX_INPUTS> mSourcesIn{};
    std::array<SpeakerOut, MAX_OUTPUTS> mSpeakersOut{};

    // Enable/disable jack process callback.
    bool mProcessBlockOn{ true };

    // True when jack reports an xrun.
    bool mIsOverloaded{ false };

    // Which spatialization mode is selected.
    ModeSpatEnum mModeSelected{ ModeSpatEnum::VBAP };

    bool mAutoConnection{ false }; // not sure this one is necessary ?

    // VBAP data.
    unsigned mVbapDimensions{};
    std::array<int, MAX_INPUTS> mVbapSourcesToUpdate{};

    std::vector<std::vector<int>> mVbapTriplets{};

    // BINAURAL data.
    std::array<unsigned, 16> mHrtfCount{};
    std::array<std::array<float, 128>, 16> mHrtfInputTmp{};
    std::array<std::array<float, 128>, 16> mVbapHrtfLeftImpulses{};
    std::array<std::array<float, 128>, 16> mVbapHrtfRightImpulses{};

    // STEREO data.
    std::array<float, MAX_INPUTS> mLastAzimuth{};

    // LBAP data.
    lbap_field * mLbapSpeakerField{};

    // Recording parameters.
    size_t mIndexRecord{};
    bool mIsRecording{ false };

    std::array<AudioRecorder, MAX_OUTPUTS> mRecorders{};
    juce::Array<juce::File> mOutputFileNames{};

    // LBAP distance attenuation values.
    float mAttenuationLinearGain{ 0.01584893f };       // -36 dB;
    float mAttenuationLowpassCoefficient{ 0.867208f }; // 1000 Hz
    std::array<float, MAX_INPUTS> mLastAttenuationGain{};
    std::array<float, MAX_INPUTS> mLastAttenuationCoefficient{};
    std::array<float, MAX_INPUTS> mAttenuationLowpassY{};
    std::array<float, MAX_INPUTS> mAttenuationLowpassZ{};
    //==============================================================================
    // Tells if an error occured while setting up the client.
    bool mClientReady{ false };

    // Private recording parameters.
    int mRecordFormat{};     // 0 = WAV, 1 = AIFF
    int mRecordFileConfig{}; // 0 = Multiple Mono Files, 1 = Single Interleaved

    juce::String mRecordPath{};

    // This structure is used to compute the VBAP algorithm only once. Each source only gets a copy.
    VBAP_DATA * mParamVBap{};

public:
    //==============================================================================
    // Class methods.
    JackClient();
    ~JackClient();

    JackClient(JackClient const &) = delete;
    JackClient(JackClient &&) = delete;

    JackClient & operator=(JackClient const &) = delete;
    JackClient & operator=(JackClient &&) = delete;
    //==============================================================================
    // Audio Status.
    [[nodiscard]] bool isReady() const { return mClientReady; }
    [[nodiscard]] float getCpuUsed() const { return jack_cpu_load(mClient); }
    [[nodiscard]] float getLevelsIn(int const index) const { return mLevelsIn[index]; }
    [[nodiscard]] float getLevelsOut(int const index) const { return mLevelsOut[index]; }

    // Manage Inputs / Outputs.
    void addRemoveInput(unsigned int number);
    void clearOutput();
    [[nodiscard]] bool addOutput(unsigned int outputPatch);
    void removeOutput(int number);

    [[nodiscard]] std::vector<int> getDirectOutOutputPatches() const;

    // Manage clients.
    void connectionClient(juce::String const & name, bool connect = true);
    void updateClientPortAvailable(bool fromJack);

    [[nodiscard]] std::string getClientName(char const * port) const;

    // Recording.
    void prepareToRecord();
    void startRecord();
    void stopRecord() { this->mIsRecording = false; }
    void setRecordFormat(const int format) { this->mRecordFormat = format; }
    [[nodiscard]] int getRecordFormat() const { return this->mRecordFormat; }
    void setRecordFileConfig(const int config) { this->mRecordFileConfig = config; }
    [[nodiscard]] int getRecordFileConfig() const { return this->mRecordFileConfig; }
    void setRecordingPath(juce::String const & filePath) { this->mRecordPath = filePath; }
    [[nodiscard]] bool isSavingRun() const { return this->mIsRecording; }

    [[nodiscard]] juce::String const & getRecordingPath() const { return this->mRecordPath; }

    // Initialize VBAP algorithm.
    [[nodiscard]] bool
        initSpeakersTriplet(std::vector<Speaker *> const & listSpk, int dimensions, bool needToComputeVbap);

    // Initialize LBAP algorithm.
    [[nodiscard]] bool lbapSetupSpeakerField(std::vector<Speaker *> const & listSpk);

    // LBAP distance attenuation functions.
    void setAttenuationDb(float value);
    void setAttenuationHz(float value);

    // Need to update a source VBAP data.
    void updateSourceVbap(int idS);

    // Reinit HRTF delay lines.
    void resetHrtf();

    [[nodiscard]] jack_client_t * getClient() { return mClient; }
    void clientRegistrationCallback(char const * name, int regist);
    void portConnectCallback(jack_port_id_t a, jack_port_id_t b, int connect) const;

    [[nodiscard]] unsigned getNumberOutputs() const { return mNumberOutputs; }
    [[nodiscard]] unsigned getNumberInputs() const { return mNumberInputs; }
    [[nodiscard]] ModeSpatEnum getMode() const { return mModeSelected; }
    [[nodiscard]] unsigned getVbapDimensions() const { return mVbapDimensions; }
    [[nodiscard]] auto const & getClients() const { return mClients; }
    [[nodiscard]] auto & getClients() { return mClients; }
    [[nodiscard]] auto & getSourcesIn() { return mSourcesIn; }
    [[nodiscard]] auto const & getSourcesIn() const { return mSourcesIn; }
    [[nodiscard]] auto & getVbapSourcesToUpdate() { return mVbapSourcesToUpdate; }
    [[nodiscard]] auto const & getVbapTriplets() const { return mVbapTriplets; }
    [[nodiscard]] bool getSoloIn() const { return mSoloIn; }
    [[nodiscard]] bool getSoloOut() const { return mSoloOut; }
    [[nodiscard]] auto const & getSpeakersOut() const { return mSpeakersOut; }
    [[nodiscard]] auto & getSpeakersOut() { return mSpeakersOut; }
    [[nodiscard]] size_t getMaxOutputPatch() const { return mMaxOutputPatch; }
    [[nodiscard]] size_t getIndexRecord() const { return mIndexRecord; }
    [[nodiscard]] auto const & getRecorders() const { return mRecorders; }
    [[nodiscard]] auto const & getOutputFileNames() const { return mOutputFileNames; }
    [[nodiscard]] auto const & getInputPorts() const { return mInputsPort; }
    [[nodiscard]] auto & getClientsLock() { return mClientsLock; }

    [[nodiscard]] bool isRecording() const { return mIsRecording; }
    [[nodiscard]] bool isOverloaded() const { return mIsOverloaded; }

    void setProcessBlockOn(bool const state) { mProcessBlockOn = state; }
    void setMaxOutputPatch(unsigned const maxOutputPatch) { mMaxOutputPatch = maxOutputPatch; }
    void setVbapDimensions(unsigned const dimensions) { mVbapDimensions = dimensions; }
    void setSoloIn(bool const state) { mSoloIn = state; }
    void setSoloOut(bool const state) { mSoloOut = state; }
    void setMode(ModeSpatEnum const mode) { mModeSelected = mode; }
    void setMasterGainOut(float const gain) { mMasterGainOut = gain; }
    void setInterMaster(float const interMaster) { mInterMaster = interMaster; }

    //==============================================================================
    // Pink noise
    void setPinkNoiseGain(float const gain) { mPinkNoiseGain = gain; }
    void setPinkNoiseActive(bool const state) { mPinkNoiseActive = state; }

    //==============================================================================
    // Audio processing
    void muteSoloVuMeterIn(jack_default_audio_sample_t ** ins, jack_nframes_t nFrames, size_t sizeInputs);
    void muteSoloVuMeterGainOut(float ** outs, size_t nFrames, size_t sizeOutputs, float gain = 1.0f);
    void addNoiseSound(float ** outs, size_t nFrames, size_t sizeOutputs);
    void processVbap(float ** ins, float ** outs, size_t nFrames, size_t sizeInputs, size_t sizeOutputs);
    void processLbap(float ** ins, float ** outs, size_t nFrames, size_t sizeInputs, size_t sizeOutputs);
    void processVBapHrtf(float ** ins, float ** outs, size_t nFrames, size_t sizeInputs, size_t sizeOutputs);
    void processStereo(float ** ins, float ** outs, size_t nFrames, size_t sizeInputs, size_t sizeOutputs);
    [[nodiscard]] int processAudio(jack_nframes_t nFrames);

private:
    //==============================================================================
    // Connect the server's outputs to the system's inputs.
    void connectedGrisToSystem();
    //==============================================================================
    JUCE_LEAK_DETECTOR(JackClient)
};
