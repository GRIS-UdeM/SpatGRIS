#pragma once

#include "LocklessQueue.h"

#define ASSERT_NOT_MESSAGE_THREAD jassert(!juce::MessageManager::getInstance()->isThisTheMessageThread())

//==============================================================================
template<typename T>
class Pool
{
    static constexpr size_t BUFFER_SIZE = 1024;

    juce::Array<T *> mFreeObjects{};
    LocklessQueue<T *, BUFFER_SIZE> mPendingFreeObjects{};
#ifndef NDEBUG
public:
#endif
    juce::OwnedArray<T> mOwnedData{};

public:
    //==============================================================================
    Pool() = default;
    explicit Pool(size_t const prealloc)
    {
        for (size_t i{}; i < prealloc; ++i) {
            mFreeObjects.add(mOwnedData.add(new T{}));
        }
    }
    ~Pool() = default;
    //==============================================================================
    Pool(Pool const &) = delete;
    Pool(Pool &&) = default;
    Pool & operator=(Pool const &) = delete;
    Pool & operator=(Pool &&) = default;
    //==============================================================================
    void giveBack(T * ptr)
    {
        if (mPendingFreeObjects.free() > 0) {
            mPendingFreeObjects.add(ptr);
        } else {
            // JUCE_ASSERT_MESSAGE_THREAD;
            mFreeObjects.add(ptr);
        }
    }

    void transferPendingObjects()
    {
        tl::optional<T *> ptr{};
        while ((ptr = mPendingFreeObjects.consume()).has_value()) {
            mFreeObjects.add(*ptr);
        }
    }

    T * acquire()
    {
        if (mFreeObjects.size() <= 0) {
            // JUCE_ASSERT_MESSAGE_THREAD;
            return mOwnedData.add(new T{});
        }

        auto * ptr{ mFreeObjects.getLast() };
        mFreeObjects.removeLast(1);
        return ptr;
    }
};