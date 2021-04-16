#pragma once

#include "Pool.hpp"

template<typename T>
class ThreadsafePtr
{
    T * mCurrentValue{};
    juce::Atomic<T *> mPendingValue{};

public:
    //==============================================================================
    static Pool<T> pool;
    //==============================================================================
    ThreadsafePtr() = default;
#ifndef NDEBUG
    ~ThreadsafePtr() { assertEmpty(); }
    ThreadsafePtr(ThreadsafePtr const & other)
    {
        assertEmpty();
        other.assertEmpty();
    }
    ThreadsafePtr(ThreadsafePtr const && other) noexcept
    {
        assertEmpty();
        other.assertEmpty();
    }
    ThreadsafePtr & operator=(ThreadsafePtr const & other)
    {
        if (&other == this) {
            return *this;
        }
        assertEmpty();
        other.assertEmpty();
        return *this;
    }
    ThreadsafePtr & operator=(ThreadsafePtr && other) noexcept
    {
        assertEmpty();
        other.assertEmpty();
        return *this;
    }
#else
    ~ThreadsafePtr() = default;
    ThreadsafePtr(ThreadsafePtr const &) = default;
    ThreadsafePtr(ThreadsafePtr &&) = default;
    ThreadsafePtr & operator=(ThreadsafePtr const &) = default;
    ThreadsafePtr & operator=(ThreadsafePtr &&) = default;
#endif
    //==============================================================================

    T const * get()
    {
        if (mPendingValue.get() == nullptr) {
            return mCurrentValue;
        }

        pool.giveBack(mCurrentValue);
        mCurrentValue = mPendingValue.exchange(nullptr);
        jassert(mCurrentValue);
        return mCurrentValue;
    }

    void set(T * const value)
    {
        auto * oldValue{ mPendingValue.exchange(value) };
        if (oldValue) {
            pool.giveBack(oldValue);
        }
    }

    void releaseResources()
    {
        JUCE_ASSERT_MESSAGE_THREAD;
        delete mCurrentValue;
        delete mPendingValue.get();
    }

private:
#ifndef NDEBUG
    void assertEmpty() const
    {
        JUCE_ASSERT_MESSAGE_THREAD;
        jassert(mCurrentValue == nullptr);
        jassert(mPendingValue.get() == nullptr);
    }
#endif
};