/*
 This file is part of SpatGRIS.

 Developers: Samuel Béland, Olivier Bélanger, Nicolas Masson

 SpatGRIS is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 SpatGRIS is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with SpatGRIS.  If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

#include "StrongTypes.hpp"

#include <bitset>

//==============================================================================
template<typename KeyType, typename ValueType, size_t CAPACITY>
class OwnedMap
{
    static_assert(std::is_base_of_v<StrongIndexBase, KeyType>);
    //==============================================================================
public:
    struct Node {
        KeyType key{};
        ValueType * value{};
    };
    struct ConstNode {
        KeyType key{};
        ValueType const * value{};
    };

private:
    //==============================================================================
    std::array<Node, CAPACITY> mData{};
    std::bitset<CAPACITY> mUsed{};

public:
    //==============================================================================
    OwnedMap()
    {
        KeyType key{ KeyType::OFFSET };
        for (auto & node : mData) {
            node.key = key++;
        }
    }
    ~OwnedMap() noexcept
    {
        for (auto & node : *this) {
            delete node.value;
        }
    }
    //==============================================================================
    OwnedMap(OwnedMap const &) = delete;
    OwnedMap(OwnedMap && other) noexcept : mData(other.mData), mUsed(other.mUsed) { other.mUsed.reset(); }
    OwnedMap & operator=(OwnedMap const &) = delete;
    OwnedMap & operator=(OwnedMap && other) noexcept
    {
        clear();
        mData = other.mData;
        mUsed = other.mUsed;
        other.mUsed.reset();
        return *this;
    }
    //==============================================================================
    [[nodiscard]] bool operator==(OwnedMap const & other) const
    {
        if (mUsed != other.mUsed) {
            return false;
        }

        return std::all_of(this->cbegin(), this->cend(), [&](ConstNode const & node) {
            return *node.value == other[node.key];
        });
    }
    //==============================================================================
    void clear() noexcept
    {
        for (auto & node : *this) {
            delete node.value;
        }
        mUsed.reset();
    }
    //==============================================================================
    [[nodiscard]] int size() const noexcept { return narrow<int>(mUsed.count()); }
    //==============================================================================
    [[nodiscard]] ValueType & operator[](KeyType const key) { return *getNode(key).value; }
    [[nodiscard]] ValueType const & operator[](KeyType const key) const { return *getConstNode(key).value; }
    //==============================================================================
    [[nodiscard]] Node & getNode(KeyType const & key) noexcept
    {
        auto const index{ toIndex(key) };
        jassert(index < CAPACITY);
        jassert(mUsed.test(index));
        return mData[index];
    }
    [[nodiscard]] ConstNode const & getConstNode(KeyType const & key) const noexcept
    {
        auto const index{ toIndex(key) };
        jassert(index < CAPACITY);
        jassert(mUsed.test(index));
        return reinterpret_cast<ConstNode const &>(mData[index]);
    }
    //==============================================================================
    [[nodiscard]] size_t toIndex(KeyType const & key) const noexcept
    {
        return narrow<size_t>(key.get() - KeyType::OFFSET);
    }
    //==============================================================================
    [[nodiscard]] bool contains(KeyType const & key) const noexcept
    {
        auto const index{ toIndex(key) };
        jassert(index < CAPACITY);
        return mUsed.test(index);
    }
    //==============================================================================
    [[nodiscard]] KeyType getNextUsedKey(KeyType const & key) const noexcept
    {
        auto index{ toIndex(key) };
        if (index >= CAPACITY) {
            return key;
        }
        do {
            ++index;
        } while (index < CAPACITY && !mUsed.test(index));
        return KeyType{ narrow<KeyType::type>(index) + KeyType::OFFSET };
    }
    //==============================================================================
    [[nodiscard]] KeyType getFirstUsedKey() const noexcept
    {
        KeyType const key{ KeyType::OFFSET };
        if (contains(key)) {
            return key;
        }
        return getNextUsedKey(key);
    }
    //==============================================================================
    ValueType & add(KeyType const key, std::unique_ptr<ValueType> value) noexcept
    {
        auto const index{ toIndex(key) };
        jassert(index < CAPACITY);
        jassert(!mUsed.test(index));
        jassert(mData[index].key == key);
        mData[index].value = value.release();
        mUsed.set(index);
        return *mData[index].value;
    }
    //==============================================================================
    void remove(KeyType const key) noexcept
    {
        auto const index{ toIndex(key) };
        jassert(index < CAPACITY);
        jassert(mUsed.test(index));
        delete mData[index].value;
        mUsed.reset(index);
    }

    //==============================================================================
    class iterator
    {
        OwnedMap * mOwnedMap{};
        KeyType mCurrentKey{};

    public:
        //==============================================================================
        iterator() = default;
        iterator(OwnedMap & map, KeyType const key) : mOwnedMap(&map), mCurrentKey(key) {}
        ~iterator() = default;
        iterator(iterator const &) = default;
        iterator(iterator &&) = default;
        iterator & operator=(iterator const &) = default;
        iterator & operator=(iterator &&) = default;
        //==============================================================================
        using iterator_category = std::forward_iterator_tag;
        using difference_type = std::ptrdiff_t;
        using value_type = Node;
        using pointer = Node *;
        using reference = Node &;
        //==============================================================================
        [[nodiscard]] bool operator==(iterator const & other) const noexcept
        {
            return mCurrentKey == other.mCurrentKey;
        }
        [[nodiscard]] bool operator!=(iterator const & other) const noexcept
        {
            return mCurrentKey != other.mCurrentKey;
        }
        [[nodiscard]] bool operator<(iterator const & other) const noexcept { return mCurrentKey < other.mCurrentKey; }
        //==============================================================================
        [[nodiscard]] reference operator*() { return mOwnedMap->getNode(mCurrentKey); }
        [[nodiscard]] reference operator*() const { return mOwnedMap->getNode(mCurrentKey); }
        [[nodiscard]] pointer operator->() { return &mOwnedMap->getNode(mCurrentKey); }
        [[nodiscard]] pointer operator->() const { return &mOwnedMap->getNode(mCurrentKey); }
        //==============================================================================
        iterator & operator++() noexcept
        {
            mCurrentKey = mOwnedMap->getNextUsedKey(mCurrentKey);
            return *this;
        }
        [[nodiscard]] iterator operator++(int) noexcept
        {
            auto const temp{ *this };
            ++(*this);
            return temp;
        }
    };

    //==============================================================================
    class const_iterator
    {
        OwnedMap const * mOwnedMap{};
        KeyType mCurrentKey{};

    public:
        //==============================================================================
        const_iterator() = default;
        const_iterator(OwnedMap const & map, KeyType const key) : mOwnedMap(&map), mCurrentKey(key) {}
        ~const_iterator() = default;
        const_iterator(const_iterator const &) = default;
        const_iterator(const_iterator &&) = default;
        const_iterator & operator=(const_iterator const &) = default;
        const_iterator & operator=(const_iterator &&) = default;
        //==============================================================================
        using iterator_category = std::forward_iterator_tag;
        using difference_type = std::ptrdiff_t;
        using value_type = ConstNode;
        using pointer = ConstNode const *;
        using reference = ConstNode const &;
        //==============================================================================
        [[nodiscard]] bool operator==(const_iterator const & other) const noexcept
        {
            return mCurrentKey == other.mCurrentKey;
        }
        [[nodiscard]] bool operator!=(const_iterator const & other) const noexcept
        {
            return mCurrentKey != other.mCurrentKey;
        }
        [[nodiscard]] bool operator<(const_iterator const & other) const noexcept
        {
            return mCurrentKey < other.mCurrentKey;
        }
        //==============================================================================
        [[nodiscard]] reference operator*() { return mOwnedMap->getConstNode(mCurrentKey); }
        [[nodiscard]] reference operator*() const { return mOwnedMap->getConstNode(mCurrentKey); }
        [[nodiscard]] pointer operator->() { return &mOwnedMap->getConstNode(mCurrentKey); }
        [[nodiscard]] pointer operator->() const { return &mOwnedMap->getConstNode(mCurrentKey); }
        //==============================================================================
        const_iterator & operator++() noexcept
        {
            mCurrentKey = mOwnedMap->getNextUsedKey(mCurrentKey);
            return *this;
        }
        [[nodiscard]] const_iterator operator++(int) noexcept
        {
            auto const temp{ *this };
            ++(*this);
            return temp;
        }
    };

    //==============================================================================
    [[nodiscard]] iterator begin() noexcept { return iterator{ *this, getFirstUsedKey() }; }
    [[nodiscard]] iterator end() noexcept
    {
        return iterator{ *this, KeyType{ narrow<typename KeyType::type>(CAPACITY) + KeyType::OFFSET } };
    }
    [[nodiscard]] const_iterator begin() const noexcept { return const_iterator{ *this, getFirstUsedKey() }; }
    [[nodiscard]] const_iterator end() const noexcept
    {
        return const_iterator{ *this, KeyType{ narrow<typename KeyType::type>(CAPACITY) + KeyType::OFFSET } };
    }
    [[nodiscard]] const_iterator cbegin() const noexcept { return const_iterator{ *this, getFirstUsedKey() }; }
    [[nodiscard]] const_iterator cend() const noexcept
    {
        return const_iterator{ *this, KeyType{ narrow<typename KeyType::type>(CAPACITY) + KeyType::OFFSET } };
    }

private:
    //==============================================================================
    JUCE_LEAK_DETECTOR(OwnedMap)
};
