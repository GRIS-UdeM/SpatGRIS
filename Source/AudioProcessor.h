/*
 This file is part of SpatGRIS.

 Developers: Samuel Béland, Olivier Bélanger, Nicolas Masson

 SpatGRIS is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 SpatGRIS is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with SpatGRIS.  If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

#include <array>
#include <vector>

#include "macros.h"
DISABLE_WARNINGS
#include <JuceHeader.h>
ENABLE_WARNINGS

#include "SourceData.hpp"
#include "SpatMode.hpp"
#include "Speaker.h"
#include "SpeakerData.hpp"
#include "constants.hpp"
#include "lbap.hpp"
#include "vbap.hpp"

class Speaker;
struct audio_port_t;

//==============================================================================
/**
 * Does most of the spatialization heavy-lifting.
 */
class AudioProcessor
{
    unsigned int mNumberInputs{};
    unsigned int mNumberOutputs{};
    output_patch_t mMaxOutputPatch{};

    std::vector<output_patch_t> mOutputPatches{};

    // Audio ports
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

    // Source and output lists.
    std::array<SourceData, MAX_INPUTS> mSourcesData{};
    std::array<SpeakerData, MAX_OUTPUTS> mSpeakersOut{};

    // Which spatialization mode is selected.
    SpatMode mModeSelected{ SpatMode::vbap };

    // VBAP data.
    unsigned mVbapDimensions{};
    std::array<int, MAX_INPUTS> mVbapSourcesToUpdate{};

    juce::Array<Triplet> mVbapTriplets{};

    // BINAURAL data.
    std::array<unsigned, 16> mHrtfCount{};
    std::array<std::array<float, 128>, 16> mHrtfInputTmp{};
    std::array<std::array<float, 128>, 16> mVbapHrtfLeftImpulses{};
    std::array<std::array<float, 128>, 16> mVbapHrtfRightImpulses{};

    // STEREO data.
    std::array<float, MAX_INPUTS> mLastAzimuth{};

    // LBAP data.
    lbap_field * mLbapSpeakerField{};

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
    AudioProcessor();
    ~AudioProcessor();
    //==============================================================================
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
    void removeOutput(int number);
    bool addOutput(output_patch_t outputPatch);

    [[nodiscard]] std::vector<output_patch_t> getDirectOutOutputPatches() const;

    // Initialize VBAP algorithm.
    [[nodiscard]] bool
        initSpeakersTriplet(std::vector<Speaker *> const & listSpk, int dimensions, bool needToComputeVbap);

    // Initialize LBAP algorithm.
    [[nodiscard]] bool lbapSetupSpeakerField(std::vector<Speaker *> const & listSpk);

    // LBAP distance attenuation functions.
    void setAttenuationDbIndex(int index);
    void setAttenuationFrequencyIndex(int index);

    // Reinit HRTF delay lines.
    void resetHrtf();

    [[nodiscard]] unsigned getNumberOutputs() const { return mNumberOutputs; }
    [[nodiscard]] unsigned getNumberInputs() const { return mNumberInputs; }
    [[nodiscard]] SpatMode getMode() const { return mModeSelected; }
    [[nodiscard]] unsigned getVbapDimensions() const { return mVbapDimensions; }
    [[nodiscard]] auto & getSourcesIn() { return mSourcesData; }
    [[nodiscard]] auto const & getSourcesIn() const { return mSourcesData; }
    [[nodiscard]] auto & getVbapSourcesToUpdate() { return mVbapSourcesToUpdate; }
    [[nodiscard]] auto const & getVbapTriplets() const { return mVbapTriplets; }
    [[nodiscard]] bool getSoloIn() const { return mSoloIn; }
    [[nodiscard]] bool getSoloOut() const { return mSoloOut; }
    [[nodiscard]] auto const & getSpeakersOut() const { return mSpeakersOut; }
    [[nodiscard]] auto & getSpeakersOut() { return mSpeakersOut; }
    [[nodiscard]] output_patch_t getMaxOutputPatch() const { return mMaxOutputPatch; }
    [[nodiscard]] auto const & getInputPorts() const { return mInputsPort; }

    juce::CriticalSection const & getCriticalSection() const noexcept { return mCriticalSection; }

    void setMaxOutputPatch(output_patch_t const maxOutputPatch) { mMaxOutputPatch = maxOutputPatch; }
    void setVbapDimensions(unsigned const dimensions) { mVbapDimensions = dimensions; }
    void setSoloIn(bool const state) { mSoloIn = state; }
    void setSoloOut(bool const state) { mSoloOut = state; }
    void setMode(SpatMode const mode) { mModeSelected = mode; }
    void setMasterGainOut(float const gain) { mMasterGainOut = gain; }
    void setInterMaster(float const interMaster) { mInterMaster = interMaster; }

    //==============================================================================
    // Pink noise
    void setPinkNoiseGain(float const gain) { mPinkNoiseGain = gain; }
    void setPinkNoiseActive(bool const state) { mPinkNoiseActive = state; }

    //==============================================================================
    // Audio processing
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
    void reconnectPorts();
    void muteSoloVuMeterIn(float * const * ins, size_t nFrames, size_t sizeInputs) noexcept;
    void updateSourceVbap(int idS) noexcept;
    //==============================================================================
    JUCE_LEAK_DETECTOR(AudioProcessor)
};
