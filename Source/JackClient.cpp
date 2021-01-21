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

//==============================================================================
// Utilities.
template<typename Coll>
static bool contains(Coll const & coll, typename Coll::value_type const & value)
{
    return std::find(coll.begin(), coll.end(), value) != coll.end();
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
    audioManager.registerJackClient(this);

    // Initialize pink noise
    srand(static_cast<unsigned int>(time(nullptr)));

    mNumberInputs = audioManager.getPortNames(PortType::input).size();
    mNumberOutputs = audioManager.getPortNames(PortType::output).size();

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
    std::lock_guard<std::mutex> lock{ mClientsLock };
    if (regist) {
        Client cli;
        cli.name = name;
        mClients.push_back(cli);
    } else {
        juce::String const name_str{ name };
        auto const find_result{ std::find_if(std::cbegin(mClients),
                                             std::cend(mClients),
                                             [&name_str](Client const & client) { return client.name == name_str; }) };
        mClients.erase(find_result);
    }
}

//==============================================================================
void JackClient::prepareToRecord()
{
    int num_of_channels;
    if (mOutputsPort.empty()) {
        return;
    }

    mIsRecording = false;
    mIndexRecord = 0;
    mOutputFileNames.clear();

    juce::String channelName;
    juce::File const file{ mRecordPath };
    auto const fileName{ file.getFileNameWithoutExtension() };
    auto const fileExtension{ file.getFileExtension() };
    auto const parentDirectory{ file.getParentDirectory().getFullPathName() };

    auto * currentAudioDevice{ AudioManager::getInstance().getAudioDeviceManager().getCurrentAudioDevice() };
    jassert(currentAudioDevice);
    auto const sampleRate{ static_cast<unsigned>(std::round(currentAudioDevice->getCurrentSampleRate())) };

    if (mModeSelected == ModeSpatEnum::VBAP || mModeSelected == ModeSpatEnum::LBAP) {
        num_of_channels = static_cast<int>(mOutputsPort.size());
        for (int i{}; i < num_of_channels; ++i) {
            if (contains(mOutputPatches, i + 1)) {
                channelName
                    = parentDirectory + "/" + fileName + "_" + juce::String(i + 1).paddedLeft('0', 3) + fileExtension;
                juce::File fileC = juce::File(channelName);
                mRecorders[i].startRecording(fileC, sampleRate, fileExtension);
                mOutputFileNames.add(fileC);
            }
        }
    } else if (mModeSelected == ModeSpatEnum::VBAP_HRTF || mModeSelected == ModeSpatEnum::STEREO) {
        num_of_channels = 2;
        for (int i{}; i < num_of_channels; ++i) {
            channelName
                = parentDirectory + "/" + fileName + "_" + juce::String(i + 1).paddedLeft('0', 3) + fileExtension;
            juce::File fileC = juce::File(channelName);
            mRecorders[i].startRecording(fileC, sampleRate, fileExtension);
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
void JackClient::addRemoveInput(unsigned int const number)
{
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
void JackClient::clearOutput()
{
    auto const num_output_ports{ mOutputsPort.size() };
    for (size_t i{}; i < num_output_ports; ++i) {
        AudioManager::getInstance().unregisterPort(mOutputsPort.back());
        mOutputsPort.pop_back();
    }
}

//==============================================================================
bool JackClient::addOutput(unsigned int const outputPatch)
{
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
void JackClient::removeOutput(int const number)
{
    AudioManager::getInstance().unregisterPort(mOutputsPort.at(number));
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
void JackClient::muteSoloVuMeterIn(float ** ins, size_t const nFrames, size_t sizeInputs)
{
    for (unsigned int i = 0; i < sizeInputs; ++i) {
        if (mSourcesIn[i].isMuted) { // Mute
            memset(ins[i], 0, sizeof(float) * nFrames);
        } else if (mSoloIn) { // Solo
            if (!mSourcesIn[i].isSolo) {
                memset(ins[i], 0, sizeof(float) * nFrames);
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
void JackClient::muteSoloVuMeterGainOut(float ** outs, size_t nFrames, size_t sizeOutputs, float const gain)
{
    size_t num_of_channels{ 2 };

    if (mModeSelected == ModeSpatEnum::VBAP || mModeSelected == ModeSpatEnum::LBAP) {
        num_of_channels = sizeOutputs;
    }

    for (unsigned int i = 0; i < sizeOutputs; ++i) {
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

        // Record buffer.
        if (mIsRecording) {
            if (num_of_channels == sizeOutputs && i < num_of_channels) {
                if (contains(mOutputPatches, i + 1u)) {
                    mRecorders[i].recordSamples(&outs[i], static_cast<int>(nFrames));
                }
            } else if (num_of_channels == 2 && i < num_of_channels) {
                mRecorders[i].recordSamples(&outs[i], static_cast<int>(nFrames));
            }
        }
    }

    // Recording index.
    if (!mIsRecording && mIndexRecord > 0) {
        if (num_of_channels == sizeOutputs) {
            for (unsigned int i = 0; i < sizeOutputs; ++i) {
                if (contains(mOutputPatches, i + 1) && i < num_of_channels) {
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
void JackClient::addNoiseSound(float ** outs, size_t nFrames, size_t sizeOutputs)
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
void JackClient::processVbap(float ** ins, float ** outs, size_t nFrames, size_t sizeInputs, size_t sizeOutputs)
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
            } else if (static_cast<unsigned>(mSourcesIn[i].directOut - 1) == o) {
                std::transform(outs[o], outs[o] + nFrames, ins[i], outs[o], std::plus());
            }
        }
    }
}

//==============================================================================
void JackClient::processLbap(float ** ins, float ** outs, size_t nFrames, size_t sizeInputs, size_t sizeOutputs)
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
            // auto distance{ sourceIn.radius };
            auto const elevation{ sourceIn.radElevation / juce::MathConstants<float>::halfPi };
            auto distance{ std::sqrt(sourceIn.radius * sourceIn.radius + elevation * elevation) };
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
                distance = (distance - 1.0f) * 1.25f;
                if (distance > 1.0f) {
                    distance = 1.0f;
                }
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
void JackClient::processVBapHrtf(float ** ins,
                                 float ** outs,
                                 size_t nFrames,
                                 size_t sizeInputs,
                                 [[maybe_unused]] size_t sizeOutputs)
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
void JackClient::processStereo(float ** ins, float ** outs, size_t nFrames, size_t sizeInputs, size_t sizeOutputs)
{
    static auto constexpr FACTOR{ juce::MathConstants<float>::pi / 360.0f };

    auto const interpolationG{ std::pow(mInterMaster, 0.1f) * 0.0099f + 0.99f };
    auto const gain{ std::pow(10.0f, (static_cast<float>(sizeInputs) - 1.0f) * -0.1f * 0.05f) };

    for (unsigned i{}; i < sizeOutputs; ++i) {
        memset(outs[i], 0, sizeof(float) * nFrames);
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
void JackClient::processAudio(size_t const nFrames)
{
    // Return if the user is editing the speaker setup.
    if (!mProcessBlockOn) {
        for (size_t i{}; i < mOutputsPort.size(); ++i) {
            auto * buffer{ AudioManager::getInstance().getBuffer(mOutputsPort[i], nFrames) };
            memset(static_cast<float *>(buffer), 0, sizeof(float) * nFrames);
            mLevelsOut[i] = 0.0f;
        }
    }

    auto const sizeInputs{ mInputsPort.size() };
    auto const sizeOutputs{ mOutputsPort.size() };

    float * ins[MAX_INPUTS];
    float * outs[MAX_OUTPUTS];

    for (unsigned int i = 0; i < sizeInputs; i++) {
        ins[i] = AudioManager::getInstance().getBuffer(mInputsPort[i], nFrames);
    }
    for (unsigned int i = 0; i < sizeOutputs; i++) {
        outs[i] = AudioManager::getInstance().getBuffer(mOutputsPort[i], nFrames);
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
}

//==============================================================================
void JackClient::connectedGrisToSystem()
{
    clearOutput();
    auto & audioManager{ AudioManager::getInstance() };
    for (unsigned int i = 0; i < mMaxOutputPatch; i++) {
        juce::String nameOut{ "output" };
        nameOut += juce::String{ mOutputsPort.size() + 1 };

        auto * newPort{ audioManager.registerPort(nameOut.toStdString().c_str(), "SpatGRIS2", PortType::output) };

        mOutputsPort.push_back(newPort);
    }

    auto const outputPortNames{ audioManager.getPortNames(PortType::output) };
    auto const inputPortNames{ audioManager.getPortNames(PortType::input) };

    size_t i{};
    size_t j{};

    // DisConnect JackClientGris to system.
    // TODO : this is confusing
    while (i < outputPortNames.size()) {
        if (getClientName(outputPortNames[i].c_str()) == CLIENT_NAME) { // jackClient
            j = 0;
            auto * portOut{ audioManager.getPort(outputPortNames[i].c_str()) };
            while (j < inputPortNames.size()) {
                auto * portIn{ audioManager.getPort(inputPortNames[j].c_str()) };
                if (getClientName(inputPortNames[j].c_str()) == SYS_CLIENT_NAME
                    && audioManager.isConnectedTo(portOut, inputPortNames[j].c_str())) {
                    audioManager.disconnect(portOut, portIn);
                }
                ++j;
            }
        }
        ++i;
    }

    i = 0;
    j = 0;

    // Connect JackClientGris to system.
    while (i < outputPortNames.size()) {
        if (getClientName(outputPortNames[i].c_str()) == CLIENT_NAME) { // jackClient
            while (j < inputPortNames.size()) {
                if (getClientName(inputPortNames[j].c_str()) == SYS_CLIENT_NAME) { // system
                    audioManager.connect(outputPortNames[i].c_str(), inputPortNames[j].c_str());
                    ++j;
                    break;
                }
                ++j;
            }
        }
        ++i;
    }

    // Build output patch list.
    mOutputPatches.clear();
    for (i = 0; i < mOutputsPort.size(); ++i) {
        if (mSpeakersOut[i].outputPatch != 0) {
            mOutputPatches.push_back(mSpeakersOut[i].outputPatch);
        }
    }
}

//==============================================================================
bool JackClient::initSpeakersTriplet(std::vector<Speaker *> const & listSpk,
                                     int const dimensions,
                                     bool const needToComputeVbap)
{
    if (listSpk.empty()) {
        return false;
    }

    ls lss[MAX_LS_AMOUNT];
    int outputPatches[MAX_LS_AMOUNT];

    int j;
    for (unsigned i{}; i < listSpk.size(); ++i) {
        for (j = 0; j < MAX_LS_AMOUNT; ++j) {
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
        mParamVBap = init_vbap_from_speakers(lss,
                                             static_cast<int>(listSpk.size()),
                                             dimensions,
                                             outputPatches,
                                             static_cast<int>(mMaxOutputPatch),
                                             nullptr);
        if (mParamVBap == nullptr) {
            return false;
        }
    }

    for (unsigned i{}; i < MAX_INPUTS; ++i) {
        mSourcesIn[i].paramVBap = copy_vbap_data(mParamVBap);
    }

    int ** triplets;
    auto const num{ vbap_get_triplets(mSourcesIn[0].paramVBap, &triplets) };
    mVbapTriplets.clear();
    for (int i{}; i < num; ++i) {
        std::vector<int> row;
        for (j = 0; j < 3; ++j) {
            row.push_back(triplets[i][j]);
        }
        mVbapTriplets.push_back(row);
    }

    for (int i{}; i < num; i++) {
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
    if (listSpk.empty()) {
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

    auto * const speakers{
        lbap_speakers_from_positions(azimuth, elevation, radius, outputPatch, static_cast<int>(listSpk.size()))
    };

    lbap_field_reset(mLbapSpeakerField);
    lbap_field_setup(mLbapSpeakerField, speakers, static_cast<int>(listSpk.size()));

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
            vbap2(mSourcesIn[idS].azimuth, 0.0f, mSourcesIn[idS].azimuthSpan, 0.0f, mSourcesIn[idS].paramVBap);
        }
    }
}

//==============================================================================
void JackClient::connectionClient(juce::String const & name, bool connect)
{
    auto & audioManager{ AudioManager::getInstance() };
    auto inputPortNames{ audioManager.getPortNames(PortType::input) };
    auto outputPortNames{ audioManager.getPortNames(PortType::output) };

    updateClientPortAvailable(false);

    // Disconnect client.
    unsigned i{};
    while (i < outputPortNames.size()) {
        auto * portOut{ audioManager.getPort(outputPortNames[i].c_str()) };
        if (getClientName(outputPortNames[i].c_str()) == name) {
            int j{};
            while (j < inputPortNames.size()) {
                auto * portIn{ audioManager.getPort(inputPortNames[j].c_str()) };
                if (getClientName(inputPortNames[j].c_str()) == CLIENT_NAME
                    && audioManager.isConnectedTo(portIn, inputPortNames[j].c_str())) {
                    audioManager.disconnect(portOut, portIn);
                }
                j += 1;
            }
        }
        i += 1;
    }

    for (auto & cli : mClients) {
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

    auto conn{ false };
    for (auto & cli : mClients) {
        i = 0;
        unsigned j{};
        auto const & nameClient = cli.name;
        auto startJ{ cli.portStart - 1 };
        auto endJ{ cli.portEnd };

        while (i < outputPortNames.size()) {
            if (nameClient == name && nameClient.compare(getClientName(outputPortNames[i].c_str())) == 0) {
                while (inputPortNames[j].c_str()) {
                    if (getClientName(inputPortNames[j].c_str()) == CLIENT_NAME) {
                        if (j >= startJ && j < endJ) {
                            audioManager.connect(outputPortNames[i].c_str(), inputPortNames[j].c_str());
                            conn = true;
                            j += 1;
                            break;
                        }
                        j += 1;

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
}

//==============================================================================
std::string JackClient::getClientName(const char * portName)
{
    jassert(portName != nullptr);
    auto * port{ AudioManager::getInstance().getPort(portName) };

    std::string const nameClient{ port->fullName };
    std::string const tempN{ port->shortName };
    return nameClient.substr(0, nameClient.size() - (tempN.size() + 1));
}

//==============================================================================
void JackClient::updateClientPortAvailable(bool const fromJack)
{
    auto const portsOut{ AudioManager::getInstance().getPortNames(PortType::output) };

    for (auto & client : mClients) {
        client.portAvailable = 0;
    }

    unsigned i{};
    while (i < portsOut.size()) {
        auto const clientName{ getClientName(portsOut[i].c_str()) };
        if (clientName != CLIENT_NAME && clientName != SYS_CLIENT_NAME) {
            for (auto & client : mClients) {
                if (client.name.compare(clientName) == 0) {
                    client.portAvailable += 1;
                }
            }
        }
        i++;
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
JackClient::~JackClient()
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
