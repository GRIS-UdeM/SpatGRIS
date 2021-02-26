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

#include "AudioProcessor.h"

#include <array>
#include <cstdarg>

#include "AudioManager.h"
#include "MainComponent.h"
#include "Speaker.h"
#include "constants.hpp"
#include "narrow.hpp"
#include "vbap.hpp"

float constexpr SMALL_GAIN = 0.0000000000001f;
size_t constexpr MAX_BUFFER_SIZE = 2048;
size_t constexpr LEFT = 0;
size_t constexpr RIGHT = 1;

//==============================================================================
// Utilities.
template<typename Coll>
static bool contains(Coll const & coll, typename Coll::value_type const & value) noexcept
{
    return std::find(std::cbegin(coll), std::cend(coll), value) != std::cend(coll);
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
    audioFormatReader->read(wavData.data(), 2, 0, narrow<int>(audioFormatReader->lengthInSamples), false);
    juce::AudioBuffer<float> samples{ 2, narrow<int>(audioFormatReader->lengthInSamples) };
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
AudioProcessor::AudioProcessor()
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
        std::memcpy(mVbapHrtfLeftImpulses[i + 8].data(), stbuf.getReadPointer(leftChannel), 128);
        std::memcpy(mVbapHrtfRightImpulses[i + 8].data(), stbuf.getReadPointer(rightChannel), 128);
    }
    // Azimuth = 80
    for (int i = 0; i < 2; i++) {
        auto const file{ HRTF_FOLDER_80.getChildFile("H80e090a.wav") };
        auto const stbuf{ getSamplesFromWavFile(file) };
        auto const leftChannel{ 1 - i };
        auto const rightChannel{ i };
        std::memcpy(mVbapHrtfLeftImpulses[i + 14].data(), stbuf.getReadPointer(leftChannel), 128);
        std::memcpy(mVbapHrtfRightImpulses[i + 14].data(), stbuf.getReadPointer(rightChannel), 128);
    }

    // Initialize STEREO data.
    // Initialize LBAP data.
    mLbapSpeakerField = lbap_field_init();

    mInterMaster = 0.8f;

    // open a client connection to the JACK server. Start server if it is not running.

    auto & audioManager{ AudioManager::getInstance() };

    // Register Jack callbacks and ports.
    audioManager.registerAudioProcessor(this);

    // Initialize pink noise
    srand(static_cast<unsigned>(time(nullptr)));

    mNumberInputs = narrow<unsigned>(audioManager.getPortNames(PortType::input).size());
    mNumberOutputs = narrow<unsigned>(audioManager.getPortNames(PortType::output).size());
}

//==============================================================================
void AudioProcessor::resetHrtf()
{
    std::fill(std::begin(mHrtfCount), std::end(mHrtfCount), 0u);
    static constexpr std::array<float, 128> EMPTY_HRTF_INPUT{};
    std::fill(std::begin(mHrtfInputTmp), std::end(mHrtfInputTmp), EMPTY_HRTF_INPUT);
}

//==============================================================================
void AudioProcessor::clientRegistrationCallback(char const * const name, int const regist)
{
    std::lock_guard<std::mutex> lock{ mClientsLock };
    if (regist) {
        ClientData cli;
        cli.name = name;
        mClients.push_back(cli);
    } else {
        juce::String const name_str{ name };
        auto const find_result{ std::find_if(
            std::cbegin(mClients),
            std::cend(mClients),
            [&name_str](ClientData const & client) { return client.name == name_str; }) };
        mClients.erase(find_result);
    }
}

//==============================================================================
void AudioProcessor::addRemoveInput(unsigned int const number)
{
    juce::ScopedLock const lock{ getCriticalSection() };

    if (number < mInputsPort.size()) {
        while (number < mInputsPort.size()) {
            AudioManager::getInstance().unregisterPort(mInputsPort.back());
            mInputsPort.pop_back();
        }
    } else {
        auto & audioManager{ AudioManager::getInstance() };
        while (number > mInputsPort.size()) {
            juce::String nameIn{ "input" };
            nameIn += juce::String{ mInputsPort.size() + 1 };
            auto * newPort{ audioManager.registerPort(nameIn.toStdString().c_str(), "SpatGRIS2", PortType::input) };
            mInputsPort.push_back(newPort);
        }
    }
    connectedGrisToSystem();
}

//==============================================================================
void AudioProcessor::clearOutput()
{
    auto const num_output_ports{ mOutputsPort.size() };
    for (size_t i{}; i < num_output_ports; ++i) {
        AudioManager::getInstance().unregisterPort(mOutputsPort.back());
        mOutputsPort.pop_back();
    }
}

//==============================================================================
bool AudioProcessor::addOutput(output_patch_t const outputPatch)
{
    juce::ScopedLock const lock{ getCriticalSection() };

    if (outputPatch > mMaxOutputPatch) {
        mMaxOutputPatch = outputPatch;
    }
    juce::String nameOut = "output";
    nameOut += juce::String(mOutputsPort.size() + 1);

    auto * newPort{
        AudioManager::getInstance().registerPort(nameOut.toStdString().c_str(), "SpatGRIS2", PortType::output)
    };

    mOutputsPort.push_back(newPort);
    connectedGrisToSystem();
    return true;
}

//==============================================================================
void AudioProcessor::removeOutput(int const number)
{
    AudioManager::getInstance().unregisterPort(mOutputsPort.at(number));
    mOutputsPort.erase(mOutputsPort.begin() + number);
}

//==============================================================================
std::vector<output_patch_t> AudioProcessor::getDirectOutOutputPatches() const
{
    std::vector<output_patch_t> directOutOutputPatches;
    for (auto const & it : mSpeakersOut) {
        if (it.directOut && it.outputPatch != output_patch_t{ 0 })
            directOutOutputPatches.push_back(it.outputPatch);
    }
    return directOutOutputPatches;
}

//==============================================================================
void AudioProcessor::muteSoloVuMeterIn(float * const * ins, size_t const nFrames, size_t const sizeInputs) noexcept
{
    for (unsigned inputIndex{}; inputIndex < sizeInputs; ++inputIndex) {
        auto * buffer{ ins[inputIndex] };
        auto const & sourceData{ mSourcesData[inputIndex] };
        if (sourceData.isMuted) { // Mute
            memset(buffer, 0, sizeof(float) * nFrames);
        } else if (mSoloIn) { // Solo
            if (!sourceData.isSolo) {
                memset(buffer, 0, sizeof(float) * nFrames);
            }
        }

        // VuMeter
        static auto constexpr ABS = [](float const value) { return std::abs(value); };
        static auto constexpr MAX = [](float const max, float const value) { return max > value ? max : value; };
        auto const maxGain{ std::transform_reduce(buffer, buffer + nFrames, 0.0f, MAX, ABS) };
        mLevelsIn[inputIndex] = maxGain;
    }
}

//==============================================================================
void AudioProcessor::muteSoloVuMeterGainOut(float * const * outs,
                                            size_t const nFrames,
                                            size_t const sizeOutputs,
                                            float const gain) noexcept
{
    for (unsigned i{}; i < sizeOutputs; ++i) {
        if (mSpeakersOut[i].isMuted) { // Mute
            memset(outs[i], 0, sizeof(float) * nFrames);
        } else if (mSoloOut) { // Solo
            if (!mSpeakersOut[i].isSolo) {
                memset(outs[i], 0, sizeof(float) * nFrames);
            }
        }

        // Speaker independent gain.
        auto const outputGain{ mSpeakersOut[i].gain * gain };
        std::for_each(outs[i], outs[i] + nFrames, [outputGain](float & value) { value *= outputGain; });

        // Speaker independent crossover filter.
        if (mSpeakersOut[i].hpActive) {
            auto const & so{ mSpeakersOut[i] };
            for (unsigned int f = 0; f < nFrames; ++f) {
                auto const inval{ static_cast<double>(outs[i][f]) };
                auto const val{ so.ha0 * inval + so.ha1 * mCrossoverHighpassX1[i] + so.ha2 * mCrossoverHighpassX2[i]
                                + so.ha1 * mCrossoverHighpassX3[i] + so.ha0 * mCrossoverHighpassX4[i]
                                - so.b1 * mCrossoverHighpassY1[i] - so.b2 * mCrossoverHighpassY2[i]
                                - so.b3 * mCrossoverHighpassY3[i] - so.b4 * mCrossoverHighpassY4[i] };
                mCrossoverHighpassY4[i] = mCrossoverHighpassY3[i];
                mCrossoverHighpassY3[i] = mCrossoverHighpassY2[i];
                mCrossoverHighpassY2[i] = mCrossoverHighpassY1[i];
                mCrossoverHighpassY1[i] = val;
                mCrossoverHighpassX4[i] = mCrossoverHighpassX3[i];
                mCrossoverHighpassX3[i] = mCrossoverHighpassX2[i];
                mCrossoverHighpassX2[i] = mCrossoverHighpassX1[i];
                mCrossoverHighpassX1[i] = inval;
                outs[i][f] = static_cast<float>(val);
            }
        }

        // VuMeter
        auto maxGain{ 0.0f };
        for (unsigned j{ 1 }; j < nFrames; ++j) {
            auto const absGain = std::abs(outs[i][j]);
            if (absGain > maxGain) {
                maxGain = absGain;
            }
        }
        mLevelsOut[i] = maxGain;

        //// Record buffer.
        // if (mIsRecording) {
        //    if (channelCount == sizeOutputs && i < channelCount) {
        //        if (contains(mOutputPatches, i + 1u)) {
        //            mRecorders[i].recordSamples(&outs[i], narrow<int>(nFrames));
        //        }
        //    } else if (channelCount == 2 && i < channelCount) {
        //        mRecorders[i].recordSamples(&outs[i], narrow<int>(nFrames));
        //    }
        //}
    }

    //// Recording index.
    // if (!mIsRecording && mIndexRecord > 0) {
    //    if (channelCount == sizeOutputs) {
    //        for (unsigned int i = 0; i < sizeOutputs; ++i) {
    //            if (contains(mOutputPatches, i + 1) && i < channelCount) {
    //                mRecorders[i].stop();
    //            }
    //        }
    //    } else if (channelCount == 2) {
    //        mRecorders[0].stop();
    //        mRecorders[1].stop();
    //    }
    //    mIndexRecord = 0;
    //} else if (mIsRecording) {
    //    mIndexRecord += nFrames;
    //}
}

//==============================================================================
void AudioProcessor::addNoiseSound(float * const * outs, size_t const nFrames, size_t const sizeOutputs) noexcept
{
    static constexpr auto FAC{ 1.0f / (static_cast<float>(RAND_MAX) / 2.0f) };
    for (unsigned int nF = 0; nF < nFrames; ++nF) {
        auto const rnd{ rand() * FAC - 1.0f };
        mPinkNoiseC0 = mPinkNoiseC0 * 0.99886f + rnd * 0.0555179f;
        mPinkNoiseC1 = mPinkNoiseC1 * 0.99332f + rnd * 0.0750759f;
        mPinkNoiseC2 = mPinkNoiseC2 * 0.96900f + rnd * 0.1538520f;
        mPinkNoiseC3 = mPinkNoiseC3 * 0.86650f + rnd * 0.3104856f;
        mPinkNoiseC4 = mPinkNoiseC4 * 0.55000f + rnd * 0.5329522f;
        mPinkNoiseC5 = mPinkNoiseC5 * -0.7616f - rnd * 0.0168980f;
        auto val{ mPinkNoiseC0 + mPinkNoiseC1 + mPinkNoiseC2 + mPinkNoiseC3 + mPinkNoiseC4 + mPinkNoiseC5 + mPinkNoiseC6
                  + rnd * 0.5362f };
        val *= 0.2f;
        val *= mPinkNoiseGain;
        mPinkNoiseC6 = rnd * 0.115926f;

        for (unsigned int i = 0; i < sizeOutputs; i++) {
            outs[i][nF] += val;
        }
    }
}

//==============================================================================
void AudioProcessor::processVbap(float const * const * ins,
                                 float * const * outs,
                                 size_t const nFrames,
                                 size_t const sizeInputs,
                                 size_t const sizeOutputs) noexcept
{
    for (unsigned i{}; i < sizeInputs; ++i) {
        if (mVbapSourcesToUpdate[i] == 1) {
            updateSourceVbap(narrow<int>(i));
            mVbapSourcesToUpdate[i] = 0;
        }
    }

    auto const gainFactor{ std::pow(mInterMaster, 0.1f) * 0.0099f + 0.99f };

    for (unsigned outputIndex{}; outputIndex < sizeOutputs; ++outputIndex) {
        auto * outputBuffer{ outs[outputIndex] };
        for (unsigned inputIndex{}; inputIndex < sizeInputs; ++inputIndex) {
            if (mLevelsIn[inputIndex] < SMALL_GAIN) {
                // nothing to process
                continue;
            }

            auto const * inputBuffer{ ins[inputIndex] };
            if (mSourcesData[inputIndex].directOut.get() || mSourcesData[inputIndex].paramVBap == nullptr) {
                // direct out
                if (narrow<unsigned>(mSourcesData[inputIndex].directOut.get() - 1) == outputIndex) {
                    // output matches
                    for (size_t sampleIndex{}; sampleIndex < nFrames; ++sampleIndex) {
                        outputBuffer[sampleIndex] += inputBuffer[sampleIndex];
                    }
                }
            } else {
                // spat
                auto currentGain{ mSourcesData[inputIndex].paramVBap->gainsSmoothing[outputIndex] };
                auto const targetGain{ mSourcesData[inputIndex].paramVBap->gains[outputIndex] };
                if (mInterMaster == 0.0f) {
                    // linear interpolation over buffer size
                    auto const gainSlope = (targetGain - currentGain) / nFrames;
                    if (targetGain < SMALL_GAIN && currentGain < SMALL_GAIN) {
                        // this is not going to produce any more sounds!
                        continue;
                    }
                    for (size_t sampleIndex{}; sampleIndex < nFrames; ++sampleIndex) {
                        currentGain += gainSlope;
                        outputBuffer[sampleIndex] += inputBuffer[sampleIndex] * currentGain;
                    }
                } else {
                    // log interpolation with 1st order filter
                    for (size_t sampleIndex{}; sampleIndex < nFrames; ++sampleIndex) {
                        currentGain = targetGain + (currentGain - targetGain) * gainFactor;
                        if (currentGain < SMALL_GAIN && targetGain < SMALL_GAIN) {
                            // If the gain is near zero and the target gain is also near zero, this means that
                            // currentGain will no ever increase over this buffer
                            break;
                        }
                        outputBuffer[sampleIndex] += inputBuffer[sampleIndex] * currentGain;
                    }
                }
                mSourcesData[inputIndex].paramVBap->gainsSmoothing[outputIndex] = currentGain;
            }
        }
    }
}

//==============================================================================
void AudioProcessor::processLbap(float const * const * ins,
                                 float * const * outs,
                                 size_t const nFrames,
                                 size_t const sizeInputs,
                                 size_t const sizeOutputs) noexcept
{
    jassert(nFrames <= MAX_BUFFER_SIZE);
    std::array<float, MAX_BUFFER_SIZE> filteredInputSignal{};

    auto const gainFactor{ std::pow(mInterMaster, 0.1f) * 0.0099f + 0.99f };

    for (unsigned inputIndex{}; inputIndex < sizeInputs; ++inputIndex) {
        if (mLevelsIn[inputIndex] < SMALL_GAIN) {
            // nothing to process
            continue;
        }

        auto const * inputBuffer{ ins[inputIndex] };
        auto & sourceData{ mSourcesData[inputIndex] };
        if (!sourceData.directOut.get()) {
            lbap_pos pos;
            lbap_pos_init_from_radians(&pos, sourceData.radAzimuth, sourceData.radElevation, sourceData.radius);
            pos.radspan = sourceData.azimuthSpan;
            pos.elespan = sourceData.zenithSpan;
            auto distance{ sourceData.radius };
            if (!lbap_pos_compare(&pos, &sourceData.lbapLastPos)) {
                lbap_field_compute(mLbapSpeakerField, &pos, sourceData.lbapGains.data());
                lbap_pos_copy(&sourceData.lbapLastPos, &pos);
            }

            float distanceGain;
            float distanceCoefficient;
            // Energy lost with distance, radius is in the range 0 - 2.6 (>1 is beyond HP circle).
            if (distance < 1.0f) {
                distanceGain = 1.0f;
                distanceCoefficient = 0.0f;
            } else {
                distance = (distance - 1.0f) * 1.25f;
                if (distance > 1.0f) {
                    distance = 1.0f;
                }
                distanceGain = (1.0f - distance) * (1.0f - mAttenuationLinearGain) + mAttenuationLinearGain;
                distanceCoefficient = distance * mAttenuationLowpassCoefficient;
            }
            auto const diffGain{ (distanceGain - mLastAttenuationGain[inputIndex]) / narrow<float>(nFrames) };
            auto const diffCoefficient
                = (distanceCoefficient - mLastAttenuationCoefficient[inputIndex]) / narrow<float>(nFrames);
            auto filterInY{ mAttenuationLowpassY[inputIndex] };
            auto filterInZ{ mAttenuationLowpassZ[inputIndex] };
            auto lastCoefficient{ mLastAttenuationCoefficient[inputIndex] };
            auto lastGain{ mLastAttenuationGain[inputIndex] };
            for (size_t sampleIndex{}; sampleIndex < nFrames; ++sampleIndex) {
                lastCoefficient += diffCoefficient;
                lastGain += diffGain;
                filterInY = inputBuffer[sampleIndex] + (filterInY - inputBuffer[sampleIndex]) * lastCoefficient;
                filterInZ = filterInY + (filterInZ - filterInY) * lastCoefficient;
                filteredInputSignal[sampleIndex] = filterInZ * lastGain;
            }
            mAttenuationLowpassY[inputIndex] = filterInY;
            mAttenuationLowpassZ[inputIndex] = filterInZ;
            mLastAttenuationGain[inputIndex] = distanceGain;
            mLastAttenuationCoefficient[inputIndex] = distanceCoefficient;
            //==============================================================================
            for (unsigned outputIndex{}; outputIndex < sizeOutputs; ++outputIndex) {
                auto * outputBuffer{ outs[outputIndex] };
                auto const targetGain{ sourceData.lbapGains[outputIndex] };
                auto currentGain{ sourceData.lbapY[outputIndex] };
                if (mInterMaster == 0.0f) {
                    // linear interpolation over buffer size
                    auto const gainSlope = (targetGain - currentGain) / nFrames;
                    if (currentGain < SMALL_GAIN && targetGain < SMALL_GAIN) {
                        // This is not going to produce any more sounds!
                        continue;
                    }
                    for (size_t sampleIndex{}; sampleIndex < nFrames; ++sampleIndex) {
                        currentGain += gainSlope;
                        outputBuffer[sampleIndex] += filteredInputSignal[sampleIndex] * currentGain;
                    }
                } else {
                    // log interpolation with 1st order filter
                    for (size_t sampleIndex{}; sampleIndex < nFrames; ++sampleIndex) {
                        currentGain = (currentGain - targetGain) * gainFactor + targetGain;
                        if (currentGain < SMALL_GAIN && targetGain < SMALL_GAIN) {
                            // If the gain is near zero and the target gain is also near zero, this means that
                            // currentGain will no ever increase over this buffer
                            break;
                        }
                        outputBuffer[sampleIndex] += filteredInputSignal[sampleIndex] * currentGain;
                    }
                }
                sourceData.lbapY[outputIndex] = currentGain;
            }
        } else {
            // direct out
            for (unsigned outputIndex{}; outputIndex < sizeOutputs; ++outputIndex) {
                if (narrow<unsigned>(sourceData.directOut.get() - 1) == outputIndex) {
                    auto * outputBuffer{ outs[outputIndex] };
                    for (size_t sampleIndex{}; sampleIndex < nFrames; ++sampleIndex) {
                        outputBuffer[sampleIndex] += inputBuffer[sampleIndex];
                    }
                }
            }
        }
    }
}

//==============================================================================
void AudioProcessor::processVBapHrtf(float const * const * ins,
                                     float * const * outs,
                                     size_t const nFrames,
                                     size_t const sizeInputs) noexcept
{
    for (unsigned i{}; i < sizeInputs; ++i) {
        if (mVbapSourcesToUpdate[i] == 1) {
            updateSourceVbap(narrow<int>(i));
            mVbapSourcesToUpdate[i] = 0;
        }
    }

    auto const gainFactor{ std::pow(mInterMaster, 0.1f) * 0.0099f + 0.99f };
    for (unsigned outputIndex{}; outputIndex < 16; ++outputIndex) {
        jassert(nFrames <= MAX_BUFFER_SIZE);
        std::array<float, MAX_BUFFER_SIZE> vbapOuts{};
        for (unsigned inputIndex{}; inputIndex < sizeInputs; ++inputIndex) {
            if (mLevelsIn[inputIndex] < SMALL_GAIN) {
                // nothing to process
                continue;
            }

            auto & sourceData{ mSourcesData[inputIndex] };
            if (!sourceData.directOut.get() && sourceData.paramVBap != nullptr) {
                auto const targetGain{ sourceData.paramVBap->gains[outputIndex] };
                auto currentGain{ sourceData.paramVBap->gainsSmoothing[outputIndex] };
                auto const * inputBuffer{ ins[inputIndex] };
                if (mInterMaster == 0.0f) {
                    // linear interpolation
                    if (currentGain < SMALL_GAIN && targetGain < SMALL_GAIN) {
                        continue;
                    }
                    auto const gainSlope = (targetGain - currentGain) / nFrames;
                    for (size_t sampleIndex{}; sampleIndex < nFrames; ++sampleIndex) {
                        currentGain += gainSlope;
                        vbapOuts[sampleIndex] += inputBuffer[sampleIndex] * currentGain;
                    }
                } else {
                    // log interpolation with 1st order filter
                    for (size_t sampleIndex{}; sampleIndex < nFrames; ++sampleIndex) {
                        currentGain = targetGain + (currentGain - targetGain) * gainFactor;
                        if (currentGain < SMALL_GAIN && targetGain < SMALL_GAIN) {
                            break;
                        }
                        vbapOuts[sampleIndex] += inputBuffer[sampleIndex] * currentGain;
                    }
                }
                sourceData.paramVBap->gainsSmoothing[outputIndex] = currentGain;
            }
        }

        for (unsigned sampleIndex{}; sampleIndex < nFrames; ++sampleIndex) {
            auto tmpCount{ narrow<int>(mHrtfCount[outputIndex]) };
            static constexpr auto MYSTERY = 128; // TODO : solve
            for (unsigned hrtfIndex{}; hrtfIndex < MYSTERY; ++hrtfIndex) {
                if (tmpCount < 0) {
                    tmpCount += MYSTERY;
                }
                auto const sig{ mHrtfInputTmp[outputIndex][tmpCount] };
                outs[LEFT][sampleIndex] += sig * mVbapHrtfLeftImpulses[outputIndex][hrtfIndex];
                outs[RIGHT][sampleIndex] += sig * mVbapHrtfRightImpulses[outputIndex][hrtfIndex];
                --tmpCount;
            }
            mHrtfCount[outputIndex]++;
            if (mHrtfCount[outputIndex] >= MYSTERY) {
                mHrtfCount[outputIndex] = 0;
            }
            mHrtfInputTmp[outputIndex][mHrtfCount[outputIndex]] = vbapOuts[sampleIndex];
        }
    }

    // Add direct outs to the now stereo signal.
    for (unsigned inputIndex{}; inputIndex < sizeInputs; ++inputIndex) {
        if (mSourcesData[inputIndex].directOut.get() != 0) {
            if (mSourcesData[inputIndex].directOut.get() % 2 == 1) {
                for (size_t sampleIndex{}; sampleIndex < nFrames; ++sampleIndex) {
                    outs[LEFT][sampleIndex] += ins[inputIndex][sampleIndex];
                }
            } else {
                for (size_t sampleIndex{}; sampleIndex < nFrames; ++sampleIndex) {
                    outs[RIGHT][sampleIndex] += ins[inputIndex][sampleIndex];
                }
            }
        }
    }
}

//==============================================================================
void AudioProcessor::processStereo(float const * const * ins,
                                   float * const * outs,
                                   size_t const nFrames,
                                   size_t const sizeInputs) noexcept
{
    static auto constexpr FACTOR{ juce::MathConstants<float>::pi / 360.0f };
    auto const gainFactor{ std::pow(mInterMaster, 0.1f) * 0.0099f + 0.99f };

    for (unsigned inputIndex{}; inputIndex < sizeInputs; ++inputIndex) {
        if (mLevelsIn[inputIndex] < SMALL_GAIN) {
            // nothing to process
            continue;
        }

        auto const & sourceData{ mSourcesData[inputIndex] };
        auto const * inputBuffer{ ins[inputIndex] };
        if (!sourceData.directOut.get()) {
            auto const azimuth{ sourceData.azimuth };
            auto lastAzimuth{ mLastAzimuth[inputIndex] };
            for (size_t sampleIndex{}; sampleIndex < nFrames; ++sampleIndex) {
                // Removes the chirp at 180->-180 degrees azimuth boundary.
                if (std::abs(lastAzimuth - azimuth) > 300.0f) {
                    lastAzimuth = azimuth;
                }
                lastAzimuth = azimuth + (lastAzimuth - azimuth) * gainFactor;
                float scaled;
                if (lastAzimuth < -90.0f) {
                    scaled = -90.0f - (lastAzimuth + 90.0f);
                } else if (lastAzimuth > 90.0f) {
                    scaled = 90.0f - (lastAzimuth - 90.0f);
                } else {
                    scaled = lastAzimuth;
                }
                scaled = (scaled + 90.0f) * FACTOR;
                using fast = juce::dsp::FastMathApproximations;
                outs[LEFT][sampleIndex] += inputBuffer[sampleIndex] * fast::cos(scaled);
                outs[RIGHT][sampleIndex] += inputBuffer[sampleIndex] * fast::sin(scaled);
            }
            mLastAzimuth[inputIndex] = lastAzimuth;

        } else if (sourceData.directOut.get() % 2 == 1) {
            // left direct out
            for (size_t sampleIndex{}; sampleIndex < nFrames; ++sampleIndex) {
                outs[LEFT][sampleIndex] += inputBuffer[sampleIndex];
            }
        } else {
            // right direct out
            for (size_t sampleIndex{}; sampleIndex < nFrames; ++sampleIndex) {
                outs[RIGHT][sampleIndex] += inputBuffer[sampleIndex];
            }
        }
    }
    // Apply gain compensation.
    auto const compensation{ std::pow(10.0f, (narrow<float>(sizeInputs) - 1.0f) * -0.1f * 0.05f) };
    for (size_t sampleIndex{}; sampleIndex < nFrames; ++sampleIndex) {
        outs[LEFT][sampleIndex] *= compensation;
        outs[RIGHT][sampleIndex] *= compensation;
    }
}

//==============================================================================
void AudioProcessor::processAudio(size_t const nFrames) noexcept
{
    // Skip if the user is editing the speaker setup.
    juce::ScopedTryLock const lock{ getCriticalSection() };
    if (!lock.isLocked()) {
        return;
    }

    float * ins[MAX_INPUTS];
    float * outs[MAX_OUTPUTS];

    auto const sizeInputs{ mInputsPort.size() };
    auto const sizeOutputs{ mOutputsPort.size() };

    // consolidate all buffers
    auto & audioManager{ AudioManager::getInstance() };
    for (unsigned int i = 0; i < sizeInputs; i++) {
        ins[i] = audioManager.getBuffer(mInputsPort[i], nFrames);
    }
    for (unsigned int i = 0; i < sizeOutputs; i++) {
        outs[i] = audioManager.getBuffer(mOutputsPort[i], nFrames);
    }

    muteSoloVuMeterIn(ins, nFrames, sizeInputs);

    switch (mModeSelected) {
    case SpatMode::vbap:
        processVbap(ins, outs, nFrames, sizeInputs, sizeOutputs);
        break;
    case SpatMode::lbap:
        processLbap(ins, outs, nFrames, sizeInputs, sizeOutputs);
        break;
    case SpatMode::hrtfVbap:
        processVBapHrtf(ins, outs, nFrames, sizeInputs);
        break;
    case SpatMode::stereo:
        processStereo(ins, outs, nFrames, sizeInputs);
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
}

//==============================================================================
void AudioProcessor::connectedGrisToSystem()
{
    clearOutput();
    auto & audioManager{ AudioManager::getInstance() };
    for (output_patch_t i{}; i < mMaxOutputPatch; ++i) {
        juce::String nameOut{ "output" };
        nameOut += juce::String{ mOutputsPort.size() + 1 };

        auto * newPort{ audioManager.registerPort(nameOut.toStdString().c_str(), "SpatGRIS2", PortType::output) };

        mOutputsPort.push_back(newPort);
    }

    auto const outputPorts{ audioManager.getOutputPorts() };
    auto const inputPorts{ audioManager.getInputPorts() };

    // DisConnect JackClientGris to system.
    // TODO : this is confusing
    for (auto * outputPort : outputPorts) {
        if (strcmp(outputPort->clientName, CLIENT_NAME) == 0) { // jackClient
            for (auto * inputPort : inputPorts) {
                if (strcmp(inputPort->clientName, SYS_CLIENT_NAME) == 0
                    && audioManager.isConnectedTo(outputPort, inputPort->fullName)) {
                    audioManager.disconnect(outputPort, inputPort);
                }
            }
        }
    }

    // Connect JackClientGris to system.
    for (auto * outputPort : outputPorts) {
        if (strcmp(outputPort->clientName, CLIENT_NAME) == 0) { // jackClient
            for (auto * inputPort : inputPorts) {
                if (strcmp(inputPort->clientName, SYS_CLIENT_NAME) == 0) { // system
                    audioManager.connect(outputPort, inputPort);
                    break;
                }
            }
        }
    }

    // Build output patch list.
    mOutputPatches.clear();
    for (size_t i{}; i < mOutputsPort.size(); ++i) {
        if (mSpeakersOut[i].outputPatch != output_patch_t{ 0 }) {
            mOutputPatches.push_back(mSpeakersOut[i].outputPatch);
        }
    }
}

//==============================================================================
bool AudioProcessor::initSpeakersTriplet(std::vector<Speaker *> const & listSpk,
                                         int const dimensions,
                                         bool const needToComputeVbap)
{
    if (listSpk.empty()) {
        return false;
    }

    LoudSpeaker lss[MAX_SPEAKER_COUNT];
    output_patch_t outputPatches[MAX_SPEAKER_COUNT];

    int j;
    for (unsigned i{}; i < listSpk.size(); ++i) {
        for (j = 0; j < MAX_SPEAKER_COUNT; ++j) {
            if (listSpk[i]->getOutputPatch() == mSpeakersOut[j].outputPatch && !mSpeakersOut[j].directOut) {
                break;
            }
        }
        lss[i].coords.x = mSpeakersOut[j].x;
        lss[i].coords.y = mSpeakersOut[j].y;
        lss[i].coords.z = mSpeakersOut[j].z;
        lss[i].angles.azimuth = mSpeakersOut[j].azimuth;
        lss[i].angles.elevation = mSpeakersOut[j].zenith;
        lss[i].angles.length = mSpeakersOut[j].radius; // Always 1.0 for VBAP.
        outputPatches[i] = mSpeakersOut[j].outputPatch;
    }

    if (needToComputeVbap) {
        free_vbap_data(mParamVBap);
        mParamVBap = init_vbap_from_speakers(lss,
                                             narrow<int>(listSpk.size()),
                                             dimensions,
                                             outputPatches,
                                             mMaxOutputPatch,
                                             nullptr);
        if (mParamVBap == nullptr) {
            return false;
        }
    }

    for (unsigned i{}; i < MAX_INPUTS; ++i) {
        auto & paramVbap{ mSourcesData[i].paramVBap };
        free_vbap_data(paramVbap);
        ;
        paramVbap = copy_vbap_data(mParamVBap);
    }

    int ** triplets;
    auto const num{ vbap_get_triplets(mSourcesData[0].paramVBap, &triplets) };
    mVbapTriplets.clear();
    for (int i{}; i < num; ++i) {
        Triplet const row{ output_patch_t{ triplets[i][0] },
                           output_patch_t{ triplets[i][1] },
                           output_patch_t{ triplets[i][2] } };
        mVbapTriplets.add(row);
    }

    for (int i{}; i < num; i++) {
        free(triplets[i]);
    }
    free(triplets);

    connectedGrisToSystem();

    return true;
}

//==============================================================================
bool AudioProcessor::lbapSetupSpeakerField(std::vector<Speaker *> const & listSpk)
{
    int j;
    if (listSpk.empty()) {
        return false;
    }

    float azimuth[MAX_OUTPUTS];
    float elevation[MAX_OUTPUTS];
    float radius[MAX_OUTPUTS];
    output_patch_t outputPatch[MAX_OUTPUTS];

    for (unsigned int i = 0; i < listSpk.size(); i++) {
        for (j = 0; j < MAX_SPEAKER_COUNT; ++j) {
            if (listSpk[i]->getOutputPatch() == mSpeakersOut[j].outputPatch && !mSpeakersOut[j].directOut) {
                break;
            }
        }
        azimuth[i] = mSpeakersOut[j].azimuth;
        elevation[i] = mSpeakersOut[j].zenith;
        radius[i] = mSpeakersOut[j].radius;
        outputPatch[i] = --mSpeakersOut[j].outputPatch;
    }

    auto * const speakers{
        lbap_speakers_from_positions(azimuth, elevation, radius, outputPatch, narrow<int>(listSpk.size()))
    };

    lbap_field_reset(mLbapSpeakerField);
    lbap_field_setup(mLbapSpeakerField, speakers, narrow<int>(listSpk.size()));

    free(speakers);

    connectedGrisToSystem();

    return true;
}

//==============================================================================
void AudioProcessor::setAttenuationDb(float const value)
{
    mAttenuationLinearGain = value;
}

//==============================================================================
void AudioProcessor::setAttenuationHz(float const value)
{
    mAttenuationLowpassCoefficient = value;
}

//==============================================================================
void AudioProcessor::updateSourceVbap(int const idS) noexcept
{
    if (mVbapDimensions == 3) {
        if (mSourcesData[idS].paramVBap != nullptr) {
            vbap2_flip_y_z(mSourcesData[idS].azimuth,
                           mSourcesData[idS].zenith,
                           mSourcesData[idS].azimuthSpan,
                           mSourcesData[idS].zenithSpan,
                           mSourcesData[idS].paramVBap);
        }
    } else if (mVbapDimensions == 2) {
        if (mSourcesData[idS].paramVBap != nullptr) {
            vbap2(mSourcesData[idS].azimuth, 0.0f, mSourcesData[idS].azimuthSpan, 0.0f, mSourcesData[idS].paramVBap);
        }
    }
}

//==============================================================================
void AudioProcessor::connectionClient(juce::String const & name, bool const connect)
{
    juce::ScopedLock const lock{ getCriticalSection() };

    auto & audioManager{ AudioManager::getInstance() };
    auto const inputPorts{ audioManager.getInputPorts() };
    auto const outputPorts{ audioManager.getOutputPorts() };

    updateClientPortAvailable(false);

    // Disconnect client.
    for (auto * outputPort : outputPorts) {
        if (name == outputPort->clientName) {
            for (auto * inputPort : inputPorts) {
                if (strcmp(inputPort->clientName, CLIENT_NAME) == 0
                    && audioManager.isConnectedTo(inputPort, outputPort)) {
                    audioManager.disconnect(inputPort, outputPort);
                }
            }
        }
    }

    for (auto & cli : mClients) {
        if (cli.name == name) {
            cli.connected = true; // since there is no checkbox anymore, lets set this to true by default.
        }
    }

    connectedGrisToSystem();

    if (!connect) {
        return;
    }

    // Connect other client to JackClientGris
    mAutoConnection = true;

    auto conn{ false };
    for (auto & cli : mClients) {
        auto const & nameClient = cli.name;
        auto startJ{ narrow<int>(cli.portStart) - 1 };
        auto endJ{ narrow<int>(cli.portEnd) };

        for (auto * outputPort : outputPorts) {
            if (nameClient == name && nameClient == outputPort->clientName) {
                for (int j{}; j < inputPorts.size(); ++j) {
                    auto * inputPort{ inputPorts[j] };
                    if (strcmp(inputPort->clientName, CLIENT_NAME) == 0) {
                        if (j >= startJ && j < endJ) {
                            audioManager.connect(outputPort, inputPort);
                            conn = true;
                            break;
                        }
                    } else {
                        startJ += 1;
                        endJ += 1;
                    }
                }
                cli.connected = conn;
            }
        }
    }

    mAutoConnection = false;
}

//==============================================================================
void AudioProcessor::updateClientPortAvailable(bool const fromJack)
{
    auto const outputPorts{ AudioManager::getInstance().getOutputPorts() };

    for (auto & client : mClients) {
        client.portAvailable = 0;
    }

    for (auto const * outputPort : outputPorts) {
        if (strcmp(outputPort->clientName, CLIENT_NAME) != 0 && strcmp(outputPort->clientName, SYS_CLIENT_NAME) != 0) {
            for (auto & client : mClients) {
                if (client.name.compare(outputPort->clientName) == 0) {
                    client.portAvailable += 1;
                }
            }
        }
    }

    unsigned start{ 1 };
    unsigned end;
    unsigned defaultActivePorts{ 64 };
    for (auto & client : mClients) {
        if (!fromJack) {
            client.initialized = true;
            end = client.activePorts;
        } else {
            end = client.portAvailable < defaultActivePorts ? client.portAvailable : defaultActivePorts;
        }
        if (client.portStart == 0 || client.portEnd == 0 || !client.initialized) { // ports not initialized.
            client.portStart = start;
            client.portEnd = start + end - 1;
            start += end;
        } else if (client.portStart >= client.portEnd
                   || client.portEnd - client.portStart > client.portAvailable) { // portStart bigger than portEnd.
            client.portStart = start;
            client.portEnd = start + client.activePorts - 1;
            start += client.activePorts;
        } else {
            if (mClients.size() > 1) {
                unsigned pos{ 0 };
                auto somethingBad{ false };
                for (unsigned c{}; c < mClients.size(); ++c) {
                    if (mClients[c].name == client.name) {
                        pos = c;
                        break;
                    }
                }
                if (pos == 0) {
                    somethingBad = false;
                } else if (pos >= mClients.size()) {
                    somethingBad = true; // Never supposed to get here.
                } else {
                    if (client.portStart - 1 != mClients[pos - 1].portEnd) {
                        auto const numPorts{ client.portEnd - client.portStart };
                        client.portStart = mClients[pos - 1].portEnd + 1;
                        client.portEnd = client.portStart + numPorts;
                    }
                    for (auto const & clientCompare : mClients) {
                        if (clientCompare.name != client.name && client.portStart > clientCompare.portStart
                            && client.portStart < clientCompare.portEnd) {
                            somethingBad = true;
                        } else if (clientCompare.name != client.name && client.portEnd > clientCompare.portStart
                                   && client.portEnd < clientCompare.portEnd) {
                            somethingBad = true;
                        }
                    }
                }

                if (somethingBad) { // ports overlap other client ports.
                    client.portStart = start;
                    client.portEnd = start + defaultActivePorts - 1;
                    start += defaultActivePorts;
                } else {
                    // If everything goes right, we keep portStart and portEnd for this client.
                    start = client.portEnd + 1;
                }
            }
        }
        client.activePorts = client.portEnd - client.portStart + 1;
        jassert(client.portStart <= mInputsPort.size());
    }
}

//==============================================================================
AudioProcessor::~AudioProcessor()
{
    // TODO: paramVBap and mSourcesIn->paramVBap are never deallocated.

    lbap_field_free(mLbapSpeakerField);

    auto & audioManager{ AudioManager::getInstance() };

    audioManager.getAudioDeviceManager().getCurrentAudioDevice()->close();
    for (auto * inputPort : mInputsPort) {
        audioManager.unregisterPort(inputPort);
    }
    for (auto * outputPort : mOutputsPort) {
        audioManager.unregisterPort(outputPort);
    }
}
