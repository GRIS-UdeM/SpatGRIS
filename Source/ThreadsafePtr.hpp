#pragma once

#include "Pool.hpp"

template<typename T>
class ThreadsafePtr
{
    T * mCurrentValue{};
    std::atomic<T *> mPendingValue{};

public:
    //==============================================================================
    static Pool<T> pool;
    //==============================================================================
    ThreadsafePtr() : mCurrentValue(pool.acquire()) {}
    ~ThreadsafePtr() = default;
    ThreadsafePtr(ThreadsafePtr const &) = default;
    ThreadsafePtr(ThreadsafePtr &&) = default;
    ThreadsafePtr & operator=(ThreadsafePtr const &) = default;
    ThreadsafePtr & operator=(ThreadsafePtr &&) = default;
    //==============================================================================

    T const * get()
    {
        if (mPendingValue.load() == nullptr) {
            return mCurrentValue;
        }

        pool.giveBack(mCurrentValue);
        mCurrentValue = mPendingValue.exchange(nullptr);
        jassert(mCurrentValue);
        return mCurrentValue;
    }

    void set(T * const value)
    {
        jassert(pool.mOwnedData.contains(value));
        auto * oldValue{ mPendingValue.exchange(value) };
        if (oldValue) {
            pool.giveBack(oldValue);
        }
    }

    // void releaseResources()
    //{
    //    JUCE_ASSERT_MESSAGE_THREAD;
    //    delete mPendingValue.exchange(nullptr);
    //    delete mCurrentValue;
    //    mCurrentValue = nullptr;
    //}

    // private:
    //#ifndef NDEBUG
    //    void assertEmpty() const
    //    {
    //        JUCE_ASSERT_MESSAGE_THREAD;
    //        jassert(mCurrentValue == nullptr);
    //        jassert(mPendingValue.load() == nullptr);
    //    }
    //#endif
};