#pragma once

#include "LocklessQueue.h"

#define ASSERT_NOT_MESSAGE_THREAD jassert(!juce::MessageManager::getInstance()->isThisTheMessageThread())

template<typename T>
class Pool
{
    static constexpr size_t BUFFER_SIZE = 1024;

    LocklessQueue<T *, BUFFER_SIZE> mLocklessWaitingQueue{};
    juce::OwnedArray<T> mPool{};

public:
    void giveBack(T * ptr)
    {
        if (mLocklessWaitingQueue.free() == 0) {
            JUCE_ASSERT_MESSAGE_THREAD;
            mPool.add(ptr);
            return;
        }

        mLocklessWaitingQueue.add(ptr);
    }

    void cleanup()
    {
        JUCE_ASSERT_MESSAGE_THREAD;
        tl::optional<T *> ptr{};
        while ((ptr = mLocklessWaitingQueue.consume())) {
            mPool.add(*ptr);
        }
    }

    T * acquire()
    {
        JUCE_ASSERT_MESSAGE_THREAD;
        if (mPool.size() <= 0) {
            return new T{};
        }

        auto * ptr{ mPool.getLast() };
        mPool.removeLast(1, false);
        return ptr;
    }
};