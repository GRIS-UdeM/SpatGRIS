/*
 This file is part of SpatGRIS.

 Developers: Samuel B�land, Olivier B�langer, Nicolas Masson

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
AudioProcessor::AudioProcessor(Manager<Speaker, speaker_id_t> const & speakers, juce::OwnedArray<Input> const & inputs)
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

    mInterMaster = 0.8f;

    auto & audioManager{ AudioManager::getInstance() };
    audioManager.registerAudioProcessor(this, speakers, inputs);

    // Initialize pink noise
    srand(static_cast<unsigned>(time(nullptr)));
}

//==============================================================================
void AudioProcessor::resetHrtf()
{
    std::fill(std::begin(mHrtfCount), std::end(mHrtfCount), 0u);
    static constexpr std::array<float, 128> EMPTY_HRTF_INPUT{};
    std::fill(std::begin(mHrtfInputTmp), std::end(mHrtfInputTmp), EMPTY_HRTF_INPUT);
}

//==============================================================================
std::vector<output_patch_t> AudioProcessor::getDirectOutOutputPatches() const
{
    std::vector<output_patch_t> directOutOutputPatches;
    for (auto const * speaker : mSpeakersOut) {
        if (speaker->directOut && speaker->outputPatch != output_patch_t{ 0 })
            directOutOutputPatches.push_back(speaker->outputPatch);
    }
    return directOutOutputPatches;
}

//==============================================================================
void AudioProcessor::muteSoloVuMeterIn(juce::AudioBuffer<float> & inputBuffer) noexcept
{
    auto const numSamples{ inputBuffer.getNumSamples() };
    int index{};
    for (auto & sourceData : mSourcesData) {
        if (sourceData.isMuted || (mSoloIn && !sourceData.isSolo)) {
            inputBuffer.clear(index, 0, numSamples);
            sourceData.magnitude = 0.0f;
        } else {
            sourceData.magnitude = inputBuffer.getMagnitude(index, 0, numSamples);
        }

        ++index;
    }
}

//==============================================================================
void AudioProcessor::muteSoloVuMeterGainOut(TaggedAudioBuffer<MAX_OUTPUTS> & outputBuffer,
                                            int const numSamples,
                                            float const gain) noexcept
{
    jassert(outputBuffer.getNumChannels());
    for (auto * speakerOut : mSpeakersOut) {
        auto buffer{ outputBuffer.getChannel(speakerOut->id, numSamples) };

        // Mute / Solo
        if (speakerOut->isMuted || (mSoloOut && !speakerOut->isSolo)) {
            buffer.clear();
        }

        // Speaker independent gain
        auto const outputGain{ speakerOut->gain * gain };
        buffer.applyGain(outputGain);

        // Speaker independent crossover
        if (speakerOut->crossoverPassiveData) {
            auto const & passiveData{ *speakerOut->crossoverPassiveData };
            auto & activeData{ speakerOut->crossoverActiveData };
            auto * const samples{ buffer.getWritePointer(0) };
            for (int sampleIndex{}; sampleIndex < numSamples; ++sampleIndex) {
                auto const inval{ static_cast<double>(samples[sampleIndex]) };
                auto const val{ passiveData.ha0 * inval + passiveData.ha1 * activeData.x1
                                + passiveData.ha2 * activeData.x2 + passiveData.ha1 * activeData.x3
                                + passiveData.ha0 * activeData.x4 - passiveData.b1 * activeData.y1
                                - passiveData.b2 * activeData.y2 - passiveData.b3 * activeData.y3
                                - passiveData.b4 * activeData.y4 };
                activeData.y4 = activeData.y3;
                activeData.y3 = activeData.y2;
                activeData.y2 = activeData.y1;
                activeData.y1 = val;
                activeData.x4 = activeData.x3;
                activeData.x3 = activeData.x2;
                activeData.x2 = activeData.x1;
                activeData.x1 = inval;
                samples[sampleIndex] = static_cast<float>(val);
            }
        }

        // VuMeter
        speakerOut->magnitude = buffer.getMagnitude(0, numSamples);
    }
}

//==============================================================================
void AudioProcessor::processVbap(juce::AudioBuffer<float> const & inputBuffer,
                                 TaggedAudioBuffer<MAX_OUTPUTS> & outputBuffer) noexcept
{
    int index{};
    for (auto & source : mSourcesData) {
        if (source.shouldUpdateVbap) {
            updateSourceVbap(index);
            source.shouldUpdateVbap = false;
        }
        ++index;
    }

    auto const gainFactor{ std::pow(mInterMaster, 0.1f) * 0.0099f + 0.99f };

    auto const numSamples{ inputBuffer.getNumSamples() };
    for (auto * speaker : mSpeakersOut) {
        auto * outputSamples{ outputBuffer.getWritePointer(speaker->id) };
        for (int sourceIndex{}; sourceIndex < mSourcesData.size(); ++sourceIndex) {
            // Is input empty?
            auto const & source{ mSourcesData[sourceIndex] };
            if (source.magnitude < SMALL_GAIN) {
                // nothing to process
                continue;
            }

            // Is direct out?
            auto const * inputSamples{ inputBuffer.getReadPointer(sourceIndex) };
            if (source.directOut || source.paramVBap == nullptr) {
                // direct out
                jassert(source.directOut);
                if (*source.directOut == speaker->outputPatch) {
                    // output patch matches
                    std::transform(inputSamples, inputSamples + numSamples, outputSamples, outputSamples, std::plus());
                }
                continue;
            }

            // Process audio
            // spat
            auto const outputIndex{ narrow<size_t>(speaker->outputPatch.get() - 1) };
            auto currentGain{ source.paramVBap->gainsSmoothing[outputIndex] };
            auto const targetGain{ source.paramVBap->gains[outputIndex] };
            if (mInterMaster == 0.0f) {
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
            source.paramVBap->gainsSmoothing[outputIndex] = currentGain;
        }
    }
}

//==============================================================================
void AudioProcessor::processLbap(juce::AudioBuffer<float> const & inputBuffer,
                                 TaggedAudioBuffer<MAX_OUTPUTS> & outputBuffer) noexcept
{
    std::array<float, MAX_BUFFER_SIZE> filteredInputSignal{};

    auto const gainFactor{ std::pow(mInterMaster, 0.1f) * 0.0099f + 0.99f };

    for (int sourceIndex{}; sourceIndex < mSourcesData.size(); ++sourceIndex) {
        auto & source{ mSourcesData[sourceIndex] };

        // Is input empty?
        if (source.magnitude < SMALL_GAIN) {
            // nothing to process
            continue;
        }

        // Is direct out?
        auto const numSamples{ inputBuffer.getNumSamples() };
        auto const * inputSamples{ inputBuffer.getReadPointer(sourceIndex) };
        if (source.directOut) {
            for (auto * speaker : mSpeakersOut) {
                if (*source.directOut == speaker->outputPatch) {
                    auto * outputSamples{ outputBuffer.getWritePointer(speaker->id) };
                    std::transform(inputSamples, inputSamples + numSamples, outputSamples, outputSamples, std::plus());
                    break;
                }
            }
            continue;
        }

        // process
        auto pos{ lbap_pos_init_from_radians(source.radAzimuth, source.radElevation, source.radius) };
        pos.radiusSpan = source.azimuthSpan;
        pos.elevationSpan = source.zenithSpan;
        // Energy is lost with distance.
        // A radius < 1 is not impact sound
        // Radius between 1 and 1.66 are reduced
        // Radius over 1.66 is clamped to 1.66
        auto const distance{ std::clamp((source.radius - 1.0f) / (LBAP_EXTENDED_RADIUS - 1.0f), 0.0f, 1.0f) };
        if (pos != source.lbapLastPos) {
            lbap_field_compute(mLbapSpeakerField, pos, source.lbapGains.data());
            source.lbapLastPos = pos;
        }
        auto const distanceGain{ (1.0f - distance) * (1.0f - mAttenuationLinearGain) + mAttenuationLinearGain };
        auto const distanceCoefficient{ distance * mAttenuationLowpassCoefficient };
        auto const diffGain{ (distanceGain - source.attenuationData.lastGain) / narrow<float>(numSamples) };
        auto const diffCoefficient
            = (distanceCoefficient - source.attenuationData.lastCoefficient) / narrow<float>(numSamples);
        auto filterInY{ source.attenuationData.lowpassY };
        auto filterInZ{ source.attenuationData.lowpassZ };
        auto lastCoefficient{ source.attenuationData.lastCoefficient };
        auto lastGain{ source.attenuationData.lastGain };
        // TODO : this could be greatly optimized
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
        source.attenuationData.lowpassY = filterInY;
        source.attenuationData.lowpassZ = filterInZ;
        source.attenuationData.lastGain = distanceGain;
        source.attenuationData.lastCoefficient = distanceCoefficient;
        //==============================================================================
        for (auto const & speaker : mSpeakersOut) {
            auto * outputSamples{ outputBuffer.getWritePointer(speaker->id) };
            auto const outputIndex{ narrow<size_t>(speaker->outputPatch.get() - 1) };
            auto const targetGain{ source.lbapGains[outputIndex] };
            auto currentGain{ source.lbapY[outputIndex] };
            if (mInterMaster == 0.0f) {
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
            source.lbapY[outputIndex] = currentGain;
        }
    }
}

//==============================================================================
void AudioProcessor::processVBapHrtf(juce::AudioBuffer<float> const & inputBuffer,
                                     TaggedAudioBuffer<MAX_OUTPUTS> & outputBuffer) noexcept
{
    jassert(outputBuffer.getNumChannels() == 16);

    // Update vbap data
    {
        int index{};
        for (auto & source : mSourcesData) {
            if (source.shouldUpdateVbap) {
                updateSourceVbap(index);
                source.shouldUpdateVbap = false;
            }
            ++index;
        }
    }

    auto const gainFactor{ std::pow(mInterMaster, 0.1f) * 0.0099f + 0.99f };
    auto const numSamples{ inputBuffer.getNumSamples() };
    auto * leftOutputSamples{ outputBuffer.getWritePointerByOutputPatch(output_patch_t{ 1 }) };
    auto * rightOutputSamples{ outputBuffer.getWritePointerByOutputPatch(output_patch_t{ 2 }) };
    for (auto const * speaker : mSpeakersOut) {
        jassert(numSamples <= narrow<int>(MAX_BUFFER_SIZE));
        std::array<float, MAX_BUFFER_SIZE> vbapOuts{};
        for (int sourceIndex{}; sourceIndex < inputBuffer.getNumChannels(); ++sourceIndex) {
            auto const & source{ mSourcesData[sourceIndex] };

            // Is empty?
            if (source.magnitude < SMALL_GAIN) {
                continue;
            }

            // Process vbap
            auto const outputIndex{ narrow<size_t>(speaker->outputPatch.get() - 1) };
            if (!source.directOut && source.paramVBap) {
                auto const targetGain{ source.paramVBap->gains[outputIndex] };
                auto currentGain{ source.paramVBap->gainsSmoothing[outputIndex] };
                auto const * inputSamples{ inputBuffer.getReadPointer(sourceIndex) };
                if (mInterMaster == 0.0f) {
                    // linear interpolation
                    if (currentGain < SMALL_GAIN && targetGain < SMALL_GAIN) {
                        continue;
                    }
                    auto const gainSlope = (targetGain - currentGain) / narrow<float>(numSamples);
                    for (int sampleIndex{}; sampleIndex < numSamples; ++sampleIndex) {
                        currentGain += gainSlope;
                        vbapOuts[sampleIndex] += inputSamples[sampleIndex] * currentGain;
                    }
                } else {
                    // log interpolation with 1st order filter
                    for (int sampleIndex{}; sampleIndex < numSamples; ++sampleIndex) {
                        currentGain = targetGain + (currentGain - targetGain) * gainFactor;
                        if (currentGain < SMALL_GAIN && targetGain < SMALL_GAIN) {
                            break;
                        }
                        vbapOuts[sampleIndex] += inputSamples[sampleIndex] * currentGain;
                    }
                }
                source.paramVBap->gainsSmoothing[outputIndex] = currentGain;
            }

            // Process hrtf and mixdown to stereo
            for (size_t sampleIndex{}; sampleIndex < narrow<size_t>(numSamples); ++sampleIndex) {
                auto tmpCount{ narrow<int>(mHrtfCount[outputIndex]) };
                for (unsigned hrtfIndex{}; hrtfIndex < HRTF_NUM_SAMPLES; ++hrtfIndex) {
                    if (tmpCount < 0) {
                        tmpCount += HRTF_NUM_SAMPLES;
                    }
                    auto const sig{ mHrtfInputTmp[outputIndex][tmpCount] };
                    leftOutputSamples[sampleIndex] += sig * mVbapHrtfLeftImpulses[outputIndex][hrtfIndex];
                    rightOutputSamples[sampleIndex] += sig * mVbapHrtfRightImpulses[outputIndex][hrtfIndex];
                    --tmpCount;
                }
                mHrtfCount[outputIndex]++;
                if (mHrtfCount[outputIndex] >= HRTF_NUM_SAMPLES) {
                    mHrtfCount[outputIndex] = 0;
                }
                mHrtfInputTmp[outputIndex][mHrtfCount[outputIndex]] = vbapOuts[sampleIndex];
            }
        }

        // Add direct outs to the now stereo signal.
        for (int inputIndex{}; inputIndex < inputBuffer.getNumChannels(); ++inputIndex) {
            auto const & source{ mSourcesData[inputIndex] };
            auto const * inputSamples{ inputBuffer.getReadPointer(inputIndex) };
            if (source.directOut) {
                if (source.directOut->get() % 2 == 1) {
                    for (int sampleIndex{}; sampleIndex < numSamples; ++sampleIndex) {
                        leftOutputSamples[sampleIndex] += inputSamples[sampleIndex];
                    }
                } else {
                    for (int sampleIndex{}; sampleIndex < numSamples; ++sampleIndex) {
                        rightOutputSamples[sampleIndex] += inputSamples[sampleIndex];
                    }
                }
            }
        }
    }
}

//==============================================================================
void AudioProcessor::processStereo(juce::AudioBuffer<float> const & inputBuffer,
                                   TaggedAudioBuffer<MAX_OUTPUTS> & outputBuffer) noexcept
{
    jassert(mSpeakersOut.size() == 2);

    static auto constexpr FACTOR{ juce::MathConstants<float>::pi / 360.0f };
    auto const gainFactor{ std::pow(mInterMaster, 0.1f) * 0.0099f + 0.99f };

    auto * leftOutputSamples{ outputBuffer.getWritePointerByOutputPatch(output_patch_t{ 1 }) };
    auto * rightOutputSamples{ outputBuffer.getWritePointerByOutputPatch(output_patch_t{ 2 }) };
    auto const numSamples{ inputBuffer.getNumSamples() };

    for (int sourceIndex{}; sourceIndex < mSourcesData.size(); ++sourceIndex) {
        // Is empty?
        auto & source{ mSourcesData[sourceIndex] };
        if (source.magnitude < SMALL_GAIN) {
            // nothing to process
            continue;
        }

        // Is direct out?
        auto const * inputSamples{ inputBuffer.getReadPointer(sourceIndex) };
        if (source.directOut) {
            auto * target{ source.directOut->get() % 2 == 1 ? leftOutputSamples : rightOutputSamples };
            std::transform(inputSamples, inputSamples + numSamples, target, target, std::plus());
            continue;
        }

        // Process
        auto const & azimuth{ source.azimuth };
        auto lastAzimuth{ source.lastAzimuth };
        for (int sampleIndex{}; sampleIndex < numSamples; ++sampleIndex) {
            // Removes the chirp at 180->-180 degrees azimuth boundary.
            if ((lastAzimuth - azimuth).abs() > degrees_t{ 300.0f }) {
                lastAzimuth = azimuth;
            }
            lastAzimuth = azimuth + (lastAzimuth - azimuth) * gainFactor;
            degrees_t scaled;
            if (lastAzimuth < degrees_t{ -90.0f }) {
                scaled = degrees_t{ -90.0f } - (lastAzimuth + degrees_t{ 90.0f });
            } else if (lastAzimuth > degrees_t{ 90.0f }) {
                scaled = degrees_t{ 90.0f } - (lastAzimuth - degrees_t{ 90.0f });
            } else {
                scaled = lastAzimuth;
            }
            scaled = (scaled + degrees_t{ 90.0f }) * FACTOR;
            using fast = juce::dsp::FastMathApproximations;
            leftOutputSamples[sampleIndex] += inputSamples[sampleIndex] * fast::cos(scaled.get());
            rightOutputSamples[sampleIndex] += inputSamples[sampleIndex] * fast::sin(scaled.get());
        }
        source.lastAzimuth = lastAzimuth;
    }

    // Apply gain compensation.
    auto const compensation{ std::pow(10.0f, (narrow<float>(inputBuffer.getNumChannels()) - 1.0f) * -0.1f * 0.05f) };
    auto const applyCompensation = [compensation](float & sample) { sample *= compensation; };
    std::for_each_n(leftOutputSamples, numSamples, applyCompensation);
    std::for_each_n(rightOutputSamples, numSamples, applyCompensation);
}

//==============================================================================
void AudioProcessor::processAudio(juce::AudioBuffer<float> & inputBuffer,
                                  TaggedAudioBuffer<MAX_OUTPUTS> & outputBuffer) noexcept
{
    // Skip if the user is editing the speaker setup.
    juce::ScopedTryLock const lock{ getCriticalSection() };
    if (!lock.isLocked()) {
        return;
    }

    jassert(mSourcesData.size() == inputBuffer.getNumChannels());

    muteSoloVuMeterIn(inputBuffer);

    switch (mModeSelected) {
    case SpatMode::vbap:
        processVbap(inputBuffer, outputBuffer);
        break;
    case SpatMode::lbap:
        processLbap(inputBuffer, outputBuffer);
        break;
    case SpatMode::hrtfVbap:
        processVBapHrtf(inputBuffer, outputBuffer);
        break;
    case SpatMode::stereo:
        processStereo(inputBuffer, outputBuffer);
        break;
    default:
        jassertfalse;
        break;
    }

    auto const numSamples{ inputBuffer.getNumSamples() };
    if (mPinkNoiseActive) {
        fillWithPinkNoise(outputBuffer.getUnderlyingBuffer(numSamples).getArrayOfWritePointers(),
                          outputBuffer.getNumChannels(),
                          numSamples,
                          mPinkNoiseGain);
    }

    muteSoloVuMeterGainOut(outputBuffer, numSamples, mMasterGainOut);
}

//==============================================================================
bool AudioProcessor::initSpeakersTriplet(std::vector<Speaker const *> const & listSpk,
                                         int const dimensions,
                                         bool const needToComputeVbap)
{
    jassert(
        std::none_of(listSpk.cbegin(), listSpk.cend(), [](Speaker const * speaker) { return speaker->isDirectOut(); }));

    if (listSpk.empty()) {
        return false;
    }

    LoudSpeaker lss[MAX_SPEAKER_COUNT];
    output_patch_t outputPatches[MAX_SPEAKER_COUNT];

    int j{};
    for (unsigned i{}; i < listSpk.size(); ++i) {
        auto const * speaker{ listSpk[i] };
        auto const * const * speakerDataIt{ std::find_if(mSpeakersOut.cbegin(),
                                                         mSpeakersOut.cend(),
                                                         [&](SpeakerData const * speakerData) {
                                                             return !speakerData->directOut
                                                                    && speakerData->outputPatch
                                                                           == speaker->getOutputPatch();
                                                         }) };
        jassert(speakerDataIt != mSpeakersOut.cend());
        auto const & speakerData{ **speakerDataIt };
        lss[i].coords.x = speakerData.x;
        lss[i].coords.y = speakerData.y;
        lss[i].coords.z = speakerData.z;
        lss[i].angles.azimuth = speakerData.azimuth;
        lss[i].angles.elevation = speakerData.zenith;
        lss[i].angles.length = speakerData.radius; // Always 1.0 for VBAP.
        outputPatches[i] = speakerData.outputPatch;
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

    for (auto & source : mSourcesData) {
        free_vbap_data(source.paramVBap);
        source.paramVBap = copy_vbap_data(mParamVBap);
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

    return true;
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
void AudioProcessor::setAttenuationDbIndex(int const index)
{
    jassert(index >= 0 && index < ATTENUATION_DB_STRINGS.size());
    auto const gain{ std::pow(10.0f, ATTENUATION_DB_STRINGS[index].getFloatValue() * 0.05f) };
    mAttenuationLinearGain = gain;
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
