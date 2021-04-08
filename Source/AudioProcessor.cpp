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

#include "AudioProcessor.h"

#include <array>
#include <cstdarg>

#include "AudioManager.h"
#include "MainComponent.h"
#include "PinkNoiseGenerator.h"
#include "Speaker.h"
#include "StaticMap.hpp"
#include "TaggedAudioBuffer.hpp"
#include "constants.hpp"
#include "narrow.hpp"
#include "vbap.hpp"

float constexpr SMALL_GAIN = 0.0000000000001f;
size_t constexpr MAX_BUFFER_SIZE = 2048;
size_t constexpr LEFT = 0;
size_t constexpr RIGHT = 1;

//==============================================================================
// Load samples from a wav file into a float array.
static juce::AudioBuffer<float> getSamplesFromWavFile(juce::File const & file)
{
    if (!file.existsAsFile()) {
        auto const error{ file.getFullPathName() + "\n\nTry re-installing SpatGRIS." };
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
AudioProcessor::AudioProcessor()
{
    auto & hrtf{ mAudioData.state.hrtf };

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
        std::memcpy(hrtf.leftImpulses[i].data(), stbuf.getReadPointer(leftChannel), 128);
        std::memcpy(hrtf.rightImpulses[i].data(), stbuf.getReadPointer(rightChannel), 128);
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
        std::memcpy(hrtf.leftImpulses[i + 8].data(), stbuf.getReadPointer(leftChannel), 128);
        std::memcpy(hrtf.rightImpulses[i + 8].data(), stbuf.getReadPointer(rightChannel), 128);
    }
    // Azimuth = 80
    for (int i = 0; i < 2; i++) {
        auto const file{ HRTF_FOLDER_80.getChildFile("H80e090a.wav") };
        auto const stbuf{ getSamplesFromWavFile(file) };
        auto const leftChannel{ 1 - i };
        auto const rightChannel{ i };
        std::memcpy(hrtf.leftImpulses[i + 14].data(), stbuf.getReadPointer(leftChannel), 128);
        std::memcpy(hrtf.rightImpulses[i + 14].data(), stbuf.getReadPointer(rightChannel), 128);
    }

    // Initialize pink noise
    srand(static_cast<unsigned>(time(nullptr)));
}

//==============================================================================
void AudioProcessor::resetHrtf()
{
    auto & hrtf{ mAudioData.state.hrtf };
    std::fill(hrtf.count.begin(), hrtf.count.end(), 0u);
    static constexpr std::array<float, 128> EMPTY_HRTF_INPUT{};
    std::fill(hrtf.inputTmp, hrtf.inputTmp, EMPTY_HRTF_INPUT);
}

//==============================================================================
void AudioProcessor::setAudioConfig(AudioConfig const & audioConfig)
{
    JUCE_ASSERT_MESSAGE_THREAD;
    juce::ScopedLock const lock{ mCriticalSection };
    mAudioData.config = audioConfig;
}

//==============================================================================
SourcePeaks AudioProcessor::muteSoloVuMeterIn(SourceAudioBuffer & inputBuffer) const noexcept
{
    SourcePeaks peaks{};
    for (auto const channel : inputBuffer) {
        auto const & config{ mAudioData.config.sourcesAudioConfig[channel.key] };
        auto const & buffer{ *channel.value };
        auto const peak{ config.isMuted ? 0.0f : buffer.getMagnitude(0, inputBuffer.getNumSamples()) };

        peaks.add(channel.key, peak);
    }
    return peaks;
}

//==============================================================================
SpeakerPeaks AudioProcessor::muteSoloVuMeterGainOut(SpeakerAudioBuffer & speakersBuffer) noexcept
{
    auto const numSamples{ speakersBuffer.getNumSamples() };
    SpeakerPeaks peaks{};

    for (auto const channel : speakersBuffer) {
        auto const & config{ mAudioData.config.speakersAudioConfig[channel.key] };
        auto & buffer{ *channel.value };
        if (config.isMuted || config.gain < SMALL_GAIN) {
            buffer.clear();
            peaks.add(channel.key, 0.0f);
            continue;
        }

        buffer.applyGain(0, numSamples, config.gain);

        if (config.highpassConfig) {
            auto * const samples{ buffer.getWritePointer(0) };
            auto const & highpassConfig{ *config.highpassConfig };
            auto & highpassVars{ mAudioData.state.speakersAudioState[channel.key].highpassState };
            highpassConfig.process(samples, numSamples, highpassVars);
        }

        auto const magnitude{ buffer.getMagnitude(0, numSamples) };
        peaks.add(channel.key, magnitude);
    }

    return peaks;
}

//==============================================================================
void AudioProcessor::processVbap(SourceAudioBuffer const & inputBuffer,
                                 SpeakerAudioBuffer & outputBuffer,
                                 SourcePeaks const & sourcePeaks) noexcept
{
    auto const & gainInterpolation{ mAudioData.config.spatGainsInterpolation };
    auto const gainFactor{ std::pow(gainInterpolation, 0.1f) * 0.0099f + 0.99f };

    auto const numSamples{ inputBuffer.getNumSamples() };
    for (auto const & source : mAudioData.config.sourcesAudioConfig) {
        if (source.value.isMuted || source.value.directOut || sourcePeaks[source.key] < SMALL_GAIN) {
            continue;
        }
        auto const & gains{ *mAudioData.spatGainMatrix[source.key].get() };
        auto & lastGains{ mAudioData.state.sourcesAudioState[source.key].lastSpatGains };
        auto const * inputSamples{ inputBuffer[source.key].getReadPointer(0) };

        for (auto const & speaker : mAudioData.config.speakersAudioConfig) {
            if (speaker.value.isMuted || speaker.value.isDirectOutOnly || speaker.value.gain < SMALL_GAIN) {
                continue;
            }
            auto & currentGain{ lastGains[speaker.key] };
            auto const & targetGain{ gains[speaker.key] };
            auto * outputSamples{ outputBuffer[speaker.key].getWritePointer(0) };
            if (gainInterpolation == 0.0f) {
                // linear interpolation over buffer size
                auto const gainSlope = (targetGain - currentGain) / narrow<float>(numSamples);
                if (targetGain < SMALL_GAIN && currentGain < SMALL_GAIN) {
                    // this is not going to produce any more sounds!
                    continue;
                }
                for (int sampleIndex{}; sampleIndex < numSamples; ++sampleIndex) {
                    currentGain += gainSlope;
                    outputSamples[sampleIndex] += inputSamples[sampleIndex] * currentGain;
                }
            } else {
                // log interpolation with 1st order filter
                for (int sampleIndex{}; sampleIndex < numSamples; ++sampleIndex) {
                    currentGain = targetGain + (currentGain - targetGain) * gainFactor;
                    if (currentGain < SMALL_GAIN && targetGain < SMALL_GAIN) {
                        // If the gain is near zero and the target gain is also near zero, this means that
                        // currentGain will no ever increase over this buffer
                        break;
                    }
                    outputSamples[sampleIndex] += inputSamples[sampleIndex] * currentGain;
                }
            }
        }
    }
}

//==============================================================================
void AudioProcessor::processLbap(SourceAudioBuffer & inputBuffer,
                                 SpeakerAudioBuffer & outputBuffer,
                                 SourcePeaks const & sourcePeaks) noexcept
{
    std::array<float, MAX_BUFFER_SIZE> filteredInputSignal{};

    auto const & gainInterpolation{ mAudioData.config.spatGainsInterpolation };
    auto const gainFactor{ std::pow(gainInterpolation, 0.1f) * 0.0099f + 0.99f };
    auto const & attenuationConfig{ mAudioData.config.lbapAttenuationConfig };
    auto const numSamples{ inputBuffer.getNumSamples() };

    for (auto const source : mAudioData.config.sourcesAudioConfig) {
        if (source.value.isMuted || source.value.directOut || sourcePeaks[source.key] < SMALL_GAIN) {
            continue;
        }

        // process attenuation

        // Energy is lost with distance.
        // A radius < 1 is not impact sound
        // Radius between 1 and 1.66 are reduced
        // Radius over 1.66 is clamped to 1.66
        // auto const distance{ std::clamp((source.radius - 1.0f) / (LBAP_EXTENDED_RADIUS - 1.0f), 0.0f, 1.0f) };
        auto const distance{ mAudioData.lbapSourceDistances[source.key].get() };
        auto const distanceGain{ (1.0f - distance) * (1.0f - attenuationConfig.linearGain)
                                 + attenuationConfig.linearGain };
        auto const distanceCoefficient{ distance * attenuationConfig.lowpassCoefficient };
        auto & attenuationState{ mAudioData.state.sourcesAudioState[source.key].lbapAttenuationState };
        auto const diffGain{ (distanceGain - attenuationState.lastGain) / narrow<float>(numSamples) };
        auto const diffCoefficient
            = (distanceCoefficient - attenuationState.lastCoefficient) / narrow<float>(numSamples);
        auto filterInY{ attenuationState.lowpassY };
        auto filterInZ{ attenuationState.lowpassZ };
        auto lastCoefficient{ attenuationState.lastCoefficient };
        auto lastGain{ attenuationState.lastGain };
        // TODO : this could be greatly optimized
        auto const * inputSamples{ inputBuffer[source.key].getReadPointer(0) };
        if (diffCoefficient == 0.0f && diffGain == 0.0f) {
            // simplified version
            for (int sampleIndex{}; sampleIndex < numSamples; ++sampleIndex) {
                filterInY = inputSamples[sampleIndex] + (filterInY - inputSamples[sampleIndex]) * lastCoefficient;
                filterInZ = filterInY + (filterInZ - filterInY) * lastCoefficient;
                filteredInputSignal[sampleIndex] = filterInZ * lastGain;
            }
        } else {
            // full version
            for (int sampleIndex{}; sampleIndex < numSamples; ++sampleIndex) {
                lastCoefficient += diffCoefficient;
                lastGain += diffGain;
                filterInY = inputSamples[sampleIndex] + (filterInY - inputSamples[sampleIndex]) * lastCoefficient;
                filterInZ = filterInY + (filterInZ - filterInY) * lastCoefficient;
                filteredInputSignal[sampleIndex] = filterInZ * lastGain;
            }
        }
        attenuationState.lowpassY = filterInY;
        attenuationState.lowpassZ = filterInZ;
        attenuationState.lastGain = distanceGain;
        attenuationState.lastCoefficient = distanceCoefficient;

        // Process spatialization
        auto const & gains{ *mAudioData.spatGainMatrix[source.key].get() };
        auto & lastGains{ mAudioData.state.sourcesAudioState[source.key].lastSpatGains };

        for (auto const speaker : mAudioData.config.speakersAudioConfig) {
            auto * outputSamples{ outputBuffer[speaker.key].getWritePointer(0) };
            auto const & targetGain{ gains[speaker.key] };
            auto & currentGain{ lastGains[speaker.key] };
            if (gainInterpolation == 0.0f) {
                // linear interpolation over buffer size
                auto const gainSlope = (targetGain - currentGain) / narrow<float>(numSamples);
                if (currentGain < SMALL_GAIN && targetGain < SMALL_GAIN) {
                    // This is not going to produce any more sounds!
                    continue;
                }
                for (int sampleIndex{}; sampleIndex < numSamples; ++sampleIndex) {
                    currentGain += gainSlope;
                    outputSamples[sampleIndex] += filteredInputSignal[sampleIndex] * currentGain;
                }
            } else {
                // log interpolation with 1st order filter
                for (int sampleIndex{}; sampleIndex < numSamples; ++sampleIndex) {
                    currentGain = (currentGain - targetGain) * gainFactor + targetGain;
                    if (currentGain < SMALL_GAIN && targetGain < SMALL_GAIN) {
                        // If the gain is near zero and the target gain is also near zero, this means that
                        // currentGain will no ever increase over this buffer
                        break;
                    }
                    outputSamples[sampleIndex] += filteredInputSignal[sampleIndex] * currentGain;
                }
            }
        }
    }
}

//==============================================================================
void AudioProcessor::processVBapHrtf(SourceAudioBuffer const & inputBuffer,
                                     SpeakerAudioBuffer & outputBuffer,
                                     SourcePeaks const & sourcePeaks) noexcept
{
    jassert(outputBuffer.size() == 16);

    processVbap(inputBuffer, outputBuffer, sourcePeaks);

    auto const numSamples{ inputBuffer.getNumSamples() };

    // Process hrtf and mixdown to stereo
    std::array<float, MAX_BUFFER_SIZE> leftOutputSamples{};
    std::array<float, MAX_BUFFER_SIZE> rightOutputSamples{};
    for (auto const speaker : mAudioData.config.speakersAudioConfig) {
        auto & hrtfState{ mAudioData.state.hrtf };
        auto & hrtfCount{ hrtfState.count };
        auto & hrtfInputTmp{ hrtfState.inputTmp };
        auto const outputIndex{ speaker.key.removeOffset<size_t>() };
        auto const * outputSamples{ outputBuffer[speaker.key].getReadPointer(0) };
        auto const & hrtfLeftImpulses{ hrtfState.leftImpulses };
        auto const & hrtfRightImpulses{ hrtfState.rightImpulses };
        for (size_t sampleIndex{}; sampleIndex < narrow<size_t>(numSamples); ++sampleIndex) {
            auto tmpCount{ narrow<int>(hrtfCount[outputIndex]) };
            for (unsigned hrtfIndex{}; hrtfIndex < HRTF_NUM_SAMPLES; ++hrtfIndex) {
                if (tmpCount < 0) {
                    tmpCount += HRTF_NUM_SAMPLES;
                }
                auto const sig{ hrtfInputTmp[outputIndex][tmpCount] };
                leftOutputSamples[sampleIndex] += sig * hrtfLeftImpulses[outputIndex][hrtfIndex];
                rightOutputSamples[sampleIndex] += sig * hrtfRightImpulses[outputIndex][hrtfIndex];
                --tmpCount;
            }
            hrtfCount[outputIndex]++;
            if (hrtfCount[outputIndex] >= HRTF_NUM_SAMPLES) {
                hrtfCount[outputIndex] = 0;
            }
            hrtfInputTmp[outputIndex][hrtfCount[outputIndex]] = outputSamples[sampleIndex];
        }
    }

    static constexpr output_patch_t LEFT_OUTPUT_PATCH{ 1 };
    static constexpr output_patch_t RIGHT_OUTPUT_PATCH{ 2 };
    for (auto const speaker : mAudioData.config.speakersAudioConfig) {
        if (speaker.key == LEFT_OUTPUT_PATCH) {
            outputBuffer[LEFT_OUTPUT_PATCH].copyFrom(0, 0, leftOutputSamples.data(), numSamples);
        } else if (speaker.key == RIGHT_OUTPUT_PATCH) {
            outputBuffer[RIGHT_OUTPUT_PATCH].copyFrom(0, 0, leftOutputSamples.data(), numSamples);
        } else {
            outputBuffer[speaker.key].clear();
        }
    }
}

//==============================================================================
void AudioProcessor::processStereo(SourceAudioBuffer const & inputBuffer,
                                   SpeakerAudioBuffer & outputBuffer,
                                   SourcePeaks const & sourcePeaks) noexcept
{
    jassert(outputBuffer.size() == 2);

    auto const gainFactor{ std::pow(mAudioData.config.spatGainsInterpolation, 0.1f) * 0.0099f + 0.99f };

    auto * leftOutputSamples{ outputBuffer[output_patch_t{ 1 }].getWritePointer(0) };
    auto * rightOutputSamples{ outputBuffer[output_patch_t{ 2 }].getWritePointer(0) };
    auto const numSamples{ inputBuffer.getNumSamples() };

    auto const & sources{ mAudioData.config.sourcesAudioConfig };
    for (auto const source : sources) {
        // Is empty?
        if (source.value.isMuted || source.value.directOut || sourcePeaks[source.key] < SMALL_GAIN) {
            continue;
        }

        auto const * inputSamples{ inputBuffer[source.key].getReadPointer(0) };

        // Process
        auto const azimuth{ mAudioData.stereoSourceAzimuths[source.key].get() };
        auto & lastAzimuth{ mAudioData.state.sourcesAudioState[source.key].stereoLastAzimuth };
        for (int sampleIndex{}; sampleIndex < numSamples; ++sampleIndex) {
            // Removes the chirp at 180->-180 degrees azimuth boundary.
            if ((lastAzimuth - azimuth).abs() > degrees_t{ 300.0f }) {
                lastAzimuth = azimuth;
            }
            lastAzimuth = azimuth + (lastAzimuth - azimuth) * gainFactor;
            radians_t scaled;
            static constexpr radians_t NINETY_DEG{ degrees_t{ 90.0f } };
            if (lastAzimuth < -NINETY_DEG) {
                scaled = -NINETY_DEG - (lastAzimuth + NINETY_DEG);
            } else if (lastAzimuth > NINETY_DEG) {
                scaled = NINETY_DEG - (lastAzimuth - NINETY_DEG);
            } else {
                scaled = lastAzimuth;
            }
            scaled += NINETY_DEG;
            using fast = juce::dsp::FastMathApproximations;
            leftOutputSamples[sampleIndex] += inputSamples[sampleIndex] * fast::cos(scaled.get());
            rightOutputSamples[sampleIndex] += inputSamples[sampleIndex] * fast::sin(scaled.get());
        }
    }

    // Apply gain compensation.
    auto const compensation{ std::pow(10.0f, (narrow<float>(sources.size()) - 1.0f) * -0.1f * 0.05f) };
    auto const applyCompensation = [compensation](float & sample) { sample *= compensation; };
    std::for_each_n(leftOutputSamples, numSamples, applyCompensation);
    std::for_each_n(rightOutputSamples, numSamples, applyCompensation);
}

//==============================================================================
void AudioProcessor::processAudio(SourceAudioBuffer & sourceBuffer, SpeakerAudioBuffer & speakerBuffer) noexcept
{
    // Skip if the user is editing the speaker setup.
    juce::ScopedTryLock const lock{ getCriticalSection() };
    if (!lock.isLocked()) {
        return;
    }

    jassert(sourceBuffer.getNumSamples() == speakerBuffer.getNumSamples());
    auto const numSamples{ sourceBuffer.getNumSamples() };

    // Process source peaks
    auto * sourcePeaks{ mAudioData.sourcePeaks.pool.acquire() };
    *sourcePeaks = muteSoloVuMeterIn(sourceBuffer);

    if (mAudioData.config.pinkNoiseGain) {
        // Process pink noise
        StaticVector<output_patch_t, MAX_OUTPUTS> activeChannels{};
        for (auto channel : mAudioData.config.speakersAudioConfig) {
            activeChannels.push_back(channel.key);
        }
        auto data{ speakerBuffer.getArrayOfWritePointers(activeChannels) };
        fillWithPinkNoise(data.data(), numSamples, data.size(), *mAudioData.config.pinkNoiseGain);
    } else {
        // Process spat algorithm
        switch (mAudioData.config.spatMode) {
        case SpatMode::vbap:
            processVbap(sourceBuffer, speakerBuffer, *sourcePeaks);
            break;
        case SpatMode::lbap:
            processLbap(sourceBuffer, speakerBuffer, *sourcePeaks);
            break;
        case SpatMode::hrtfVbap:
            processVBapHrtf(sourceBuffer, speakerBuffer, *sourcePeaks);
            break;
        case SpatMode::stereo:
            processStereo(sourceBuffer, speakerBuffer, *sourcePeaks);
            break;
        default:
            jassertfalse;
            break;
        }

        // Process direct outs
        for (auto const & directOutPair : mAudioData.config.directOutPairs) {
            auto const & origin{ sourceBuffer[directOutPair.first] };
            auto & dest{ speakerBuffer[directOutPair.second] };
            dest.addFrom(0, 0, origin, 0, 0, numSamples);
        }
    }

    // Process speaker highpass

    // Process speaker peaks
    auto * speakerPeaks{ mAudioData.speakerPeaks.pool.acquire() };
    *speakerPeaks = muteSoloVuMeterGainOut(speakerBuffer);

    // return peaks data to message thread
    mAudioData.sourcePeaks.set(sourcePeaks);
    mAudioData.speakerPeaks.set(speakerPeaks);
}

//==============================================================================
bool AudioProcessor::initSpeakersTriplet(std::vector<Speaker const *> const & listSpk,
                                         int const dimensions,
                                         bool const needToComputeVbap)
{
}

//==============================================================================
bool AudioProcessor::lbapSetupSpeakerField(std::vector<Speaker const *> const & listSpk)
{
    jassert(
        std::none_of(listSpk.cbegin(), listSpk.cend(), [](Speaker const * speaker) { return speaker->isDirectOut(); }));

    if (listSpk.empty()) {
        return false;
    }

    for (auto const * speaker : listSpk) {
        auto * const * const matchingSpeakerDataIt{ std::find_if(
            mSpeakersOut.begin(),
            mSpeakersOut.end(),
            [outputPatch = speaker->getOutputPatch()](SpeakerData const * speakerData) -> bool {
                return !speakerData->directOut && outputPatch == speakerData->outputPatch;
            }) };
        if (matchingSpeakerDataIt == mSpeakersOut.end()) {
            continue;
        }
        auto & matchingSpeaker{ **matchingSpeakerDataIt };
        --matchingSpeaker.outputPatch;
    }

    auto speakers{ lbap_speakers_from_positions(mSpeakersOut) };

    mLbapSpeakerField.reset();
    lbap_field_setup(mLbapSpeakerField, speakers);

    return true;
}

//==============================================================================
void AudioProcessor::setAttenuationFrequencyIndex(int const index)
{
    jassert(index >= 0 && index < ATTENUATION_FREQUENCY_STRINGS.size());
    auto * audioDevice{ AudioManager::getInstance().getAudioDeviceManager().getCurrentAudioDevice() };
    jassert(audioDevice);
    if (!audioDevice) {
        return;
    }
    auto const coefficient{ std::exp(-juce::MathConstants<float>::twoPi
                                     * ATTENUATION_FREQUENCY_STRINGS[index].getFloatValue()
                                     / narrow<float>(audioDevice->getCurrentSampleRate())) };
    mAttenuationLowpassCoefficient = coefficient;
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
            vbap2(mSourcesData[idS].azimuth,
                  degrees_t{},
                  mSourcesData[idS].azimuthSpan,
                  0.0f,
                  mSourcesData[idS].paramVBap);
        }
    }
}

//==============================================================================
AudioProcessor::~AudioProcessor()
{
    free_vbap_data(mParamVBap);
    AudioManager::getInstance().getAudioDeviceManager().getCurrentAudioDevice()->close();
}
