#pragma once

#include "macros.h"
DISABLE_WARNINGS
#include <JuceHeader.h>
ENABLE_WARNINGS

#include "Manager.hpp"
#include "Speaker.h"
#include "StaticVector.h"
#include "StrongTypes.hpp"

template<size_t CAPACITY>
class TaggedAudioBuffer
{
    struct ChannelInfo {
        speaker_id_t speakerId;
        output_patch_t outputPatch;
        float * buffer;
    };

    using container_type = StaticVector<ChannelInfo, CAPACITY>;

    StaticVector<ChannelInfo, CAPACITY> mChannelInfo{};
    juce::AudioBuffer<float> mBuffer{};

public:
    using iterator = typename container_type::iterator;
    using const_iterator = typename container_type::const_iterator;

    static auto constexpr MAX_BUFFER_LENGTH = 4096;

    int getNumChannels() const
    {
        jassert(mBuffer.getNumChannels() == mChannelInfo.size());
        return mChannelInfo.size();
    }
    void clear() { mBuffer.clear(); }
    void setSpeakers(Manager<Speaker, speaker_id_t> const & speakers)
    {
        juce::ScopedLock const lock{ speakers.getLock() };
        jassert(speakers.size() <= narrow<int>(CAPACITY));
        mBuffer.setSize(speakers.size(), MAX_BUFFER_LENGTH);
        mChannelInfo.resize(narrow<size_t>(speakers.size()));
        int channelCount{};
        std::transform(speakers.cbegin(),
                       speakers.cend(),
                       mChannelInfo.begin(),
                       [&](Speaker const * speaker) -> ChannelInfo {
                           return ChannelInfo{ speaker->getSpeakerId(),
                                               speaker->getOutputPatch(),
                                               mBuffer.getWritePointer(channelCount++) };
                       });
        std::sort(mChannelInfo.begin(), mChannelInfo.end(), [](ChannelInfo const & a, ChannelInfo const & b) -> bool {
            return a.speakerId < b.speakerId;
        });
    }

    [[nodiscard]] float * getWritePointerByOutputPatch(output_patch_t const outputPatch)
    {
        jassert(
            std::count_if(cbegin(), cend(), [&](ChannelInfo const & info) { return info.outputPatch == outputPatch; })
            == 1);
        auto const info{ std::find_if(begin(), end(), [&](ChannelInfo const info) {
            return info.outputPatch == outputPatch;
        }) };
        return info->buffer;
    }

    [[nodiscard]] juce::AudioBuffer<float> getUnderlyingBuffer(int const numSamples)
    {
        jassert(numSamples <= MAX_BUFFER_LENGTH);
        return juce::AudioBuffer<float>{ mBuffer.getArrayOfWritePointers(),
                                         mBuffer.getNumChannels(),
                                         mBuffer.getNumSamples() };
    }
    [[nodiscard]] juce::AudioBuffer<float> getChannel(speaker_id_t const id, int const numSamples)
    {
        jassert(numSamples <= MAX_BUFFER_LENGTH);
        auto const index{ getIndexOf(id) };
        auto * const data{ mBuffer.getWritePointer(index) };
        return juce::AudioBuffer<float>{ &data, 1, numSamples };
    }

    [[nodiscard]] float * getWritePointer(speaker_id_t const id)
    {
        auto const index{ getIndexOf(id) };
        return mBuffer.getWritePointer(index);
    }
    [[nodiscard]] float const * getReadPointer(speaker_id_t const id) const
    {
        auto const index{ getIndexOf(id) };
        return mBuffer.getReadPointer(index);
    }
    [[nodiscard]] StaticVector<float const *, CAPACITY>
        getArrayOfReadPointers(StaticVector<speaker_id_t, CAPACITY> const & ids) const
    {
        StaticVector<float const *, CAPACITY> result{};
        for (auto const id : ids) {
            auto const index{ getIndexOf(id) };
            result.push_back(mBuffer.getReadPointer(index));
        }
        return result;
    }
    void copyToPhysicalOutput(float * const * outs, int const numOutputs, int const numSamples) const
    {
        jassert(numSamples <= mBuffer.getNumSamples());
        int bufferIndex{};
        for (auto const & channelInfo : mChannelInfo) {
            auto const outIndex{ channelInfo.outputPatch.get() - 1 };
            jassert(outIndex >= 0);
            if (outIndex < numOutputs) {
                auto const * origin{ mBuffer.getReadPointer(bufferIndex) };
                auto * dest{ outs[outIndex] };
                std::transform(origin, origin + numSamples, dest, dest, std::plus());
            }
            ++bufferIndex;
        }
    }

    const_iterator begin() const { return mChannelInfo.cbegin(); }
    const_iterator end() const { return mChannelInfo.cend(); }
    const_iterator cbegin() const { return mChannelInfo.cbegin(); }
    const_iterator cend() const { return mChannelInfo.cend(); }

private:
    [[nodiscard]] int getIndexOf(speaker_id_t const id) const
    {
        auto const it{ std::lower_bound(
            mChannelInfo.cbegin(),
            mChannelInfo.cend(),
            id,
            [](ChannelInfo const & channelInfo, speaker_id_t const id) { return channelInfo.speakerId < id; }) };
        jassert(it != mChannelInfo.cend());
        auto const index{ narrow<int>(it - mChannelInfo.cbegin()) };
        jassert(index >= 0 && index < mBuffer.getNumChannels());
        return index;
    }
};