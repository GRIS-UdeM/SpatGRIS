/*
 This file is part of SpatGRIS.

 Developers: Gaël Lane Lépine, Samuel Béland, Olivier Bélanger, Nicolas Masson

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

#include "../Data/StrongTypes/sg_StrongIndex.hpp"

#include <bitset>

namespace gris
{
//==============================================================================
/** A stack-allocated fixed-capacity associative map of objects accessed using a strongly-typed index.
 *
 * Values have to be trivial since the destructor is sometimes omitted.
 */
template<typename KeyType, typename ValueType, size_t Capacity>
class StaticMap
{
    static_assert(std::is_base_of_v<StrongIndexBase, KeyType>);
    static_assert(std::is_default_constructible_v<ValueType>);
    static_assert(std::is_trivially_destructible_v<ValueType>);

public:
    //==============================================================================
    using key_type = KeyType;
    using value_type = ValueType;
    //==============================================================================
    struct Node {
        KeyType key{};
        ValueType value{};
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
        //==============================================================================
        using iterator_category = std::forward_iterator_tag;
        using difference_type = std::ptrdiff_t;
        using value_type = Node;
        using pointer = value_type *;
        using reference = value_type &;
        //==============================================================================
        iterator() = delete;
        iterator(StaticMap & manager, size_t index);
        ~iterator() = default;
        SG_DEFAULT_COPY_AND_MOVE(iterator)
        //==============================================================================
        [[nodiscard]] reference operator*() const;
        [[nodiscard]] pointer operator->();
        //==============================================================================
        iterator & operator++();
        [[nodiscard]] iterator operator++(int);
        //==============================================================================
        [[nodiscard]] bool operator==(iterator const & other) const;
        [[nodiscard]] bool operator!=(iterator const & other) const;

    private:
        //==============================================================================
        void skipUntilValid();
    };

    //==============================================================================
    class const_iterator
    {
        StaticMap const & mManager;
        size_t mIndex;

    public:
        //==============================================================================
        using iterator_category = std::forward_iterator_tag;
        using difference_type = std::ptrdiff_t;
        using value_type = Node;
        using pointer = value_type const *;
        using reference = value_type const &;
        //==============================================================================
        const_iterator() = delete;
        const_iterator(StaticMap const & manager, size_t index);
        ~const_iterator() = default;
        SG_DEFAULT_COPY_AND_MOVE(const_iterator)
        //==============================================================================
        [[nodiscard]] reference operator*() const;
        [[nodiscard]] pointer operator->();
        //==============================================================================
        const_iterator & operator++();
        [[nodiscard]] const_iterator operator++(int);
        //==============================================================================
        [[nodiscard]] bool operator==(const_iterator const & other) const;
        [[nodiscard]] bool operator!=(const_iterator const & other) const;

    private:
        //==============================================================================
        void skipUntilValid();
    };
    friend const_iterator;
    //==============================================================================
    StaticMap();
    ~StaticMap() = default;
    SG_DEFAULT_COPY_AND_MOVE(StaticMap)
    //==============================================================================
    void clear();
    void add(KeyType key, ValueType const & value);
    void add(KeyType key);
    void remove(KeyType key);
    void fill(KeyType firstKey, ValueType const & value);
    [[nodiscard]] bool isEmpty() const;
    [[nodiscard]] size_t size() const;
    [[nodiscard]] juce::Array<KeyType> getKeys() const noexcept;
    [[nodiscard]] bool contains(KeyType key) const noexcept;
    [[nodiscard]] bool hasSameKeys(StaticMap const & other) const noexcept;
    [[nodiscard]] ValueType & operator[](KeyType key);
    [[nodiscard]] ValueType const & operator[](KeyType key) const;
    //==============================================================================
    [[nodiscard]] iterator begin();
    [[nodiscard]] iterator end();
    [[nodiscard]] const_iterator begin() const;
    [[nodiscard]] const_iterator end() const;
    [[nodiscard]] const_iterator cbegin() const;
    [[nodiscard]] const_iterator cend() const;

private:
    //==============================================================================
    [[nodiscard]] size_t toIndex(KeyType key) const;
    [[nodiscard]] Node & operator[](size_t index);
    [[nodiscard]] Node const & operator[](size_t index) const;
};

//==============================================================================
template<typename KeyType, typename ValueType, size_t Capacity>
StaticMap<KeyType, ValueType, Capacity>::iterator::iterator(StaticMap & manager, size_t const index)
    : mManager(manager)
    , mIndex(index)
{
    skipUntilValid();
}

//==============================================================================
template<typename KeyType, typename ValueType, size_t Capacity>
typename StaticMap<KeyType, ValueType, Capacity>::iterator::reference
    StaticMap<KeyType, ValueType, Capacity>::iterator ::operator*() const
{
    return mManager[mIndex];
}

//==============================================================================
template<typename KeyType, typename ValueType, size_t Capacity>
typename StaticMap<KeyType, ValueType, Capacity>::iterator::pointer
    StaticMap<KeyType, ValueType, Capacity>::iterator::operator->()
{
    return &mManager[mIndex];
}

//==============================================================================
template<typename KeyType, typename ValueType, size_t Capacity>
typename StaticMap<KeyType, ValueType, Capacity>::iterator &
    StaticMap<KeyType, ValueType, Capacity>::iterator::operator++()
{
    ++mIndex;
    skipUntilValid();
    return *this;
}

//==============================================================================
template<typename KeyType, typename ValueType, size_t Capacity>
typename StaticMap<KeyType, ValueType, Capacity>::iterator
    StaticMap<KeyType, ValueType, Capacity>::iterator::operator++(int)
{
    iterator temp{ mManager, mIndex };
    ++(*this);
    return temp;
}

//==============================================================================
template<typename KeyType, typename ValueType, size_t Capacity>
bool StaticMap<KeyType, ValueType, Capacity>::iterator::operator==(iterator const & other) const
{
    return mIndex == other.mIndex;
}

//==============================================================================
template<typename KeyType, typename ValueType, size_t Capacity>
bool StaticMap<KeyType, ValueType, Capacity>::iterator::operator!=(iterator const & other) const
{
    return mIndex != other.mIndex;
}

//==============================================================================
template<typename KeyType, typename ValueType, size_t Capacity>
void StaticMap<KeyType, ValueType, Capacity>::iterator::skipUntilValid()
{
    while (mIndex < CAPACITY && !mManager.mUsed.test(mIndex)) {
        ++mIndex;
    }
    mIndex = std::min(mIndex, CAPACITY);
}

//==============================================================================
template<typename KeyType, typename ValueType, size_t Capacity>
StaticMap<KeyType, ValueType, Capacity>::const_iterator::const_iterator(StaticMap const & manager, size_t const index)
    : mManager(manager)
    , mIndex(index)
{
    skipUntilValid();
}

//==============================================================================
template<typename KeyType, typename ValueType, size_t Capacity>
typename StaticMap<KeyType, ValueType, Capacity>::const_iterator::reference
    StaticMap<KeyType, ValueType, Capacity>::const_iterator::operator*() const
{
    return mManager[mIndex];
}

//==============================================================================
template<typename KeyType, typename ValueType, size_t Capacity>
typename StaticMap<KeyType, ValueType, Capacity>::const_iterator::pointer
    StaticMap<KeyType, ValueType, Capacity>::const_iterator::operator->()
{
    return &mManager[mIndex];
}

//==============================================================================
template<typename KeyType, typename ValueType, size_t Capacity>
typename StaticMap<KeyType, ValueType, Capacity>::const_iterator &
    StaticMap<KeyType, ValueType, Capacity>::const_iterator::operator++()
{
    ++mIndex;
    skipUntilValid();
    return *this;
}

//==============================================================================
template<typename KeyType, typename ValueType, size_t Capacity>
typename StaticMap<KeyType, ValueType, Capacity>::const_iterator
    StaticMap<KeyType, ValueType, Capacity>::const_iterator ::operator++(int)
{
    const_iterator temp{ mManager, mIndex };
    ++(*this);
    return temp;
}

//==============================================================================
template<typename KeyType, typename ValueType, size_t Capacity>
bool StaticMap<KeyType, ValueType, Capacity>::const_iterator::operator==(const_iterator const & other) const
{
    return mIndex == other.mIndex;
}

//==============================================================================
template<typename KeyType, typename ValueType, size_t Capacity>
bool StaticMap<KeyType, ValueType, Capacity>::const_iterator::operator!=(const_iterator const & other) const
{
    return mIndex != other.mIndex;
}

//==============================================================================
template<typename KeyType, typename ValueType, size_t Capacity>
void StaticMap<KeyType, ValueType, Capacity>::const_iterator::skipUntilValid()
{
    while (mIndex < Capacity && !mManager.mUsed.test(mIndex)) {
        ++mIndex;
    }
}

//==============================================================================
template<typename KeyType, typename ValueType, size_t Capacity>
StaticMap<KeyType, ValueType, Capacity>::StaticMap()
{
    KeyType key{ KeyType::OFFSET };
    for (auto & node : mData) {
        node.key = key++;
    }
}

//==============================================================================
template<typename KeyType, typename ValueType, size_t Capacity>
bool StaticMap<KeyType, ValueType, Capacity>::isEmpty() const
{
    return mUsed.none();
}

//==============================================================================
template<typename KeyType, typename ValueType, size_t Capacity>
size_t StaticMap<KeyType, ValueType, Capacity>::size() const
{
    return mUsed.count();
}

//==============================================================================
template<typename KeyType, typename ValueType, size_t Capacity>
void StaticMap<KeyType, ValueType, Capacity>::clear()
{
    mUsed.reset();
}

//==============================================================================
template<typename KeyType, typename ValueType, size_t Capacity>
void StaticMap<KeyType, ValueType, Capacity>::add(KeyType const key, ValueType const & value)
{
    jassert(std::is_trivially_copy_assignable_v<ValueType>);

    auto const index{ toIndex(key) };
    jassert(!mUsed.test(index));
    auto & node{ mData[index] };
    jassert(node.key == key);
    node.value = value;
    mUsed.set(index);
}

//==============================================================================
template<typename KeyType, typename ValueType, size_t Capacity>
void StaticMap<KeyType, ValueType, Capacity>::add(KeyType const key)
{
    auto const index{ toIndex(key) };
    jassert(!mUsed.test(index));
    jassert(mData[index].key == key);
    mUsed.set(index);
}

//==============================================================================
template<typename KeyType, typename ValueType, size_t Capacity>
void StaticMap<KeyType, ValueType, Capacity>::remove(KeyType const key)
{
    auto const index{ toIndex(key) };
    jassert(mUsed.test(index));
    mUsed.set(index, false);
}

//==============================================================================
template<typename KeyType, typename ValueType, size_t Capacity>
void StaticMap<KeyType, ValueType, Capacity>::fill(KeyType firstKey, ValueType const & value)
{
    for (auto & node : mData) {
        node.key = firstKey++;
        node.value = value;
    }
    mUsed.set();
}

//==============================================================================
template<typename KeyType, typename ValueType, size_t Capacity>
juce::Array<KeyType> StaticMap<KeyType, ValueType, Capacity>::getKeys() const noexcept
{
    JUCE_ASSERT_MESSAGE_THREAD;
    juce::Array<KeyType> result{};
    result.ensureStorageAllocated(narrow<int>(size()));
    for (auto const node : *this) {
        result.add(node.key);
    }
    return result;
}

//==============================================================================
template<typename KeyType, typename ValueType, size_t Capacity>
bool StaticMap<KeyType, ValueType, Capacity>::contains(KeyType const key) const noexcept
{
    auto const index{ toIndex(key) };
    return mUsed.test(index);
}

//==============================================================================
template<typename KeyType, typename ValueType, size_t Capacity>
bool StaticMap<KeyType, ValueType, Capacity>::hasSameKeys(StaticMap const & other) const noexcept
{
    return mUsed == other.mUsed;
}

//==============================================================================
template<typename KeyType, typename ValueType, size_t Capacity>
ValueType & StaticMap<KeyType, ValueType, Capacity>::operator[](KeyType const key)
{
    auto const index{ toIndex(key) };
    jassert(mUsed.test(index));
    return mData[index].value;
}

//==============================================================================
template<typename KeyType, typename ValueType, size_t Capacity>
ValueType const & StaticMap<KeyType, ValueType, Capacity>::operator[](KeyType const key) const
{
    auto const index{ toIndex(key) };
    jassert(mUsed.test(index));
    return mData[index].value;
}

//==============================================================================
template<typename KeyType, typename ValueType, size_t Capacity>
typename StaticMap<KeyType, ValueType, Capacity>::iterator StaticMap<KeyType, ValueType, Capacity>::begin()
{
    return iterator{ *this, 0 };
}

//==============================================================================
template<typename KeyType, typename ValueType, size_t Capacity>
typename StaticMap<KeyType, ValueType, Capacity>::iterator StaticMap<KeyType, ValueType, Capacity>::end()
{
    return iterator{ *this, CAPACITY };
}

//==============================================================================
template<typename KeyType, typename ValueType, size_t Capacity>
typename StaticMap<KeyType, ValueType, Capacity>::const_iterator StaticMap<KeyType, ValueType, Capacity>::begin() const
{
    return const_iterator{ *this, 0 };
}

//==============================================================================
template<typename KeyType, typename ValueType, size_t Capacity>
typename StaticMap<KeyType, ValueType, Capacity>::const_iterator StaticMap<KeyType, ValueType, Capacity>::end() const
{
    return const_iterator{ *this, CAPACITY };
}

//==============================================================================
template<typename KeyType, typename ValueType, size_t Capacity>
typename StaticMap<KeyType, ValueType, Capacity>::const_iterator StaticMap<KeyType, ValueType, Capacity>::cbegin() const
{
    return const_iterator{ *this, 0 };
}

//==============================================================================
template<typename KeyType, typename ValueType, size_t Capacity>
typename StaticMap<KeyType, ValueType, Capacity>::const_iterator StaticMap<KeyType, ValueType, Capacity>::cend() const
{
    return const_iterator{ *this, CAPACITY };
}

//==============================================================================
template<typename KeyType, typename ValueType, size_t Capacity>
size_t StaticMap<KeyType, ValueType, Capacity>::toIndex(KeyType const key) const
{
    auto const index{ narrow<size_t>(key.get() - KeyType::OFFSET) };
    jassert(index < CAPACITY);
    return index;
}

//==============================================================================
template<typename KeyType, typename ValueType, size_t Capacity>
typename StaticMap<KeyType, ValueType, Capacity>::Node &
    StaticMap<KeyType, ValueType, Capacity>::operator[](size_t const index)
{
    jassert(index < CAPACITY);
    return mData[index];
}

//==============================================================================
template<typename KeyType, typename ValueType, size_t Capacity>
typename StaticMap<KeyType, ValueType, Capacity>::Node const &
    StaticMap<KeyType, ValueType, Capacity>::operator[](size_t const index) const
{
    jassert(index < CAPACITY);
    return mData[index];
}

} // namespace gris
