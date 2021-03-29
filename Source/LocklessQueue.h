#pragma once

#include <JuceHeader.h>

#include "lib/tl/optional.hpp"

//==============================================================================
template<typename T, size_t BufferSize>
class LocklessQueue
{
    static_assert(std::atomic<int>::is_always_lock_free);
    //==============================================================================
    std::atomic<int> mProduceCount{};
    std::atomic<int> mConsumeCount{};

    std::array<T, BufferSize> mBuffer{};

public:
    //==============================================================================
    [[nodiscard]] size_t used() const { return mProduceCount - mConsumeCount; }
    [[nodiscard]] size_t free() const { return BufferSize - used(); }
    //==============================================================================
    void add(T newValue)
    {
        jassert(free() > 0);
        auto const index{ mProduceCount % BufferSize };
        mBuffer[index] = newValue;
        ++mProduceCount;
    }
    //==============================================================================
    tl::optional<T> consume()
    {
        if (used() == 0) {
            return tl::nullopt;
        }

        auto const index{ mConsumeCount % BufferSize };
        auto const result{ mBuffer[index] };
        ++mConsumeCount;
        return result;
    }
};
