#pragma once

#include "LocklessQueue.h"

#define ASSERT_NOT_MESSAGE_THREAD jassert(!juce::MessageManager::getInstance()->isThisTheMessageThread())

template<typename T>
class Pool
{
    static constexpr size_t BUFFER_SIZE = 1024;

    juce::Array<T *> mFreeObjects{};
    LocklessQueue<T *, BUFFER_SIZE> mPendingFreeObjects{};
    juce::OwnedArray<T> mOwnedData{};

public:
    void giveBack(T * ptr)
    {
        if (mPendingFreeObjects.free() > 0) {
            mPendingFreeObjects.add(ptr);
        } else {
            JUCE_ASSERT_MESSAGE_THREAD;
            mFreeObjects.add(ptr);
        }
    }

    void transferPendingObjects()
    {
        JUCE_ASSERT_MESSAGE_THREAD;
        tl::optional<T *> ptr{};
        while ((ptr = mPendingFreeObjects.consume()).has_value()) {
            mFreeObjects.add(*ptr);
        }
    }

    T * acquire()
    {
        if (mFreeObjects.size() <= 0) {
            JUCE_ASSERT_MESSAGE_THREAD;
            return mOwnedData.add(new T{});
        }

        auto * ptr{ mFreeObjects.getLast() };
        mFreeObjects.removeLast(1);
        return ptr;
    }
};