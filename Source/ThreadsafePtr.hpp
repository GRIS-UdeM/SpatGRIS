#pragma once

#include "Pool.hpp"

template<typename T>
class ThreadsafePtr
{
    T * mCurrentValue{};
    juce::Atomic<T *> mPendingValue{};

public:
    //==============================================================================
#ifndef NDEBUG
    ~ThreadsafePtr()
    {
        JUCE_ASSERT_MESSAGE_THREAD;
        jassert(mCurrentValue == nullptr);
        jassert(mPendingValue.get() == nullptr);
    }
#endif
    //==============================================================================
    ThreadsafePtr & operator=(ThreadsafePtr const &) = delete;
    ThreadsafePtr & operator=(ThreadsafePtr &&) = delete;
    //==============================================================================

    T const * get(Pool<T> & pool)
    {
        // This should only get called from the audio thread in order to make sure that the SpatParams are the most
        // recent ones available.
        ASSERT_NOT_MESSAGE_THREAD;
        if (!mPendingValue.get()) {
            return mCurrentValue;
        }

        pool.giveBack(mCurrentValue);
        auto * const newValue{ mPendingValue.exchange(nullptr) };
        jassert(newValue);
        mCurrentValue = newValue;
        return newValue;
    }

    void set(T * const value, Pool<T> & pool)
    {
        // This should only get called by the message thread to push a new set of SpatParams to get picked up by the
        // audio thread.
        JUCE_ASSERT_MESSAGE_THREAD;
        auto * oldValue{ mPendingValue.exchange(value) };
        if (oldValue) {
            pool.giveBack(oldValue);
        }
        pool.cleanup();
    }

    void free()
    {
        JUCE_ASSERT_MESSAGE_THREAD;
        delete mCurrentValue;
        delete mPendingValue.get();
    }
};