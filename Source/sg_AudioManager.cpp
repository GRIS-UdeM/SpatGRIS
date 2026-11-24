/*
 This file is part of SpatGRIS.

 Developers: Gaël Lane Lépine, Samuel Béland, Olivier Bélanger, Nicolas Masson

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

#include "sg_AudioManager.hpp"

#include "Data/sg_constants.hpp"
#include "sg_AudioProcessor.hpp"

// #define SIMULATE_NO_AUDIO_DEVICES

namespace gris
{
//==============================================================================
juce::BigInteger const NEEDED_INPUT_CHANNELS{ [] {
    juce::BigInteger channels{};
    channels.setRange(0, MAX_NUM_SOURCES, true);
    return channels;
}() };

//==============================================================================
juce::BigInteger const NEEDED_OUTPUT_CHANNELS{ [] {
    juce::BigInteger channels{};
    channels.setRange(0, MAX_NUM_SPEAKERS, true);
    return channels;
}() };

//==============================================================================
std::unique_ptr<AudioManager> AudioManager::mInstance{ nullptr };

//==============================================================================
AudioManager::AudioManager(juce::String const & deviceType,
                           juce::String const & inputDevice,
                           juce::String const & outputDevice,
                           double const pSampleRate,
                           int const pBufferSize,
                           tl::optional<StereoRouting> const & stereoRouting)
    : mStereoRouting(stereoRouting)
{
    JUCE_ASSERT_MESSAGE_THREAD;

#ifndef SIMULATE_NO_AUDIO_DEVICES
    auto const success{ tryInitAudioDevice(deviceType, inputDevice, outputDevice, pSampleRate, pBufferSize) };

    if (!success) {
        mAudioDeviceManager.initialiseWithDefaultDevices(MAX_NUM_SOURCES, MAX_NUM_SPEAKERS);

        auto setup{ mAudioDeviceManager.getAudioDeviceSetup() };
        setup.inputChannels = NEEDED_INPUT_CHANNELS;
        setup.outputChannels = NEEDED_OUTPUT_CHANNELS;
        if (mAudioDeviceManager.setAudioDeviceSetup(setup, true).isNotEmpty()) {
            mAudioDeviceManager.initialiseWithDefaultDevices(MAX_NUM_SOURCES, MAX_NUM_SPEAKERS);
        }
    }
#endif

    mAudioDeviceManager.addAudioCallback(this);
}

//==============================================================================
void AudioManager::audioDeviceIOCallbackWithContext(const float * const * inputChannelData,
                                                    int totalNumInputChannels,
                                                    float * const * outputChannelData,
                                                    int totalNumOutputChannels,
                                                    int numSamples,
                                                    [[maybe_unused]] const juce::AudioIODeviceCallbackContext & context)
{
    jassert(numSamples <= mInputBuffer.MAX_NUM_SAMPLES);
    jassert(numSamples <= mOutputBuffer.MAX_NUM_SAMPLES);

    // clear buffers
    mInputBuffer.silence();
    if (mStereoRouting) {
        mStereoOutputBuffer.clear();
    }
    mOutputBuffer.silence();
    if (mIsRecordingAmbiFiles) {
        mOutputAmbiEncBuffer.clear();
    }
    mInputAmbiDecBuffer.clear();
    mInputAmbiDecSrcAudioBuffer.silence();

    std::for_each_n(outputChannelData, totalNumOutputChannels, [numSamples](float * const data) {
        std::fill_n(data, numSamples, 0.0f);
    });

    if (!mAudioProcessor || mIsPlayerLoading) {
        return;
    }

    juce::ScopedTryLock const lock{ mAudioProcessor->getLock() };
    if (!lock.isLocked()) {
        return;
    }

    auto const playerState{ mPlayerState.load() };
    auto const isFadedOut{ playerState == PlayerState::fadedOut };
    if (isFadedOut) {
        // The faded-out block went through the ambisonic decoder during the previous callback : it can now be
        // turned off without cutting anything, and without it re-processing a stale input buffer.
        mAudioProcessor->setShouldProcessAmbiDecAlgo(false);
    }

    // if there is a player, copy audio file data to buffers, if not,
    // copy input data to buffers
    if (playerState != PlayerState::stopped) {
        // Gain ramp of this block : from 0 on the first block after a start, to 0 on the last block before a stop.
        // Once faded out, the transports are still pulled (silenced) so that AudioTransportSource::stop(), which
        // waits for one more rendered block, returns quickly.
        auto const gainStart{ isFadedOut || mPlayerFadeInPending.exchange(false) ? 0.0f : 1.0f };
        auto const gainEnd{ playerState == PlayerState::playing ? 1.0f : 0.0f };
        auto const applyRamp = [gainStart, gainEnd](float * data, int const numRampSamples) {
            if (gainStart == 1.0f && gainEnd == 1.0f) {
                return;
            }
            auto const increment{ (gainEnd - gainStart) / static_cast<float>(numRampSamples) };
            auto gain{ gainStart };
            for (int i{}; i < numRampSamples; ++i) {
                data[i] *= gain;
                gain += increment;
            }
        };

        // Renders every player file into its SpatGRIS source buffers. Each file has a single transport source
        // (whatever its number of channels), so an interleaved file is read and decoded only once.
        // All transport sources are started, stopped and positioned together (startPlaying(), stopPlaying(),
        // setPosition(), reloadPlayerAudioFiles()), so they stay in sync without re-seeking them here.
        auto const renderPlayer = [numSamples, &applyRamp](juce::OwnedArray<juce::AudioTransportSource> & transports,
                                                           juce::OwnedArray<PlayerTrack> & tracks,
                                                           SourceAudioBuffer & destination,
                                                           juce::Array<float *> * channelPtrs) {
            jassert(transports.size() == tracks.size());
            auto const numTracks{ std::min(transports.size(), tracks.size()) };
            for (int i{}; i < numTracks; ++i) {
                auto & track{ *tracks.getUnchecked(i) };
                auto & transport{ *transports.getUnchecked(i) };

                if (track.sourceIndexes.size() == 1) {
                    // mono file: render directly into the source buffer
                    auto & buffer{ destination[track.sourceIndexes.getUnchecked(0)] };
                    transport.getNextAudioBlock(juce::AudioSourceChannelInfo{ buffer });
                    applyRamp(buffer.getWritePointer(0), buffer.getNumSamples());
                    if (channelPtrs != nullptr) {
                        channelPtrs->add(buffer.getWritePointer(0));
                    }
                    continue;
                }

                jassert(numSamples <= track.scratchBuffer.getNumSamples());
                if (numSamples > track.scratchBuffer.getNumSamples()) {
                    continue;
                }
                transport.getNextAudioBlock(juce::AudioSourceChannelInfo{ &track.scratchBuffer, 0, numSamples });
                for (int channel{}; channel < track.sourceIndexes.size(); ++channel) {
                    auto * dest{ destination[track.sourceIndexes.getUnchecked(channel)].getWritePointer(0) };
                    std::copy_n(track.scratchBuffer.getReadPointer(channel), numSamples, dest);
                    applyRamp(dest, numSamples);
                    if (channelPtrs != nullptr) {
                        channelPtrs->add(dest);
                    }
                }
            }
        };

        auto const isAudioMode{ mPlayingMode.load() == PlayerMode::audio };
        auto & transports{ isAudioMode ? mTransportSources : mAmbiTransportSources };
        if (isAudioMode) {
            renderPlayer(mTransportSources, mPlayerTracks, mInputBuffer, nullptr);
        } else { // play ambisonic files
            auto const & ambiDecSpatAlgo = mAudioProcessor->getAmbiDecSpatAlgorithm();
            mInputAmbiDecChannelPtrs.clearQuick();
            renderPlayer(mAmbiTransportSources,
                         mAmbiPlayerTracks,
                         mInputAmbiDecSrcAudioBuffer,
                         &mInputAmbiDecChannelPtrs);
            if (ambiDecSpatAlgo != nullptr && !isFadedOut) {
                mInputAmbiDecBuffer.setDataToReferTo(mInputAmbiDecChannelPtrs.getRawDataPointer(),
                                                     mInputAmbiDecChannelPtrs.size(),
                                                     numSamples);
                static_cast<AmbiDecSpatAlgorithm *>(ambiDecSpatAlgo.get())->copyAmbPlayerBuffer(mInputAmbiDecBuffer);
                // Turned on here so the decoder never runs before it has received a fresh block.
                // It is turned off once the faded-out block has been decoded (fadedOut state).
                mAudioProcessor->setShouldProcessAmbiDecAlgo(true);
            }
        }

        if (playerState == PlayerState::fadingOut) {
            // The transports keep being pulled until they are actually stopped.
            if (!transports.isEmpty()) {
                mPlayerStopPosition = transports.getUnchecked(0)->getNextReadPosition();
            }
            auto expected{ PlayerState::fadingOut };
            mPlayerState.compare_exchange_strong(expected, PlayerState::fadedOut);
        }
    } else {
        auto const numInputChannelsToCopy{ std::min(totalNumInputChannels, mInputBuffer.size()) };

        auto activeChannel{ std::begin(mAudioProcessor->getAudioData().config->sourcesAudioConfig) };
        for (int i{}; i < numInputChannelsToCopy; ++i) {
            source_index_t const sourceIndex{ activeChannel->key };
            auto const * sourceData{ inputChannelData[sourceIndex.get() - source_index_t::OFFSET] };
            auto * destinationData{ mInputBuffer[sourceIndex].getWritePointer(0) };
            std::copy_n(sourceData, numSamples, destinationData);
            ++activeChannel;
        }
    }

    // do the actual processing
    mAudioProcessor->processAudio(mInputBuffer, mOutputBuffer, mStereoOutputBuffer, mSampleRate);

    // copy buffers to output
    if (mStereoRouting) {
        jassert(mStereoOutputBuffer.getNumChannels() == 2);
        auto const leftIndex{ mStereoRouting->left.template removeOffset<int>() };
        auto const rightIndex{ mStereoRouting->right.template removeOffset<int>() };
        if (leftIndex < totalNumOutputChannels) {
            std::copy_n(mStereoOutputBuffer.getReadPointer(0), numSamples, outputChannelData[leftIndex]);
        }
        if (rightIndex < totalNumOutputChannels) {
            std::copy_n(mStereoOutputBuffer.getReadPointer(1), numSamples, outputChannelData[rightIndex]);
        }
    } else {
        mOutputBuffer.copyToPhysicalOutput(outputChannelData, totalNumOutputChannels);
    }

    // Record
    if (mIsRecording) {
        auto const stopRecordingAndDisplayError = [this]() {
            stopRecording();
            juce::AlertWindow::showMessageBox(
                juce::AlertWindow::AlertIconType::WarningIcon,
                "Error",
                "Recording stopped because samples were dropped.\nRecording on a faster disk might solve this issue.");
        };

        if (!mIsRecordingOnlyAmbisonic) {
            for (auto const & recorder : mRecorders) {
                jassert(recorder->audioFormatWriterPtr->getNumChannels() == recorder->dataToRecord.size());
                auto const success{ recorder->threadedWriter->write(recorder->dataToRecord.data(), numSamples) };
                if (!success) {
                    jassertfalse;
                    stopRecordingAndDisplayError();
                }
            }
        }
        if (mIsRecordingAmbiFiles) {
            auto const & ambiEncSpatAlgo = mAudioProcessor->getAmbiEncSpatAlgorithm();
            static_cast<AmbiEncSpatAlgorithm *>(ambiEncSpatAlgo.get())->copyAmbProcessedBuffer(mOutputAmbiEncBuffer);
            for (auto const & recorder : mAmbiRecorders) {
                jassert(recorder->audioFormatWriterPtr->getNumChannels() == recorder->dataToRecord.size());
                auto const success{ recorder->threadedWriter->write(recorder->dataToRecord.data(), numSamples) };
                if (!success) {
                    jassertfalse;
                    stopRecordingAndDisplayError();
                }
            }
        }
        mNumSamplesRecorded += numSamples;
    }
}

//==============================================================================
bool AudioManager::tryInitAudioDevice(juce::String const & deviceType,
                                      juce::String const & inputDevice,
                                      juce::String const & outputDevice,
                                      double const requestedSampleRate,
                                      int const requestedBufferSize)
{
    JUCE_ASSERT_MESSAGE_THREAD;

    if (deviceType.isEmpty() || inputDevice.isEmpty() || outputDevice.isEmpty()) {
        return false;
    }

    auto const & availableDeviceTypes{ mAudioDeviceManager.getAvailableDeviceTypes() };

    auto const * const foundDeviceType{ std::find_if(
        std::cbegin(availableDeviceTypes),
        std::cend(availableDeviceTypes),
        [&deviceType](juce::AudioIODeviceType const * audioDeviceType) -> bool {
            return audioDeviceType->getTypeName() == deviceType;
        }) };
    if (foundDeviceType == std::cend(availableDeviceTypes)) {
        return false;
    }

    mAudioDeviceManager.setCurrentAudioDeviceType(deviceType, true);
    auto * deviceTypeObject{ mAudioDeviceManager.getCurrentDeviceTypeObject() };
    jassert(deviceTypeObject);
    deviceTypeObject->scanForDevices();

    auto const filterDevice = [&](juce::String const & name, bool const isInput) -> juce::String {
        auto const names{ deviceTypeObject->getDeviceNames(isInput) };
        if (names.contains(name)) {
            return name;
        }

        return names[deviceTypeObject->getDefaultDeviceIndex(isInput)];
    };

    auto const filteredInputDevice{ filterDevice(inputDevice, true) };
    auto const filteredOutputDevice{ filterDevice(outputDevice, false) };

    juce::AudioDeviceManager::AudioDeviceSetup const setup{ filteredOutputDevice,   filteredInputDevice,
                                                            requestedSampleRate,    requestedBufferSize,
                                                            NEEDED_INPUT_CHANNELS,  false,
                                                            NEEDED_OUTPUT_CHANNELS, false };
    auto const errorString{ mAudioDeviceManager.setAudioDeviceSetup(setup, true) };
    if (errorString.isNotEmpty()) {
        return false;
    }
    return true;
}

//==============================================================================
void AudioManager::init(juce::String const & deviceType,
                        juce::String const & inputDevice,
                        juce::String const & outputDevice,
                        double const sampleRate,
                        int const bufferSize,
                        tl::optional<StereoRouting> const & stereoRouting)
{
    JUCE_ASSERT_MESSAGE_THREAD;
    mInstance.reset(new AudioManager{ deviceType, inputDevice, outputDevice, sampleRate, bufferSize, stereoRouting });
}

//==============================================================================
AudioManager::~AudioManager()
{
    JUCE_ASSERT_MESSAGE_THREAD;

    if (mIsRecording) {
        stopRecording();
        mRecorders.clear(true);
        mAmbiRecorders.clear(true);
    }

    unloadPlayer();
}

//==============================================================================
void AudioManager::registerAudioProcessor(AudioProcessor * audioProcessor)
{
    JUCE_ASSERT_MESSAGE_THREAD;
    mAudioProcessor = audioProcessor;
}

//==============================================================================
juce::AudioDeviceManager const & AudioManager::getAudioDeviceManager() const
{
    return mAudioDeviceManager;
}

//==============================================================================
juce::AudioDeviceManager & AudioManager::getAudioDeviceManager()
{
    return mAudioDeviceManager;
}

//==============================================================================
bool AudioManager::isRecording() const
{
    return mIsRecording;
}

//==============================================================================
int64_t AudioManager::getNumSamplesRecorded() const
{
    return mNumSamplesRecorded.get();
}

//==============================================================================
void AudioManager::startPlaying()
{
    JUCE_ASSERT_MESSAGE_THREAD;

    if (mPlayerState.load() != PlayerState::stopped) {
        return;
    }

    auto const mode{ mAudioProcessor->getAudioData().config->playerMode };
    mPlayingMode = mode;

    // The audio thread doesn't pull the transports while stopped, so they all start in sync. The fade-in is applied
    // by the audio callback. The ambisonic decoder is turned on by the audio callback with the first block.
    for (auto transportSource : mode == PlayerMode::audio ? mTransportSources : mAmbiTransportSources) {
        transportSource->setGain(1.0f);
        transportSource->start();
    }

    mPlayerFadeInPending = true;
    mPlayerState = PlayerState::playing;
}

//==============================================================================
void AudioManager::stopPlaying()
{
    JUCE_ASSERT_MESSAGE_THREAD;

    auto expected{ PlayerState::playing };
    if (!mPlayerState.compare_exchange_strong(expected, PlayerState::fadingOut)) {
        return; // not playing
    }

    // Let the audio thread render one last block faded out to 0 (and decode it, in ambisonic mode).
    auto const fadedOut{ waitForPlayerState(PlayerState::fadedOut) };

    // Stop the transports of the mode that was playing : config->playerMode may already have been switched.
    // The audio thread keeps pulling them (silenced) meanwhile, so each stop() returns within one block.
    auto & transports{ mPlayingMode.load() == PlayerMode::audio ? mTransportSources : mAmbiTransportSources };
    for (auto transportSource : transports) {
        transportSource->stop();
    }

    // Move transports back to where playback audibly stopped.
    if (fadedOut) {
        auto const stopPosition{ mPlayerStopPosition.load() };
        for (auto transportSource : transports) {
            transportSource->setNextReadPosition(stopPosition);
        }
    }

    mPlayerState = PlayerState::stopped;
    // Normally already done by the audio thread in the fadedOut state. Needed if the audio device isn't running.
    mAudioProcessor->setShouldProcessAmbiDecAlgo(false);
}

//==============================================================================
bool AudioManager::waitForPlayerState(PlayerState const state) const
{
    JUCE_ASSERT_MESSAGE_THREAD;

    auto * device{ mAudioDeviceManager.getCurrentAudioDevice() };
    if (device == nullptr || !device->isPlaying()) {
        return mPlayerState.load() == state;
    }

    // A few audio blocks, plus some margin for the scheduler.
    auto const blockDurationMs{ 1000.0 * device->getCurrentBufferSizeSamples()
                                / std::max(1.0, device->getCurrentSampleRate()) };
    auto const timeoutMs{ 100 + juce::roundToInt(4.0 * blockDurationMs) };
    auto const deadline{ juce::Time::getMillisecondCounter() + static_cast<juce::uint32>(timeoutMs) };

    while (mPlayerState.load() != state) {
        if (juce::Time::getMillisecondCounter() > deadline) {
            jassertfalse; // the audio callback didn't run (device stopped, audio lock held, Player loading...)
            return false;
        }
        juce::Thread::sleep(1);
    }
    return true;
}

//==============================================================================
bool AudioManager::isPlaying() const
{
    return mPlayerState.load() == PlayerState::playing;
}

//==============================================================================
void AudioManager::setPlayerLoading(bool const playerIsLoading)
{
    JUCE_ASSERT_MESSAGE_THREAD;

    mIsPlayerLoading = playerIsLoading;
}

//==============================================================================
void AudioManager::unloadPlayer()
{
    JUCE_ASSERT_MESSAGE_THREAD;

    mPlayerState = PlayerState::stopped;

    mTransportSources.clear(true);
    mAmbiTransportSources.clear(true);
    mReaderSources.clear(true);
    mAmbiReaderSources.clear(true);
    mPlayerTracks.clear(true);
    mAmbiPlayerTracks.clear(true);
    mAudioFiles.clear();
    mAmbiAudioFiles.clear();
    mPlayerThread.stopThread(-1);
    mAmbiPlayerThread.stopThread(-1);
}

//==============================================================================
void AudioManager::setPosition(double const newPos)
{
    JUCE_ASSERT_MESSAGE_THREAD;

    for (auto transportSource : mTransportSources) {
        transportSource->setPosition(newPos);
    }
    for (auto transportSource : mAmbiTransportSources) {
        transportSource->setPosition(newPos);
    }
}

//==============================================================================
void AudioManager::reloadPlayerAudioFiles(int currentBufferSize, double currentSampleRate)
{
    JUCE_ASSERT_MESSAGE_THREAD;

    if (mTransportSources.size() == 0 && mAmbiTransportSources.size() == 0) {
        return;
    }

    // Transports only contain a resampler if the file and device rates differed when they were attached. If the
    // device rate changed since, re-attach them so the resampler is added/removed as needed.
    auto const reattachIfNeeded = [currentSampleRate](juce::OwnedArray<juce::AudioTransportSource> & transports,
                                                      juce::OwnedArray<juce::AudioFormatReaderSource> & readers,
                                                      double & transportsSampleRate,
                                                      juce::TimeSliceThread & thread) {
        if (juce::approximatelyEqual(transportsSampleRate, currentSampleRate)) {
            return;
        }
        jassert(transports.size() == readers.size()); // one reader source per transport, in the same order
        for (int i{}; i < transports.size(); ++i) {
            attachTransportSource(*transports.getUnchecked(i), readers[i], currentSampleRate, thread);
        }
        transportsSampleRate = currentSampleRate;
    };
    reattachIfNeeded(mTransportSources, mReaderSources, mTransportsSampleRate, mPlayerThread);
    reattachIfNeeded(mAmbiTransportSources, mAmbiReaderSources, mAmbiTransportsSampleRate, mAmbiPlayerThread);

    for (auto transportSource : mTransportSources) {
        transportSource->prepareToPlay(currentBufferSize, currentSampleRate);
        transportSource->setPosition(0.0);
    }
    for (auto ambiTransportSource : mAmbiTransportSources) {
        ambiTransportSource->prepareToPlay(currentBufferSize, currentSampleRate);
        ambiTransportSource->setPosition(0.0);
    }
}

//==============================================================================
void AudioManager::attachTransportSource(juce::AudioTransportSource & transport,
                                         juce::AudioFormatReaderSource * source,
                                         double const deviceSampleRate,
                                         juce::TimeSliceThread & thread)
{
    jassert(source != nullptr && source->getAudioFormatReader() != nullptr);
    auto const * reader{ source->getAudioFormatReader() };
    auto const fileSampleRate{ reader->sampleRate };
    // A non-zero rate makes AudioTransportSource always insert a ResamplingAudioSource, even for a 1:1 ratio,
    // so only pass it when resampling is actually needed.
    auto const rateToCorrectFor{ juce::approximatelyEqual(fileSampleRate, deviceSampleRate) ? 0.0 : fileSampleRate };
    // Use the file's real channel count (the default of 2 would buffer a duplicated channel for mono files).
    auto const numChannels{ static_cast<int>(reader->numChannels) };
    transport.setSource(source, PLAYER_READ_AHEAD_SIZE_IN_SAMPLES, &thread, rateToCorrectFor, numChannels);
}

//==============================================================================
bool AudioManager::preparePlayer(juce::File const & folder,
                                 PlayerMode mode,
                                 tl::optional<SpeakerSetup> & playerSpeakerSetup)
{
    JUCE_ASSERT_MESSAGE_THREAD;

    auto * currentAudioDevice{ mAudioDeviceManager.getCurrentAudioDevice() };
    jassert(currentAudioDevice);
    if (!currentAudioDevice) {
        return false;
    }

    auto & formatManager = mode == PlayerMode::audio ? mFormatManager : mAmbiFormatManager;
    auto & formatsRegistered = mode == PlayerMode::audio ? mFormatsRegistered : mAmbiFormatsRegistered;
    auto & audioFiles = mode == PlayerMode::audio ? mAudioFiles : mAmbiAudioFiles;
    auto & readerSources = mode == PlayerMode::audio ? mReaderSources : mAmbiReaderSources;
    auto & transportSources = mode == PlayerMode::audio ? mTransportSources : mAmbiTransportSources;
    auto & playerTracks = mode == PlayerMode::audio ? mPlayerTracks : mAmbiPlayerTracks;
    auto & playerThread = mode == PlayerMode::audio ? mPlayerThread : mAmbiPlayerThread;
    auto & audioFormat = mode == PlayerMode::audio ? mAudioFormat : mAmbiAudioFormat;

    if (formatsRegistered == false) {
        formatManager.registerBasicFormats();
        formatsRegistered = true;
    }

    if (isPlaying()) {
        stopPlaying();
    }

    auto const deviceSampleRate{ currentAudioDevice->getCurrentSampleRate() };
    auto & transportsSampleRate = mode == PlayerMode::audio ? mTransportsSampleRate : mAmbiTransportsSampleRate;
    transportsSampleRate = deviceSampleRate;

    audioFiles.clear();
    transportSources.clear(true);
    readerSources.clear(true);
    playerTracks.clear(true);

    // load audio/ambisonic files and sort them
    auto files = folder.findChildFiles(juce::File::TypesOfFileToFind::findFiles, false, "*.wav;*.aif;*.aiff");
    FileSorter sorter;
    files.sort(sorter);
    audioFiles = files; // for audio thumbnails

    for (const auto & filenameThatWasFound : files) {
        jassert(filenameThatWasFound.existsAsFile());

        // audio format to use
        audioFormat = formatManager.findFormatForFileExtension(filenameThatWasFound.getFileExtension());
        jassert(audioFormat);

        std::unique_ptr<juce::AudioFormatReader> reader(formatManager.createReaderFor(filenameThatWasFound));
        if (!reader)
            continue;

        auto const channelCount{ static_cast<int>(reader->numChannels) };
        auto track{ std::make_unique<PlayerTrack>() };

        if (channelCount > 1) {
            // Interleaved file : a single reader/transport for all its channels, so the file is read and decoded
            // only once. The audio callback dispatches its channels to their sources.
            auto spkSetupOrdered{ playerSpeakerSetup->ordering };
            spkSetupOrdered.sort(); // same speaker order used when recording
            if (mode == PlayerMode::audio) {
                jassert(channelCount == playerSpeakerSetup->speakers.size());
            }
            for (int channel{}; channel < channelCount; ++channel) {
                if (mode == PlayerMode::ambisonic) {
                    track->sourceIndexes.add(static_cast<source_index_t>(channel + source_index_t::OFFSET));
                } else {
                    track->sourceIndexes.add(static_cast<source_index_t>(spkSetupOrdered.getUnchecked(channel).get()));
                }
            }
            track->scratchBuffer.setSize(channelCount, SourceAudioBuffer::MAX_NUM_SAMPLES);
        } else {
            // make sure the file is mono
            jassert(reader->getChannelLayout().size() == 1);

            // get channel of audio file
            auto channel{ filenameThatWasFound.getFileNameWithoutExtension()
                              .fromLastOccurrenceOf(juce::String("-"), false, true)
                              .getIntValue() };
            track->sourceIndexes.add(source_index_t{ channel });
        }

        readerSources.add(new juce::AudioFormatReaderSource(reader.release(), true));

        auto transportSource = std::make_unique<juce::AudioTransportSource>();
        attachTransportSource(*transportSource, readerSources.getLast(), deviceSampleRate, playerThread);

        transportSources.add(transportSource.release());
        playerTracks.add(track.release());
    }

    // check all audio files are the same length
    for (auto source : transportSources) {
        if (source->getTotalLength() != transportSources[0]->getTotalLength()) {
            playerTracks.clear(true);
            transportSources.clear(true);
            readerSources.clear(true);
            audioFiles.clear();
            return false;
        }
    }

    juce::Thread::RealtimeOptions threadOptions;
    playerThread.startRealtimeThread(threadOptions.withPriority(9));
    reloadPlayerAudioFiles(currentAudioDevice->getCurrentBufferSizeSamples(),
                           currentAudioDevice->getCurrentSampleRate());

    return true;
}

//==============================================================================
juce::OwnedArray<juce::AudioTransportSource> & AudioManager::getTransportSources()
{
    return mTransportSources;
}

//==============================================================================
juce::AudioFormatManager & AudioManager::getAudioFormatManager()
{
    return mFormatManager;
}

//==============================================================================
juce::Array<juce::File> & AudioManager::getAudioFiles()
{
    return mAudioFiles;
}

//==============================================================================
juce::OwnedArray<juce::AudioTransportSource> & AudioManager::getAmbiTransportSources()
{
    return mAmbiTransportSources;
}

//==============================================================================
juce::AudioFormatManager & AudioManager::getAmbiAudioFormatManager()
{
    return mAmbiFormatManager;
}

//==============================================================================
juce::Array<juce::File> & AudioManager::getAmbiAudioFiles()
{
    return mAmbiAudioFiles;
}

//==============================================================================
juce::StringArray AudioManager::getAvailableDeviceTypeNames()
{
    JUCE_ASSERT_MESSAGE_THREAD;
    juce::StringArray result{};
    for (auto const * deviceType : mAudioDeviceManager.getAvailableDeviceTypes()) {
        result.add(deviceType->getTypeName());
    }
    return result;
}

//==============================================================================
bool AudioManager::prepareToRecord(RecordingParameters const & recordingParams)
{
    JUCE_ASSERT_MESSAGE_THREAD;

    static constexpr auto BITS_PER_SAMPLE = 24;
    static constexpr auto RECORD_QUALITY = 0;

    jassert(std::is_sorted(recordingParams.speakersToRecord.begin(), recordingParams.speakersToRecord.end()));
    mNumSamplesRecorded = 0;
    mRecorders.clearQuick(true);
    mAmbiRecorders.clearQuick(true);

    auto * currentAudioDevice{ mAudioDeviceManager.getCurrentAudioDevice() };
    jassert(currentAudioDevice);
    if (!currentAudioDevice) {
        return false;
    }

    mIsRecordingOnlyAmbisonic = recordingParams.options.recordOnlyAmbisonic;

    int totalNumAmbiBufferChannels{};
    int numAmbiChannels{};
    if (recordingParams.options.shouldRecordAmbisonicFiles) {
        mAudioProcessor->setShouldProcessAmbiEncAlgo(true);
        mIsRecordingAmbiFiles = true;
        totalNumAmbiBufferChannels = mOutputAmbiEncBuffer.getNumChannels();
    }
    numAmbiChannels = totalNumAmbiBufferChannels - mNumAmbiEncSpeakersDirectOutOnly;

    static auto const GET_AUDIO_FORMAT = [](RecordingFormat const format) -> std::unique_ptr<juce::AudioFormat> {
        std::unique_ptr<juce::AudioFormat> audioFormat{};
        switch (format) {
        case RecordingFormat::aiff:
            return std::make_unique<juce::AiffAudioFormat>();
        case RecordingFormat::wav:
            return std::make_unique<juce::WavAudioFormat>();
#ifdef USE_CAF
        case RecordingFormat::caf:
            return std::make_unique<juce::CoreAudioFormat>();
#endif
        }
        return nullptr;
    };

    // prepare audioFormat
    auto const audioFormat{ GET_AUDIO_FORMAT(recordingParams.options.format) };
    if (!audioFormat) {
        jassertfalse;
        return false;
    }

    // subroutine to build the RecorderInfos
    static auto const MAKE_RECORDING_INFO
        = [](juce::String const & path,
             juce::AudioFormat & format,
             double const sampleRate_,
             int const bufferSize_,
             juce::Array<float const *> dataToRecord,
             juce::TimeSliceThread & timeSlicedThread) -> std::unique_ptr<FileRecorder> {
        juce::StringPairArray const metaData{}; // lets leave this empty for now

        juce::File const outputFile{ path };
        std::unique_ptr<juce::OutputStream> outputStream{ outputFile.createOutputStream() };
        jassert(outputStream);
        if (!outputStream) {
            return nullptr;
        }
        auto audioFormatWriter{ format.createWriterFor(outputStream,
                                                       juce::AudioFormatWriterOptions{}
                                                           .withSampleRate(sampleRate_)
                                                           .withNumChannels(narrow<unsigned>(dataToRecord.size()))
                                                           .withBitsPerSample(BITS_PER_SAMPLE)
                                                           .withQualityOptionIndex(RECORD_QUALITY)) };
        jassert(audioFormatWriter);
        if (!audioFormatWriter) {
            return nullptr;
        }
        auto * audioFormatWriterPtr = audioFormatWriter.get();
        auto threadedWriter{ std::make_unique<juce::AudioFormatWriter::ThreadedWriter>(audioFormatWriter.release(),
                                                                                       timeSlicedThread,
                                                                                       bufferSize_) };
        jassert(threadedWriter);
        auto result{ std::make_unique<FileRecorder>() };
        result->audioFormatWriterPtr = audioFormatWriterPtr;
        result->threadedWriter = std::move(threadedWriter);
        result->dataToRecord = std::move(dataToRecord);
        return result;
    };

    // Compute file paths
    auto const multichannel_folder{ juce::String("mutlichannel_files") };
    auto const ambisonic_folder{ juce::String("ambisonic_files") };
    auto const baseOutputFile{ juce::File{ recordingParams.path }.getParentDirectory().getFullPathName()
                               + juce::File::getSeparatorString() + multichannel_folder
                               + juce::File::getSeparatorString()
                               + juce::File{ recordingParams.path }.getFileNameWithoutExtension() };
    auto const baseOutputAmbiFile{ juce::File{ recordingParams.path }.getParentDirectory().getFullPathName()
                                   + juce::File::getSeparatorString() + ambisonic_folder
                                   + juce::File::getSeparatorString()
                                   + juce::File{ recordingParams.path }.getFileNameWithoutExtension() };
    auto const extension{ juce::File{ recordingParams.path }.getFileExtension() };

    auto const getSeparateStereoFilePaths = [&]() {
        juce::StringArray result{};
        result.add(baseOutputFile + "-L" + extension);
        result.add(baseOutputFile + "-R" + extension);
        return result;
    };
    auto const getSeparateSpeakersFilePath = [&]() {
        juce::StringArray result{};
        result.ensureStorageAllocated(recordingParams.speakersToRecord.size());
        for (auto const outputPatch : recordingParams.speakersToRecord) {
            auto const path{ baseOutputFile + "-" + juce::String{ outputPatch.get() } + extension };
            jassert(!result.contains(path));
            result.add(path);
        }
        return result;
    };
    auto const getInterleavedAmbisonicFilePath = [&]() {
        juce::StringArray result{};
        result.ensureStorageAllocated(totalNumAmbiBufferChannels);
        result.add(baseOutputAmbiFile + "-amb" + extension);
        for (int i{ numAmbiChannels }; i < totalNumAmbiBufferChannels; ++i) {
            auto const path{ baseOutputAmbiFile + "-direct-" + juce::String{ i + 1 } + extension };
            jassert(!result.contains(path));
            result.add(path);
        }
        return result;
    };
    auto const getSeparateAmbisonicFilePath = [&]() {
        juce::StringArray result{};
        result.ensureStorageAllocated(totalNumAmbiBufferChannels);
        for (int i{}; i < numAmbiChannels; ++i) {
            auto const path{ baseOutputAmbiFile + "-amb-" + juce::String{ i + 1 } + extension };
            jassert(!result.contains(path));
            result.add(path);
        }
        for (int i{ numAmbiChannels }; i < totalNumAmbiBufferChannels; ++i) {
            auto const path{ baseOutputAmbiFile + "-direct-" + juce::String{ i + 1 } + extension };
            jassert(!result.contains(path));
            result.add(path);
        }
        return result;
    };
    auto const getMultichannelFilePaths = [&]() {
        if (recordingParams.options.fileType == RecordingFileType::interleaved) {
            return juce::StringArray{ baseOutputFile + extension };
        }

        jassert(recordingParams.options.fileType == RecordingFileType::mono);
        if (mStereoRouting) {
            return getSeparateStereoFilePaths();
        }

        return getSeparateSpeakersFilePath();
    };

    auto const getAmbisonicFilePaths = [&]() {
        if (recordingParams.options.fileType == RecordingFileType::interleaved) {
            return getInterleavedAmbisonicFilePath();
        }

        return getSeparateAmbisonicFilePath();
    };

    auto const filePaths{ getMultichannelFilePaths() };
    auto const ambiFilePaths{ getAmbisonicFilePaths() };

    // Delete files if needed
    auto const checkDeleteFiles = [&](juce::StringArray path) {
        auto deleteAllFiles{ false };
        for (auto const & fileName : path) {
            juce::File file{ fileName };
            if (file.existsAsFile()) {
                if (!deleteAllFiles) {
                    juce::AlertWindow alertWindow{ "error",
                                                   "File \"" + file.getFullPathName() + "\" will be deleted. Proceed ?",
                                                   juce::AlertWindow::WarningIcon };
                    alertWindow.addButton("No", 0);
                    alertWindow.addButton("yes", 1);
                    alertWindow.addButton("Yes for all", 2);
                    auto const result{ alertWindow.runModalLoop() };
                    if (result == 0) {
                        return false;
                    }
                    if (result == 2) {
                        deleteAllFiles = true;
                    }
                }
                auto const success{ file.deleteFile() };
                if (!success) {
                    juce::AlertWindow::showMessageBox(juce::AlertWindow::WarningIcon,
                                                      "Error",
                                                      juce::String{ "Unable to overwrite file \"" }
                                                          + file.getFullPathName() + "\".");
                    return false;
                }
            }
        }
        return true;
    };
    if (!mIsRecordingOnlyAmbisonic && !checkDeleteFiles(filePaths))
        return false;
    if (mIsRecordingAmbiFiles && !checkDeleteFiles(ambiFilePaths))
        return false;

    auto const recordingBufferSize{ RECORDERS_BUFFER_SIZE_IN_SAMPLES };
    auto const recordingAmbiBufferSize{ RECORDERS_BUFFER_SIZE_IN_SAMPLES };

    auto const makeInterleavedStereoRecorder = [&]() {
        jassert(filePaths.size() == 1);
        jassert(mStereoOutputBuffer.getNumChannels() == 2);
        juce::Array<float const *> dataToRecord{};
        dataToRecord.add(mStereoOutputBuffer.getReadPointer(0));
        dataToRecord.add(mStereoOutputBuffer.getReadPointer(1));
        auto recorder{ MAKE_RECORDING_INFO(filePaths[0],
                                           *audioFormat,
                                           recordingParams.sampleRate,
                                           recordingBufferSize,
                                           std::move(dataToRecord),
                                           mRecordersThread) };
        if (!recorder) {
            return false;
        }
        mRecorders.add(std::move(recorder));
        return true;
    };

    auto const makeSeparateStereoRecorder = [&]() {
        jassert(filePaths.size() == 2);
        jassert(mStereoOutputBuffer.getNumChannels() == 2);
        for (int i{}; i < 2; ++i) {
            auto recorder{ MAKE_RECORDING_INFO(filePaths[i],
                                               *audioFormat,
                                               recordingParams.sampleRate,
                                               recordingBufferSize,
                                               juce::Array<float const *>{ mStereoOutputBuffer.getReadPointer(i) },
                                               mRecordersThread) };
            if (!recorder) {
                return false;
            }
            mRecorders.add(std::move(recorder));
        }
        return true;
    };

    auto const makeInterleavedSpeakersRecorder = [&]() {
        jassert(filePaths.size() == 1);
        auto const & filePath{ filePaths[0] };
        auto dataToRecord = mOutputBuffer.getArrayOfReadPointers(recordingParams.speakersToRecord);
        auto recordingInfo{ MAKE_RECORDING_INFO(filePath,
                                                *audioFormat,
                                                recordingParams.sampleRate,
                                                recordingBufferSize,
                                                std::move(dataToRecord),
                                                mRecordersThread) };
        if (!recordingInfo) {
            return false;
        }
        mRecorders.add(std::move(recordingInfo));
        return true;
    };

    auto const makeSeparateSpeakersRecorder = [&]() {
        jassert(recordingParams.speakersToRecord.size() == filePaths.size());
        for (int i{}; i < recordingParams.speakersToRecord.size(); ++i) {
            auto const outputPatch{ recordingParams.speakersToRecord[i] };
            auto const & filePath{ filePaths[i] };
            juce::Array<float const *> dataToRecord{};
            dataToRecord.add(mOutputBuffer[outputPatch].getReadPointer(0));
            auto recordingInfo{ MAKE_RECORDING_INFO(filePath,
                                                    *audioFormat,
                                                    recordingParams.sampleRate,
                                                    recordingBufferSize,
                                                    std::move(dataToRecord),
                                                    mRecordersThread) };
            if (!recordingInfo) {
                return false;
            }
            mRecorders.add(std::move(recordingInfo));
        }
        return true;
    };

    auto const makeInterleavedAmbisonicRecorder = [&]() {
        jassert(ambiFilePaths.size() == mNumAmbiEncSpeakersDirectOutOnly + 1);
        auto const & filePath{ ambiFilePaths[0] };
        const int numChannelsToRecord = numAmbiChannels;
        jassert(numChannelsToRecord >= 0 && numChannelsToRecord <= mOutputAmbiEncBuffer.getNumChannels());
        juce::Array<const float *> interleavedDataToRecord;
        interleavedDataToRecord.ensureStorageAllocated(numChannelsToRecord);

        for (int ch = 0; ch < numChannelsToRecord; ++ch)
            interleavedDataToRecord.add(mOutputAmbiEncBuffer.getReadPointer(ch));

        auto recordingInterleavedInfo{ MAKE_RECORDING_INFO(filePath,
                                                           *audioFormat,
                                                           recordingParams.sampleRate,
                                                           recordingAmbiBufferSize,
                                                           std::move(interleavedDataToRecord),
                                                           mAmbiRecordersThread) };
        if (!recordingInterleavedInfo) {
            return false;
        }
        mAmbiRecorders.add(std::move(recordingInterleavedInfo));

        for (int ch{ numAmbiChannels }, filePathIndex{ 1 }; ch < totalNumAmbiBufferChannels; ++ch, ++filePathIndex) {
            auto const & directFilePath{ ambiFilePaths[filePathIndex] };
            juce::Array<float const *> dataToRecord{};
            dataToRecord.add(mOutputAmbiEncBuffer.getReadPointer(ch));
            auto recordingInfo{ MAKE_RECORDING_INFO(directFilePath,
                                                    *audioFormat,
                                                    recordingParams.sampleRate,
                                                    recordingAmbiBufferSize,
                                                    std::move(dataToRecord),
                                                    mAmbiRecordersThread) };
            if (!recordingInfo) {
                return false;
            }
            mAmbiRecorders.add(std::move(recordingInfo));
        }
        return true;
    };

    auto const makeSeparateAmbiRecorder = [&]() {
        jassert(ambiFilePaths.size() == totalNumAmbiBufferChannels);
        for (int ch{}; ch < totalNumAmbiBufferChannels; ++ch) {
            auto const & filePath{ ambiFilePaths[ch] };
            juce::Array<float const *> dataToRecord{};
            dataToRecord.add(mOutputAmbiEncBuffer.getReadPointer(ch));
            auto recordingInfo{ MAKE_RECORDING_INFO(filePath,
                                                    *audioFormat,
                                                    recordingParams.sampleRate,
                                                    recordingAmbiBufferSize,
                                                    std::move(dataToRecord),
                                                    mAmbiRecordersThread) };
            if (!recordingInfo) {
                return false;
            }
            mAmbiRecorders.add(std::move(recordingInfo));
        }
        return true;
    };

    auto const makeRecorders = [&]() {
        auto const isInterleaved{ recordingParams.options.fileType == RecordingFileType::interleaved };
        if (mStereoRouting) {
            if (isInterleaved) {
                return makeInterleavedStereoRecorder();
            }
            return makeSeparateStereoRecorder();
        }

        if (isInterleaved) {
            return makeInterleavedSpeakersRecorder();
        }

        return makeSeparateSpeakersRecorder();
    };

    auto const makeAmbiRecorders = [&]() {
        auto const isInterleaved{ recordingParams.options.fileType == RecordingFileType::interleaved };
        if (isInterleaved) {
            return makeInterleavedAmbisonicRecorder();
        }

        return makeSeparateAmbiRecorder();
    };

    // Make recorders
    if (!mIsRecordingOnlyAmbisonic) {
        if (!juce::File(baseOutputFile).getParentDirectory().exists()) {
            juce::File(baseOutputFile).getParentDirectory().createDirectory();
        }
        auto const success{ makeRecorders() };
        if (!success) {
            jassertfalse;
            return false;
        }
    }
    if (mIsRecordingAmbiFiles) {
        if (!juce::File(baseOutputAmbiFile).getParentDirectory().exists()) {
            juce::File(baseOutputAmbiFile).getParentDirectory().createDirectory();
        }
        auto const ambiSuccess{ makeAmbiRecorders() };
        if (!ambiSuccess) {
            jassertfalse;
            return false;
        }
    }

    // export speaker setup
    if (recordingParams.options.shouldSaveSpeakerSetup) {
        juce::File const & lastSpeakerSetup{ recordingParams.lastSpeakerSetup };
        auto const file{ juce::File(baseOutputFile).getParentDirectory().getFullPathName()
                         + juce::File::getSeparatorString() + lastSpeakerSetup.getFileName() };
        auto const content{ recordingParams.speakerSetup.toXml() };
        [[maybe_unused]] auto const success{ content->writeTo(file) };
        jassert(success);
    }

    if (!mIsRecordingOnlyAmbisonic)
        mRecordersThread.startThread(juce::Thread::Priority::highest);
    if (mIsRecordingAmbiFiles)
        mAmbiRecordersThread.startThread(juce::Thread::Priority::highest);

    return true;
}

//==============================================================================
void AudioManager::startRecording()
{
    JUCE_ASSERT_MESSAGE_THREAD;
    mIsRecording = true;
}

//==============================================================================
void AudioManager::stopRecording()
{
    JUCE_ASSERT_MESSAGE_THREAD;
    jassert(mAudioProcessor);
    juce::ScopedLock const sl{ mAudioProcessor->getLock() };
    // threadedWriters will flush their data before going off
    mRecorders.clear(true);
    mAmbiRecorders.clear(true);
    mRecordersThread.stopThread(-1);
    mAmbiRecordersThread.stopThread(-1);
    mIsRecording = false;
    mIsRecordingOnlyAmbisonic = false;
    mAudioProcessor->setShouldProcessAmbiEncAlgo(false);
    mIsRecordingAmbiFiles = false;
}

//==============================================================================
void AudioManager::initInputBuffer(juce::Array<source_index_t> const & sources)
{
    JUCE_ASSERT_MESSAGE_THREAD;
    jassert(mAudioProcessor);
    juce::ScopedLock const lock{ mAudioProcessor->getLock() };
    mInputBuffer.init(sources);
}

//==============================================================================
void AudioManager::initOutputBuffer(juce::Array<output_patch_t> const & speakers)
{
    JUCE_ASSERT_MESSAGE_THREAD;
    jassert(mAudioProcessor);
    juce::ScopedLock const lock{ mAudioProcessor->getLock() };
    mOutputBuffer.init(speakers);
}

//==============================================================================
void AudioManager::initAmbiEncOutputBuffer(int ambisonicOrder, int newBufferSize, int numSpeakersDirectOutOnly)
{
    mNumAmbiEncSpeakersDirectOutOnly = numSpeakersDirectOutOnly;
    auto const numAmbiChan{ ((ambisonicOrder + 1) * (ambisonicOrder + 1)) + numSpeakersDirectOutOnly };
    mOutputAmbiEncBuffer.setSize(numAmbiChan, newBufferSize);
}

//==============================================================================
void AudioManager::initAmbiDecInputBuffer(int numAmbiChannels, int newBufferSize, int numSpeakersDirectOutOnly)
{
    juce::Array<source_index_t> sources;
    auto const totalNumChan{ numAmbiChannels + numSpeakersDirectOutOnly };

    for (int i{ 1 }; i <= totalNumChan; ++i) {
        sources.add(static_cast<source_index_t>(i));
    }

    juce::ScopedLock const lock{ mAudioProcessor->getLock() };
    mInputAmbiDecSrcAudioBuffer.init(sources);
    mInputAmbiDecSrcAudioBuffer.setNumSamples(newBufferSize);
    mInputAmbiDecBuffer.setSize(totalNumChan, newBufferSize);
    mInputAmbiDecBuffer.clear();
    mInputAmbiDecChannelPtrs.ensureStorageAllocated(totalNumChan);
}

//==============================================================================
void AudioManager::setBufferSize(int const newBufferSize)
{
    JUCE_ASSERT_MESSAGE_THREAD;
    jassert(mAudioProcessor);
    juce::ScopedLock const lock{ mAudioProcessor->getLock() };
    mInputBuffer.setNumSamples(newBufferSize);
    mOutputBuffer.setNumSamples(newBufferSize);
    mStereoOutputBuffer.setSize(2, newBufferSize);
}

//==============================================================================
void AudioManager::setStereoRouting(tl::optional<StereoRouting> const & stereoRouting)
{
    JUCE_ASSERT_MESSAGE_THREAD;
    juce::ScopedLock const lock{ mAudioProcessor->getLock() };
    mStereoRouting = stereoRouting;
}

//==============================================================================
void AudioManager::audioDeviceError(juce::String const & /*errorMessage*/)
{
    jassertfalse;
}

//==============================================================================
void AudioManager::audioDeviceAboutToStart(juce::AudioIODevice * device)
{
    // when AudioProcessor will be a real AudioSource, prepareToPlay() should be called here.

    mSampleRate = device->getCurrentSampleRate();
}

//==============================================================================
void AudioManager::audioDeviceStopped()
{
    // when AudioProcessor will be a real AudioSource, releaseResources() should be called here.
}

//==============================================================================
AudioManager & AudioManager::getInstance()
{
    jassert(mInstance);
    return *mInstance;
}

//==============================================================================
void AudioManager::free()
{
    JUCE_ASSERT_MESSAGE_THREAD;
    mInstance.reset();
}

} // namespace gris
