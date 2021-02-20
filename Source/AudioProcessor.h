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

#include <array>
#include <mutex>
#include <vector>

#include "macros.h"

DISABLE_WARNINGS
#include <JuceHeader.h>
ENABLE_WARNINGS

#include "AudioRecorder.h"
#include "ClientData.hpp"
#include "SourceData.hpp"
#include "SpatModes.hpp"
#include "SpeakerData.hpp"
#include "constants.hpp"
#include "lbap.hpp"
#include "vbap.hpp"

class Speaker;
struct audio_port_t;

//==============================================================================
class AudioProcessor
{
    // class variables.
    //-----------------
    unsigned int mNumberInputs{};
    unsigned int mNumberOutputs{};
    unsigned int mMaxOutputPatch{};

    std::vector<int> mOutputPatches{};

    // Jack variables.
    std::vector<audio_port_t *> mInputsPort{};
    std::vector<audio_port_t *> mOutputsPort{};

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
    std::vector<ClientData> mClients{};
    std::mutex mClientsLock{};

    // Source and output lists.
    std::array<SourceData, MAX_INPUTS> mSourcesData{};
    std::array<SpeakerData, MAX_OUTPUTS> mSpeakersOut{};

    // True when jack reports an xrun.
    bool mIsOverloaded{ false };

    // Which spatialization mode is selected.
    SpatModes mModeSelected{ SpatModes::vbap };

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

    // LBAP distance attenuation values.
    float mAttenuationLinearGain{ 0.01584893f };       // -36 dB;
    float mAttenuationLowpassCoefficient{ 0.867208f }; // 1000 Hz
    std::array<float, MAX_INPUTS> mLastAttenuationGain{};
    std::array<float, MAX_INPUTS> mLastAttenuationCoefficient{};
    std::array<float, MAX_INPUTS> mAttenuationLowpassY{};
    std::array<float, MAX_INPUTS> mAttenuationLowpassZ{};
    //==============================================================================
    // This structure is used to compute the VBAP algorithm only once. Each source only gets a copy.
    VbapData * mParamVBap{};

    juce::CriticalSection mCriticalSection{};

public:
    //==============================================================================
    // Class methods.
    AudioProcessor();
    ~AudioProcessor();

    AudioProcessor(AudioProcessor const &) = delete;
    AudioProcessor(AudioProcessor &&) = delete;

    AudioProcessor & operator=(AudioProcessor const &) = delete;
    AudioProcessor & operator=(AudioProcessor &&) = delete;
    //==============================================================================
    // Audio Status.
    [[nodiscard]] float getLevelsIn(int const index) const { return mLevelsIn[index]; }
    [[nodiscard]] float getLevelsOut(int const index) const { return mLevelsOut[index]; }

    // Manage Inputs / Outputs.
    void addRemoveInput(unsigned int number);
    void clearOutput();
    bool addOutput(unsigned int outputPatch);
    void removeOutput(int number);

    [[nodiscard]] std::vector<int> getDirectOutOutputPatches() const;

    // Manage clients.
    void connectionClient(juce::String const & name, bool connect = true);
    void updateClientPortAvailable(bool fromJack);

    // Initialize VBAP algorithm.
    [[nodiscard]] bool
        initSpeakersTriplet(std::vector<Speaker *> const & listSpk, int dimensions, bool needToComputeVbap);

    // Initialize LBAP algorithm.
    [[nodiscard]] bool lbapSetupSpeakerField(std::vector<Speaker *> const & listSpk);

    // LBAP distance attenuation functions.
    void setAttenuationDb(float value);
    void setAttenuationHz(float value);

    // Need to update a source VBAP data.
    void updateSourceVbap(int idS) noexcept;

    // Reinit HRTF delay lines.
    void resetHrtf();

    void clientRegistrationCallback(char const * name, int regist);

    [[nodiscard]] unsigned getNumberOutputs() const { return mNumberOutputs; }
    [[nodiscard]] unsigned getNumberInputs() const { return mNumberInputs; }
    [[nodiscard]] SpatModes getMode() const { return mModeSelected; }
    [[nodiscard]] unsigned getVbapDimensions() const { return mVbapDimensions; }
    [[nodiscard]] auto const & getClients() const { return mClients; }
    [[nodiscard]] auto & getClients() { return mClients; }
    [[nodiscard]] auto & getSourcesIn() { return mSourcesData; }
    [[nodiscard]] auto const & getSourcesIn() const { return mSourcesData; }
    [[nodiscard]] auto & getVbapSourcesToUpdate() { return mVbapSourcesToUpdate; }
    [[nodiscard]] auto const & getVbapTriplets() const { return mVbapTriplets; }
    [[nodiscard]] bool getSoloIn() const { return mSoloIn; }
    [[nodiscard]] bool getSoloOut() const { return mSoloOut; }
    [[nodiscard]] auto const & getSpeakersOut() const { return mSpeakersOut; }
    [[nodiscard]] auto & getSpeakersOut() { return mSpeakersOut; }
    [[nodiscard]] size_t getMaxOutputPatch() const { return mMaxOutputPatch; }
    [[nodiscard]] size_t getIndexRecord() const { return mIndexRecord; }
    //[[nodiscard]] auto const & getRecorders() const { return mRecorders; }
    //[[nodiscard]] auto const & getOutputFileNames() const { return mOutputFileNames; }
    [[nodiscard]] auto const & getInputPorts() const { return mInputsPort; }
    [[nodiscard]] auto & getClientsLock() { return mClientsLock; }

    //[[nodiscard]] bool isRecording() const { return mIsRecording; }
    [[nodiscard]] bool isOverloaded() const { return mIsOverloaded; }
    //[[nodiscard]] juce::String const & getRecordingPath() const { return mRecordingPath; }

    juce::CriticalSection const & getCriticalSection() const noexcept { return mCriticalSection; }

    void setMaxOutputPatch(unsigned const maxOutputPatch) { mMaxOutputPatch = maxOutputPatch; }
    void setVbapDimensions(unsigned const dimensions) { mVbapDimensions = dimensions; }
    void setSoloIn(bool const state) { mSoloIn = state; }
    void setSoloOut(bool const state) { mSoloOut = state; }
    void setMode(SpatModes const mode) { mModeSelected = mode; }
    void setMasterGainOut(float const gain) { mMasterGainOut = gain; }
    void setInterMaster(float const interMaster) { mInterMaster = interMaster; }

    //==============================================================================
    // Pink noise
    void setPinkNoiseGain(float const gain) { mPinkNoiseGain = gain; }
    void setPinkNoiseActive(bool const state) { mPinkNoiseActive = state; }

    //==============================================================================
    // Audio processing
    void muteSoloVuMeterIn(float * const * ins, size_t nFrames, size_t sizeInputs) noexcept;
    void muteSoloVuMeterGainOut(float * const * outs, size_t nFrames, size_t sizeOutputs, float gain = 1.0f) noexcept;
    void addNoiseSound(float * const * outs, size_t nFrames, size_t sizeOutputs) noexcept;
    void processVbap(float const * const * ins,
                     float * const * outs,
                     size_t nFrames,
                     size_t sizeInputs,
                     size_t sizeOutputs) noexcept;
    void processLbap(float const * const * ins,
                     float * const * outs,
                     size_t nFrames,
                     size_t sizeInputs,
                     size_t sizeOutputs) noexcept;
    void processVBapHrtf(float const * const * ins, float * const * outs, size_t nFrames, size_t sizeInputs) noexcept;
    void processStereo(float const * const * ins, float * const * outs, size_t nFrames, size_t sizeInputs) noexcept;
    void processAudio(size_t nFrames) noexcept;

private:
    //==============================================================================
    // Connect the server's outputs to the system's inputs.
    void connectedGrisToSystem();
    //==============================================================================
    JUCE_LEAK_DETECTOR(AudioProcessor)
};
