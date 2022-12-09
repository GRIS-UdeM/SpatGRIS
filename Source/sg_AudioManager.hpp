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

#pragma once

#include "sg_AudioStructs.hpp"
#include "sg_LogicStrucs.hpp"
#include "sg_TaggedAudioBuffer.hpp"

#include <JuceHeader.h>

namespace gris
{
extern juce::BigInteger const NEEDED_INPUT_CHANNELS;
extern juce::BigInteger const NEEDED_OUTPUT_CHANNELS;

class AudioProcessor;

//==============================================================================
/** Manages the audio hardware, the main audio callback and is responsible for recording live audio.
 *
 * This class is a Singleton that can be accessed with getInstance(), but note that is HAS to be initialized first with
 * init() and freed before main() exits with free(). TODO : this should NOT be a singleton at all, then!.
 */
class AudioManager final : juce::AudioSourcePlayer
{
    // 2^17 samples * 32 bits per sample == 0.5 mb buffer per channel
    static constexpr auto RECORDERS_BUFFER_SIZE_IN_SAMPLES = 131072;
    //==============================================================================
    /** Records audio for a single file, regardless of the number of channels. */
    struct FileRecorder {
        // The actual object that writes to disk.
        std::unique_ptr<juce::AudioFormatWriter::ThreadedWriter> threadedWriter{};
        // A pointer to the audio format. Note that the format writer is actually owned and accessed by the
        // ThreadedWriter : this pointer is never actually used. This has been left in only for doing sanity checks
        // assertions.
        juce::AudioFormatWriter * audioFormatWriter{};
        // A collection of pointers to the buffers that will get recorded on disk. Note that this is NOT null terminated
        // : all pointers are non-null and valid.
        juce::Array<float const *> dataToRecord{};
    };

    //==============================================================================
    class FileSorter
    {
    public:
        static int compareElements(juce::File a, juce::File b)
        {
            return a.getFileName().compareNatural(b.getFileName());
        }
    };

public:
    //==============================================================================
    /** The main parameters needed before starting a recording. */
    struct RecordingParameters {
        juce::String path{};
        RecordingOptions options{};
        double sampleRate{};
        juce::Array<output_patch_t> speakersToRecord{};
    };

private:
    //==============================================================================
    AudioProcessor * mAudioProcessor{};
    juce::AudioDeviceManager mAudioDeviceManager{};
    SourceAudioBuffer mInputBuffer{};
    SpeakerAudioBuffer mOutputBuffer{};
    juce::AudioBuffer<float> mStereoOutputBuffer{};
    tl::optional<StereoRouting> mStereoRouting{};
    // Recording
    bool mIsRecording{};
    juce::Atomic<int64_t> mNumSamplesRecorded{};
    juce::OwnedArray<FileRecorder> mRecorders{};
    juce::TimeSliceThread mRecordersThread{ "SpatGRIS recording thread" };
    // Playing
    juce::AudioFormatManager mFormatManager{};
    juce::Array<juce::File> mAudioFiles; // for audio thumbnails
    juce::OwnedArray<juce::AudioFormatReaderSource> mReaderSources;
    juce::OwnedArray<juce::AudioTransportSource> mTransportSources{};
    juce::OwnedArray<source_index_t> mTransportSourcesIndexes{};
    juce::TimeSliceThread mPlayerThread{ "SpatGRIS player thread" };
    juce::AudioFormat * mAudioFormat{};
    bool mFormatsRegistered{};
    bool mIsPlaying{};
    //==============================================================================
    static std::unique_ptr<AudioManager> mInstance;

public:
    //==============================================================================
    AudioManager() = delete;
    ~AudioManager() override;
    SG_DELETE_COPY_AND_MOVE(AudioManager)
    //==============================================================================
    [[nodiscard]] juce::AudioDeviceManager const & getAudioDeviceManager() const;
    [[nodiscard]] juce::AudioDeviceManager & getAudioDeviceManager();

    juce::StringArray getAvailableDeviceTypeNames();

    void registerAudioProcessor(AudioProcessor * audioProcessor);

    bool prepareToRecord(RecordingParameters const & recordingParams);
    void startRecording();
    void stopRecording();
    bool isRecording() const;
    int64_t getNumSamplesRecorded() const;

    // Player stuff
    bool prepareAudioPlayer(juce::File const & folder);
    void startPlaying();
    void stopPlaying();
    bool isPlaying() const;
    void unloadPlayer();
    void setPosition(double const newPos);
    void reloadPlayerAudioFiles(int currentBufferSize, double currentSampleRate);
    juce::OwnedArray<juce::AudioTransportSource> & getTransportSources();
    juce::AudioFormatManager & getAudioFormatManager();
    juce::Array<juce::File> & getAudioFiles();

    void initInputBuffer(juce::Array<source_index_t> const & sources);
    void initOutputBuffer(juce::Array<output_patch_t> const & speakers);
    void setBufferSize(int newBufferSize);
    void setStereoRouting(tl::optional<StereoRouting> const & stereoRouting);
    //==============================================================================
    // AudioSourcePlayer overrides
    void audioDeviceError(const juce::String & errorMessage) override;
    void audioDeviceIOCallbackWithContext(const float *const * inputChannelData,
                                          int totalNumInputChannels,
                                          float *const * outputChannelData,
                                          int totalNumOutputChannels,
                                          int numSamples,
                                          const juce::AudioIODeviceCallbackContext & context) override;
    void audioDeviceAboutToStart(juce::AudioIODevice * device) override;
    void audioDeviceStopped() override;
    //==============================================================================
    static void init(juce::String const & deviceType,
                     juce::String const & inputDevice,
                     juce::String const & outputDevice,
                     double sampleRate,
                     int bufferSize,
                     tl::optional<StereoRouting> const & stereoRouting);
    static void free();
    [[nodiscard]] static AudioManager & getInstance();

private:
    //==============================================================================
    AudioManager(juce::String const & deviceType,
                 juce::String const & inputDevice,
                 juce::String const & outputDevice,
                 double sampleRate,
                 int bufferSize,
                 tl::optional<StereoRouting> const & stereoRouting);
    //==============================================================================
    [[nodiscard]] bool tryInitAudioDevice(juce::String const & deviceType,
                                          juce::String const & inputDevice,
                                          juce::String const & outputDevice,
                                          double requestedSampleRate,
                                          int requestedBufferSize);
    //==============================================================================

    JUCE_LEAK_DETECTOR(AudioManager)
}; // class AudioManager
} // namespace gris
