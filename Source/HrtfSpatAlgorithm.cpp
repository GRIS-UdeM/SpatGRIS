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

#include "DummySpatAlgorithm.hpp"
#include "LbapSpatAlgorithm.hpp"
#include "VbapSpatAlgorithm.hpp"

static constexpr size_t MAX_BUFFER_SIZE = 2048;

//==============================================================================
// Load samples from a wav file into a float array.
static juce::AudioBuffer<float> getSamplesFromWavFile(juce::File const & file)
{
    JUCE_ASSERT_MESSAGE_THREAD;

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

    juce::Array<juce::AudioBuffer<float>> impulseResponses{};
    impulseResponses.resize(16);
    for (auto & buffer : impulseResponses) {
        buffer.setSize(2, 128);
    }

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
        std::copy_n(buffer.getReadPointer(leftChannel), 128, impulseResponses[i].getWritePointer(0));
        std::copy_n(buffer.getReadPointer(rightChannel), 128, impulseResponses[i].getWritePointer(1));
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
        auto const index{ i + 8 };
        std::copy_n(buffer.getReadPointer(leftChannel), 128, impulseResponses[index].getWritePointer(0));
        std::copy_n(buffer.getReadPointer(rightChannel), 128, impulseResponses[index].getWritePointer(1));
    }
    // Azimuth = 80
    for (int i{}; i < 2; ++i) {
        auto const file{ HRTF_FOLDER_80.getChildFile("H80e090a.wav") };
        auto const buffer{ getSamplesFromWavFile(file) };
        auto const leftChannel{ 1 - i };
        auto const rightChannel{ i };
        auto const index{ i + 14 };
        std::copy_n(buffer.getReadPointer(leftChannel), 128, impulseResponses[index].getWritePointer(0));
        std::copy_n(buffer.getReadPointer(rightChannel), 128, impulseResponses[index].getWritePointer(1));
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

    for (int i{}; i < impulseResponses.size(); ++i) {
        // mConvolutions[i].loadImpulseResponse(std::move(impulseResponses.getReference(i)), 44100.0,
        // juce::dsp::Convolution::Stereo::yes, juce::dsp::Convolution::Trim::no, juce::dsp::Convolution::Normalise::no);
        mConvolutions[i].prepare(juce::dsp::ProcessSpec{ 44100.0, 2048, 2 });
        mConvolutions[i].reset();
    }

    jassert(mInnerAlgorithm);

    fixDirectOutsIntoPlace(sources, speakerSetup);
}

//==============================================================================
void HrtfSpatAlgorithm::updateSpatData(source_index_t const sourceIndex, SourceData const & sourceData) noexcept
{
    jassert(!isProbablyAudioThread());

    if (sourceData.directOut) {
        return;
    }

    mInnerAlgorithm->updateSpatData(sourceIndex, sourceData);
}

//==============================================================================
void HrtfSpatAlgorithm::process(AudioConfig const & config,
                                SourceAudioBuffer & sourcesBuffer,
                                SpeakerAudioBuffer & speakersBuffer,
                                SourcePeaks const & sourcePeaks,
                                [[maybe_unused]] SpeakersAudioConfig const * altSpeakerConfig)
{
    ASSERT_AUDIO_THREAD;
    jassert(!altSpeakerConfig);

    if (speakersBuffer.size() < 2) {
        return;
    }

    speakersBuffer.silence();

    auto & hrtfBuffer{ mHrtfData.speakersBuffer };
    jassert(hrtfBuffer.size() == 16);
    hrtfBuffer.silence();

    mInnerAlgorithm->process(config, sourcesBuffer, hrtfBuffer, sourcePeaks, &mHrtfData.speakersAudioConfig);

    auto const numSamples{ sourcesBuffer.getNumSamples() };

    auto const getFirstTwoBuffers = [&]() {
        auto it{ speakersBuffer.begin() };
        auto & left{ *(*it++).value };
        auto & right{ *(*it).value };

        return std::array<juce::AudioBuffer<float> *, 2>{ &left, &right };
    };

    auto outputBuffers{ getFirstTwoBuffers() };

    static juce::AudioBuffer<float> convolutionBuffer{};
    convolutionBuffer.setSize(2, numSamples);

    size_t speakerIndex{};
    static auto constexpr MAGNITUDE{ 1.0f / 16.0f };
    for (auto const & speaker : mHrtfData.speakersAudioConfig) {
        convolutionBuffer.copyFrom(0, 0, hrtfBuffer[speaker.key], 0, 0, numSamples);
        convolutionBuffer.copyFrom(1, 0, hrtfBuffer[speaker.key], 0, 0, numSamples);
        juce::dsp::AudioBlock<float> block{ convolutionBuffer };
        juce::dsp::ProcessContextReplacing<float> const context{ block };
        mConvolutions[speakerIndex++].process(context);
        auto const & result{ context.getOutputBlock() };
        outputBuffers[0]->addFrom(0, 0, result.getChannelPointer(0), numSamples, MAGNITUDE);
        outputBuffers[1]->addFrom(0, 0, result.getChannelPointer(1), numSamples, MAGNITUDE);
    }
}

//==============================================================================
juce::Array<Triplet> HrtfSpatAlgorithm::getTriplets() const noexcept
{
    JUCE_ASSERT_MESSAGE_THREAD;
    return mInnerAlgorithm->getTriplets();
}

//==============================================================================
bool HrtfSpatAlgorithm::hasTriplets() const noexcept
{
    JUCE_ASSERT_MESSAGE_THREAD;
    return mInnerAlgorithm->hasTriplets();
}

//==============================================================================
std::unique_ptr<AbstractSpatAlgorithm>
    HrtfSpatAlgorithm::make(SpeakerSetup const & speakerSetup, SourcesData const & sources, juce::Component * parent)
{
    JUCE_ASSERT_MESSAGE_THREAD;

    static bool errorShown{};

    if (speakerSetup.numOfSpatializedSpeakers() < 2) {
        if (!errorShown) {
            juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::AlertIconType::InfoIcon,
                                                   "Disabled spatialization",
                                                   "The Binaural mode needs at least 2 speakers.\n",
                                                   "Ok",
                                                   parent);
            errorShown = true;
        }
        return std::make_unique<DummySpatAlgorithm>();
    }

    errorShown = false;
    return std::make_unique<HrtfSpatAlgorithm>(speakerSetup, sources);
}
