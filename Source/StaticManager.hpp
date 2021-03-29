#pragma once

#include <bitset>

#include "StrongTypes.hpp"

template<typename key_t, typename value_t, size_t CAPACITY>
class StaticManager
{
    static_assert(std::is_default_constructible_v<value_t>);
    static_assert(std::is_same_v<key_t, speaker_index_t> || std::is_same_v<key_t, output_patch_t>);
#ifdef NDEBUG
    // StaticManager has assertion checks that make it non-trivially-destructible in debug mode.
    static_assert(std::is_trivially_destructible_v<value_t>);
#endif

    std::array<value_t, CAPACITY> mData{};
    std::bitset<CAPACITY> mUsed{};
    juce::CriticalSection mCriticalSection{};

public:
    class iterator
    {
        StaticManager & mManager;
        size_t mIndex;

    public:
        using iterator_category = std::forward_iterator_tag;
        using difference_type = std::ptrdiff_t;
        using value_type = value_t;
        using pointer = value_type *;
        using reference = value_type &;

        iterator(StaticManager & manager, size_t const index) : mManager(manager), mIndex(index) { skipUntilValid(); }
        [[nodiscard]] reference operator*() const { return mManager[mIndex]; }
        [[nodiscard]] pointer operator->() { return &mManager[mIndex]; }

        iterator & operator++()
        {
            ++mIndex;
            skipUntilValid();
            return *this;
        }
        [[nodiscard]] iterator operator++(int)
        {
            iterator temp{ mManager, mIndex };
            ++(*this);
            return temp;
        }

        [[nodiscard]] bool operator==(iterator const & other) const { return mIndex == other.mIndex; }
        [[nodiscard]] bool operator!=(iterator const & other) const { return mIndex != other.mIndex; }

    private:
        void skipUntilValid()
        {
            while (mIndex < CAPACITY && !mManager.mUsed.test(mIndex)) {
                ++mIndex;
            }
        }
    };
    class const_iterator
    {
        StaticManager const & mManager;
        size_t mIndex;

    public:
        using iterator_category = std::forward_iterator_tag;
        using difference_type = std::ptrdiff_t;
        using value_type = value_t;
        using pointer = value_type const *;
        using reference = value_type const &;

        const_iterator(StaticManager const & manager, size_t const index) : mManager(manager), mIndex(index)
        {
            skipUntilValid();
        }
        [[nodiscard]] reference operator*() const { return mManager[mIndex]; }
        [[nodiscard]] pointer operator->() { return &mManager[mIndex]; }

        const_iterator & operator++()
        {
            ++mIndex;
            skipUntilValid();
            return *this;
        }
        [[nodiscard]] iterator operator++(int)
        {
            iterator temp{ mManager, mIndex };
            ++(*this);
            return temp;
        }

        [[nodiscard]] bool operator==(const_iterator const & other) const { return mIndex == other.mIndex; }
        [[nodiscard]] bool operator!=(const_iterator const & other) const { return mIndex != other.mIndex; }

    private:
        void skipUntilValid()
        {
            while (mIndex < CAPACITY && !mManager.mUsed.test(mIndex)) {
                ++mIndex;
            }
        }
    };
    friend const_iterator;

    void clear() { mUsed.reset(); }
    void setAll(value_t const & value)
    {
        std::fill_n(mData.begin(), CAPACITY, value);
        mUsed.set();
    }

    value_t & add(key_t const key, value_t && value)
    {
        auto const index{ toIndex(key) };
        jassert(!mUsed.test(index));
        mData[index] = std::forward<value_t>(value);
        mUsed.set(index);
        return mData[index];
    }

    void remove(key_t const key)
    {
        auto const index{ toIndex(key) };
        jassert(mUsed.test(index));
        mUsed.set(index, false);
    }

    [[nodiscard]] value_t & operator[](key_t const key)
    {
        auto const index{ toIndex(key) };
        jassert(mUsed.test(index));
        return mData[index];
    }

    [[nodiscard]] value_t const & operator[](key_t const key) const
    {
        auto const index{ toIndex(key) };
        jassert(mUsed.test(index));
        return mData[index];
    }

    auto const & getCriticalSection() const { return mCriticalSection; }

    [[nodiscard]] iterator begin() { return iterator{ *this, 0 }; }
    [[nodiscard]] iterator end() { return iterator{ *this, CAPACITY }; }
    [[nodiscard]] const_iterator begin() const { return const_iterator{ *this, 0 }; }
    [[nodiscard]] const_iterator end() const { return const_iterator{ *this, CAPACITY }; }
    [[nodiscard]] const_iterator cbegin() const { return const_iterator{ *this, 0 }; }
    [[nodiscard]] const_iterator cend() const { return const_iterator{ *this, CAPACITY }; }

private:
    [[nodiscard]] size_t toIndex(key_t const key) const
    {
        auto const index{ narrow<size_t>(key.get() - 1) };
        jassert(index < CAPACITY);
        return index;
    }

    [[nodiscard]] value_t & operator[](size_t const index)
    {
        jassert(index < CAPACITY);
        return mData[index];
    }
    [[nodiscard]] value_t const & operator[](size_t const index) const
    {
        jassert(index < CAPACITY);
        return mData[index];
    }
};