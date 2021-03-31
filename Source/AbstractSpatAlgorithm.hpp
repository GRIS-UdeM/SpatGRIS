#pragma once

#include "AudioStructs.hpp"
#include "SpatMode.hpp"
#include "TaggedAudioBuffer.hpp"

class AbstractSpatAlgorithm
{
protected:
    StaticMap<source_index_t, juce::Atomic<SpeakersSpatGains *>, MAX_INPUTS> mSpatGains{};
    // Last effective gains used in spatialization algorithms. Used for interpolation.
    StaticMap<source_index_t, SpeakersSpatGains, MAX_INPUTS> mLastSpatGains{};

public:
    AbstractSpatAlgorithm() = default;
    virtual ~AbstractSpatAlgorithm() = default;
    //==============================================================================
    AbstractSpatAlgorithm(AbstractSpatAlgorithm const &) = delete;
    AbstractSpatAlgorithm(AbstractSpatAlgorithm &&) = delete;
    AbstractSpatAlgorithm & operator=(AbstractSpatAlgorithm const &) = delete;
    AbstractSpatAlgorithm & operator=(AbstractSpatAlgorithm &&) = delete;
    //==============================================================================
    virtual void init(SpatGrisData::SpeakersData const & speakers) = 0;
    [[nodiscard]] virtual SpatMode getSpatMode() const noexcept = 0;
    virtual void updateSourcePosition(source_index_t sourceIndex, PolarVector const & position) = 0;
    void process(SourceAudioBuffer const & inputBuffer,
                 SpeakerAudioBuffer & outputBuffer,
                 AudioConfig const & config,
                 StaticMap<source_index_t, float, MAX_INPUTS> const & sourcePeaks)
    {
        for (auto const & source : config.sourcesAudioConfig) {
            auto const & peak{ sourcePeaks[source.key] };
            if (source.value.isMuted || source.value.directOut || peak < SMALL_GAIN) {
                continue;
            }
            for (auto const & speaker : config.speakersAudioConfig) {
                if (speaker.value.isMuted || speaker.value.isDirectOutOnly || speaker.value.gain < SMALL_GAIN) {
                    continue;
                }
                auto const * inputData{ inputBuffer[source.key].getReadPointer(0) };
                auto * outputData{ outputBuffer[speaker.key].getWritePointer(0) };
                processChannel(inputData, outputData, source.key, speaker.key);
            }
        }
    }
    //==============================================================================
    [[nodiscard]] static std::unique_ptr<AbstractSpatAlgorithm> make(SpatMode spatMode);

protected:
    virtual void processChannel(float const * input,
                                float * output,
                                source_index_t sourceIndex,
                                output_patch_t outputPatch,
                                AudioConfig const & config)
        = 0;
    void updateGains(source_index_t const sourceIndex, PolarVector const & position)
    {
        JUCE_ASSERT_MESSAGE_THREAD;
        static auto gains{ std::make_unique<SpeakersSpatGains>() };
        *gains = computeSpeakerGains(position);
        gains.reset(mSpatGains[sourceIndex].exchange(gains.release()));
    }
    [[nodiscard]] virtual SpeakersSpatGains computeSpeakerGains(PolarVector const & sourcePosition) const noexcept = 0;
};
