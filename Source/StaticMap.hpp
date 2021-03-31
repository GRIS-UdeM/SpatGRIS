#pragma once

#include <bitset>

#include "StrongTypes.hpp"

template<typename KeyType, typename ValueType, size_t Capacity>
class StaticMap
{
    static_assert(std::is_base_of_v<StrongIndexBase, KeyType>);
    static_assert(std::is_default_constructible_v<ValueType>);
#ifdef NDEBUG
    // StaticManager has assertion checks that make it non-trivial in debug mode.
    static_assert(std::is_trivially_copyable_v<value_t>);
    static_assert(std::is_trivially_copy_assignable_v<value_t>);
    static_assert(std::is_trivially_destructible_v<value_t>);
#endif
public:
    //==============================================================================
    using key_type = KeyType;
    using value_type = ValueType;
    struct Node {
        KeyType key;
        ValueType value;
    };
    static constexpr auto CAPACITY = Capacity;

private:
    //==============================================================================
    std::array<Node, CAPACITY> mData{};
    std::bitset<CAPACITY> mUsed{};

public:
    //==============================================================================
    class iterator
    {
        StaticMap & mManager;
        size_t mIndex;

    public:
        using iterator_category = std::forward_iterator_tag;
        using difference_type = std::ptrdiff_t;
        using value_type = Node;
        using pointer = value_type *;
        using reference = value_type &;

        iterator(StaticMap & manager, size_t const index) : mManager(manager), mIndex(index) { skipUntilValid(); }
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
            mIndex = std::min(mIndex, CAPACITY);
        }
    };
    //==============================================================================
    class const_iterator
    {
        StaticMap const & mManager;
        size_t mIndex;

    public:
        using iterator_category = std::forward_iterator_tag;
        using difference_type = std::ptrdiff_t;
        using value_type = Node;
        using pointer = value_type const *;
        using reference = value_type const &;

        const_iterator(StaticMap const & manager, size_t const index) : mManager(manager), mIndex(index)
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
        [[nodiscard]] const_iterator operator++(int)
        {
            const_iterator temp{ mManager, mIndex };
            ++(*this);
            return temp;
        }

        [[nodiscard]] bool operator==(const_iterator const & other) const { return mIndex == other.mIndex; }
        [[nodiscard]] bool operator!=(const_iterator const & other) const { return mIndex != other.mIndex; }

    private:
        void skipUntilValid()
        {
            while (mIndex < Capacity && !mManager.mUsed.test(mIndex)) {
                ++mIndex;
            }
        }
    };
    friend const_iterator;
    //==============================================================================
    [[nodiscard]] bool isEmpty() const { return size() == 0; }
    [[nodiscard]] size_t size() const { return mUsed.count(); }
    void clear() { mUsed.reset(); }

    void add(KeyType const key, ValueType const & value)
    {
        auto const index{ toIndex(key) };
        jassert(!mUsed.test(index));
        Node const newNode{ key, value };
        mData[index] = newNode;
        mUsed.set(index);
    }

    void remove(KeyType const key)
    {
        auto const index{ toIndex(key) };
        jassert(mUsed.test(index));
        mUsed.set(index, false);
    }

    [[nodiscard]] ValueType & operator[](KeyType const key)
    {
        auto const index{ toIndex(key) };
        jassert(mUsed.test(index));
        return mData[index].value;
    }

    [[nodiscard]] ValueType const & operator[](KeyType const key) const
    {
        auto const index{ toIndex(key) };
        jassert(mUsed.test(index));
        return mData[index].value;
    }

    [[nodiscard]] iterator begin() { return iterator{ *this, 0 }; }
    [[nodiscard]] iterator end() { return iterator{ *this, CAPACITY }; }
    [[nodiscard]] const_iterator begin() const { return const_iterator{ *this, 0 }; }
    [[nodiscard]] const_iterator end() const { return const_iterator{ *this, CAPACITY }; }
    [[nodiscard]] const_iterator cbegin() const { return const_iterator{ *this, 0 }; }
    [[nodiscard]] const_iterator cend() const { return const_iterator{ *this, CAPACITY }; }

private:
    [[nodiscard]] size_t toIndex(KeyType const key) const
    {
        auto const index{ narrow<size_t>(key.get() - KeyType::OFFSET) };
        jassert(index < CAPACITY);
        return index;
    }

    [[nodiscard]] Node & operator[](size_t const index)
    {
        jassert(index < CAPACITY);
        return mData[index];
    }
    [[nodiscard]] Node const & operator[](size_t const index) const
    {
        jassert(index < CAPACITY);
        return mData[index];
    }
};