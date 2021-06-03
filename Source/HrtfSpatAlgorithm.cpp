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

#include "HrtfSpatAlgorithm.hpp"

#include "LbapSpatAlgorithm.hpp"
#include "VbapSpatAlgorithm.hpp"

static constexpr size_t MAX_BUFFER_SIZE = 2048;

//==============================================================================
// Load samples from a wav file into a float array.
static juce::AudioBuffer<float> getSamplesFromWavFile(juce::File const & file)
{
    if (!file.existsAsFile()) {
        auto const error{ file.getFullPathName() + "\n\nTry re-installing SpatGRIS." };
        juce::AlertWindow::showMessageBox(juce::AlertWindow::AlertIconType::WarningIcon, "Missing file", error);
        std::exit(-1);
    }

    static auto const FACTOR{ std::pow(2.0f, 31.0f) };

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
            samples.setSample(i, j, static_cast<float>(wavData[i][j]) / FACTOR);
        }
    }

    for (auto * it : wavData) {
        delete[] it;
    }
    return samples;
}

//==============================================================================
HrtfSpatAlgorithm::HrtfSpatAlgorithm(SpeakerSetup const & speakerSetup, SourcesData const & sources)
{
    JUCE_ASSERT_MESSAGE_THREAD;

    // Initialize impulse responses for VBAP+HRTF (BINAURAL mode).
    // Azimuth = 0
    juce::String names0[8] = { "H0e025a.wav", "H0e020a.wav", "H0e065a.wav", "H0e110a.wav",
                               "H0e155a.wav", "H0e160a.wav", "H0e115a.wav", "H0e070a.wav" };
    int reverse0[8] = { 1, 0, 0, 0, 0, 1, 1, 1 };
    for (int i{}; i < 8; ++i) {
        auto const file{ HRTF_FOLDER_0.getChildFile(names0[i]) };
        auto const buffer{ getSamplesFromWavFile(file) };
        auto const leftChannel{ reverse0[i] };
        auto const rightChannel{ 1 - reverse0[i] };
        std::memcpy(mHrtfData.leftImpulses[i].data(), buffer.getReadPointer(leftChannel), 128);
        std::memcpy(mHrtfData.rightImpulses[i].data(), buffer.getReadPointer(rightChannel), 128);
    }
    // Azimuth = 40
    juce::String names40[6]
        = { "H40e032a.wav", "H40e026a.wav", "H40e084a.wav", "H40e148a.wav", "H40e154a.wav", "H40e090a.wav" };
    int reverse40[6] = { 1, 0, 0, 0, 1, 1 };
    for (int i{}; i < 6; ++i) {
        auto const file{ HRTF_FOLDER_40.getChildFile(names40[i]) };
        auto const buffer{ getSamplesFromWavFile(file) };
        auto const leftChannel{ reverse40[i] };
        auto const rightChannel{ 1 - reverse40[i] };
        std::memcpy(mHrtfData.leftImpulses[i + 8].data(), buffer.getReadPointer(leftChannel), 128);
        std::memcpy(mHrtfData.rightImpulses[i + 8].data(), buffer.getReadPointer(rightChannel), 128);
    }
    // Azimuth = 80
    for (int i{}; i < 2; ++i) {
        auto const file{ HRTF_FOLDER_80.getChildFile("H80e090a.wav") };
        auto const buffer{ getSamplesFromWavFile(file) };
        auto const leftChannel{ 1 - i };
        auto const rightChannel{ i };
        std::memcpy(mHrtfData.leftImpulses[i + 14].data(), buffer.getReadPointer(leftChannel), 128);
        std::memcpy(mHrtfData.rightImpulses[i + 14].data(), buffer.getReadPointer(rightChannel), 128);
    }

    // Init temp hrtf buffer
    juce::Array<output_patch_t> hrtfPatches{};
    auto const binauralXml{ juce::XmlDocument{ BINAURAL_SPEAKER_SETUP_FILE }.getDocumentElement() };
    jassert(binauralXml);
    auto const binauralSpeakerSetup{ SpeakerSetup::fromXml(*binauralXml) };
    jassert(binauralSpeakerSetup);
    mHrtfData.speakersAudioConfig
        = binauralSpeakerSetup->toAudioConfig(48000.0f); // TODO: find a way to update this number!
    auto speakers{ binauralSpeakerSetup->order };
    speakers.sort();
    mHrtfData.speakersBuffer.init(speakers);

    auto const & binauralSpeakerData{ binauralSpeakerSetup->speakers };

    switch (speakerSetup.spatMode) {
    case SpatMode::vbap:
        mInnerAlgorithm = std::make_unique<VbapSpatAlgorithm>(binauralSpeakerData);
        break;
    case SpatMode::lbap:
        mInnerAlgorithm = std::make_unique<LbapSpatAlgorithm>(binauralSpeakerData);
        break;
    }

    jassert(mInnerAlgorithm);

    fixDirectOutsIntoPlace(sources, speakerSetup);
}

//==============================================================================
void HrtfSpatAlgorithm::updateSpatData(source_index_t const sourceIndex, SourceData const & sourceData) noexcept
{
    ASSERT_OSC_THREAD;

    mInnerAlgorithm->updateSpatData(sourceIndex, sourceData);
}

//==============================================================================
void HrtfSpatAlgorithm::process(AudioConfig const & config,
                                SourceAudioBuffer & sourcesBuffer,
                                SpeakerAudioBuffer & speakersBuffer,
                                SourcePeaks const & sourcePeaks,
                                SpeakersAudioConfig const * altSpeakerConfig)
{
    ASSERT_AUDIO_THREAD;

    auto const getFirstTwoBuffers = [&]() {
        auto it{ speakersBuffer.begin() };
        auto & left{ *(*it++).value };
        auto & right{ *(*it).value };

        return std::array<juce::AudioBuffer<float> &, 2>{ left, right };
    };

    auto outputBuffers{ getFirstTwoBuffers() };

    auto & hrtfBuffer{ mHrtfData.speakersBuffer };
    jassert(hrtfBuffer.size() == 16);
    hrtfBuffer.silence();

    mInnerAlgorithm->process(config, sourcesBuffer, hrtfBuffer, sourcePeaks, &mHrtfData.speakersAudioConfig);

    auto const numSamples{ sourcesBuffer.getNumSamples() };

    // Process hrtf and mix to stereo
    auto * leftOutputSamples{ outputBuffers[0].getWritePointer(0) };
    auto * rightOutputSamples{ outputBuffers[1].getWritePointer(0) };

    std::fill_n(leftOutputSamples, numSamples, 0.0f);
    std::fill_n(rightOutputSamples, numSamples, 0.0f);

    for (auto const & speaker : hrtfBuffer) {
        auto & hrtfCount{ mHrtfData.count };
        auto & hrtfInputTmp{ mHrtfData.inputTmp };
        auto const outputIndex{ speaker.key.removeOffset<size_t>() };
        auto const & outputSamplesBuffer{ *speaker.value };
        if (outputSamplesBuffer.getMagnitude(0, numSamples) < SMALL_GAIN) {
            continue;
        }
        auto const * outputSamples{ outputSamplesBuffer.getReadPointer(0) };
        auto const & hrtfLeftImpulses{ mHrtfData.leftImpulses };
        auto const & hrtfRightImpulses{ mHrtfData.rightImpulses };
        for (size_t sampleIndex{}; sampleIndex < narrow<size_t>(numSamples); ++sampleIndex) {
            auto tmpCount{ narrow<int>(hrtfCount[outputIndex]) };
            for (unsigned hrtfIndex{}; hrtfIndex < HRTF_NUM_SAMPLES; ++hrtfIndex) {
                if (tmpCount < 0) {
                    tmpCount += HRTF_NUM_SAMPLES;
                }
                // TODO : traversing tmpCount backwards is probably hurting performances
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
}

//==============================================================================
juce::Array<Triplet> HrtfSpatAlgorithm::getTriplets() const noexcept
{
    return mInnerAlgorithm->getTriplets();
}

//==============================================================================
bool HrtfSpatAlgorithm::hasTriplets() const noexcept
{
    return mInnerAlgorithm->hasTriplets();
}
