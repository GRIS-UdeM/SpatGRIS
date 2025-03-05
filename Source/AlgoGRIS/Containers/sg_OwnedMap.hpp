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

#include "../Data/sg_Macros.hpp"
#include "../Data/StrongTypes/sg_StrongIndex.hpp"

#include <bitset>

namespace gris
{
//==============================================================================
/** A heap-allocated fixed-capacity associative map of objects accessed using a strongly-typed index.
 */
template<typename KeyType, typename ValueType, size_t CAPACITY>
class OwnedMap
{
    static_assert(std::is_base_of_v<StrongIndexBase, KeyType>);

public:
    //==============================================================================
    struct Node {
        KeyType key{};
        ValueType * value{};
    };
    //==============================================================================
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
    OwnedMap();
    ~OwnedMap() noexcept;
    SG_DELETE_COPY(OwnedMap)
    OwnedMap(OwnedMap && other) noexcept;
    OwnedMap & operator=(OwnedMap && other) noexcept;
    //==============================================================================
    void clear() noexcept;
    void remove(KeyType key) noexcept;
    ValueType & add(KeyType key, std::unique_ptr<ValueType> value) noexcept;
    [[nodiscard]] bool operator==(OwnedMap const & other) const;
    [[nodiscard]] int size() const noexcept;
    [[nodiscard]] ValueType & operator[](KeyType key);
    [[nodiscard]] ValueType const & operator[](KeyType key) const;
    [[nodiscard]] Node & getNode(KeyType const & key) noexcept;
    [[nodiscard]] ConstNode const & getConstNode(KeyType const & key) const noexcept;
    [[nodiscard]] size_t toIndex(KeyType const & key) const noexcept;
    [[nodiscard]] bool contains(KeyType const & key) const noexcept;
    [[nodiscard]] KeyType getNextUsedKey(KeyType const & key) const noexcept;
    [[nodiscard]] KeyType getFirstUsedKey() const noexcept;
    //==============================================================================
    class iterator
    {
        OwnedMap * mOwnedMap{};
        KeyType mCurrentKey{};

    public:
        //==============================================================================
        iterator() = default;
        iterator(OwnedMap & map, KeyType key);
        ~iterator() = default;
        SG_DEFAULT_COPY_AND_MOVE(iterator)
        //==============================================================================
        using iterator_category = std::forward_iterator_tag;
        using difference_type = std::ptrdiff_t;
        using value_type = Node;
        using pointer = Node *;
        using reference = Node &;
        //==============================================================================
        [[nodiscard]] bool operator==(iterator const & other) const noexcept;
        [[nodiscard]] bool operator!=(iterator const & other) const noexcept;
        [[nodiscard]] bool operator<(iterator const & other) const noexcept;
        //==============================================================================
        [[nodiscard]] reference operator*();
        [[nodiscard]] reference operator*() const;
        [[nodiscard]] pointer operator->();
        [[nodiscard]] pointer operator->() const;
        //==============================================================================
        iterator & operator++() noexcept;
        [[nodiscard]] iterator operator++(int) noexcept;
    };

    //==============================================================================
    class const_iterator
    {
        OwnedMap const * mOwnedMap{};
        KeyType mCurrentKey{};

    public:
        //==============================================================================
        const_iterator() = default;
        const_iterator(OwnedMap const & map, KeyType key);
        ~const_iterator() = default;
        SG_DEFAULT_COPY_AND_MOVE(const_iterator)
        //==============================================================================
        using iterator_category = std::forward_iterator_tag;
        using difference_type = std::ptrdiff_t;
        using value_type = ConstNode;
        using pointer = ConstNode const *;
        using reference = ConstNode const &;
        //==============================================================================
        [[nodiscard]] bool operator==(const_iterator const & other) const noexcept;
        [[nodiscard]] bool operator!=(const_iterator const & other) const noexcept;
        [[nodiscard]] bool operator<(const_iterator const & other) const noexcept;
        //==============================================================================
        [[nodiscard]] reference operator*();
        [[nodiscard]] reference operator*() const;
        [[nodiscard]] pointer operator->();
        [[nodiscard]] pointer operator->() const;
        //==============================================================================
        const_iterator & operator++() noexcept;
        [[nodiscard]] const_iterator operator++(int) noexcept;
    };

    //==============================================================================
    [[nodiscard]] iterator begin() noexcept;
    [[nodiscard]] iterator end() noexcept;
    [[nodiscard]] const_iterator begin() const noexcept;
    [[nodiscard]] const_iterator end() const noexcept;
    [[nodiscard]] const_iterator cbegin() const noexcept;
    [[nodiscard]] const_iterator cend() const noexcept;

private:
    //==============================================================================
    JUCE_LEAK_DETECTOR(OwnedMap)
};

//==============================================================================
template<typename KeyType, typename ValueType, size_t CAPACITY>
OwnedMap<KeyType, ValueType, CAPACITY>::OwnedMap()
{
    KeyType key{ KeyType::OFFSET };
    for (auto & node : mData) {
        node.key = key++;
    }
}

//==============================================================================
template<typename KeyType, typename ValueType, size_t CAPACITY>
OwnedMap<KeyType, ValueType, CAPACITY>::~OwnedMap() noexcept
{
    for (auto & node : *this) {
        delete node.value;
    }
}

//==============================================================================
template<typename KeyType, typename ValueType, size_t CAPACITY>
OwnedMap<KeyType, ValueType, CAPACITY>::OwnedMap(OwnedMap && other) noexcept : mData(other.mData)
                                                                             , mUsed(other.mUsed)
{
    other.mUsed.reset();
}

//==============================================================================
template<typename KeyType, typename ValueType, size_t CAPACITY>
OwnedMap<KeyType, ValueType, CAPACITY> & OwnedMap<KeyType, ValueType, CAPACITY>::operator=(OwnedMap && other) noexcept
{
    clear();
    mData = other.mData;
    mUsed = other.mUsed;
    other.mUsed.reset();
    return *this;
}

//==============================================================================
template<typename KeyType, typename ValueType, size_t CAPACITY>
bool OwnedMap<KeyType, ValueType, CAPACITY>::operator==(OwnedMap const & other) const
{
    if (mUsed != other.mUsed) {
        return false;
    }

    return std::all_of(this->cbegin(), this->cend(), [&](ConstNode const & node) {
        return *node.value == other[node.key];
    });
}

//==============================================================================
template<typename KeyType, typename ValueType, size_t CAPACITY>
void OwnedMap<KeyType, ValueType, CAPACITY>::clear() noexcept
{
    for (auto & node : *this) {
        delete node.value;
    }
    mUsed.reset();
}

//==============================================================================
template<typename KeyType, typename ValueType, size_t CAPACITY>
int OwnedMap<KeyType, ValueType, CAPACITY>::size() const noexcept
{
    return narrow<int>(mUsed.count());
}

//==============================================================================
template<typename KeyType, typename ValueType, size_t CAPACITY>
ValueType & OwnedMap<KeyType, ValueType, CAPACITY>::operator[](KeyType const key)
{
    return *getNode(key).value;
}

//==============================================================================
template<typename KeyType, typename ValueType, size_t CAPACITY>
ValueType const & OwnedMap<KeyType, ValueType, CAPACITY>::operator[](KeyType const key) const
{
    return *getConstNode(key).value;
}

//==============================================================================
template<typename KeyType, typename ValueType, size_t CAPACITY>
typename OwnedMap<KeyType, ValueType, CAPACITY>::Node &
    OwnedMap<KeyType, ValueType, CAPACITY>::getNode(KeyType const & key) noexcept
{
    auto const index{ toIndex(key) };
    jassert(index < CAPACITY);
    jassert(mUsed.test(index));
    return mData[index];
}

//==============================================================================
template<typename KeyType, typename ValueType, size_t CAPACITY>
typename OwnedMap<KeyType, ValueType, CAPACITY>::ConstNode const &
    OwnedMap<KeyType, ValueType, CAPACITY>::getConstNode(KeyType const & key) const noexcept
{
    auto const index{ toIndex(key) };
    jassert(index < CAPACITY);
    jassert(mUsed.test(index));
    return reinterpret_cast<ConstNode const &>(mData[index]);
}

//==============================================================================
template<typename KeyType, typename ValueType, size_t CAPACITY>
size_t OwnedMap<KeyType, ValueType, CAPACITY>::toIndex(KeyType const & key) const noexcept
{
    return narrow<size_t>(key.get() - KeyType::OFFSET);
}

//==============================================================================
template<typename KeyType, typename ValueType, size_t CAPACITY>
bool OwnedMap<KeyType, ValueType, CAPACITY>::contains(KeyType const & key) const noexcept
{
    auto const index{ toIndex(key) };
    jassert(index < CAPACITY);
    return mUsed.test(index);
}

//==============================================================================
template<typename KeyType, typename ValueType, size_t CAPACITY>
KeyType OwnedMap<KeyType, ValueType, CAPACITY>::getNextUsedKey(KeyType const & key) const noexcept
{
    auto index{ toIndex(key) };
    if (index >= CAPACITY) {
        return key;
    }
    do {
        ++index;
    } while (index < CAPACITY && !mUsed.test(index));
    return KeyType{ narrow<typename KeyType::type>(index) + KeyType::OFFSET };
}

//==============================================================================
template<typename KeyType, typename ValueType, size_t CAPACITY>
KeyType OwnedMap<KeyType, ValueType, CAPACITY>::getFirstUsedKey() const noexcept
{
    KeyType const key{ KeyType::OFFSET };
    if (contains(key)) {
        return key;
    }
    return getNextUsedKey(key);
}

//==============================================================================
template<typename KeyType, typename ValueType, size_t CAPACITY>
ValueType & OwnedMap<KeyType, ValueType, CAPACITY>::add(KeyType const key, std::unique_ptr<ValueType> value) noexcept
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
template<typename KeyType, typename ValueType, size_t CAPACITY>
void OwnedMap<KeyType, ValueType, CAPACITY>::remove(KeyType const key) noexcept
{
    auto const index{ toIndex(key) };
    jassert(index < CAPACITY);
    jassert(mUsed.test(index));
    delete mData[index].value;
    mUsed.reset(index);
}

//==============================================================================
template<typename KeyType, typename ValueType, size_t CAPACITY>
OwnedMap<KeyType, ValueType, CAPACITY>::iterator::iterator(OwnedMap & map, KeyType const key)
    : mOwnedMap(&map)
    , mCurrentKey(key)
{
}

//==============================================================================
template<typename KeyType, typename ValueType, size_t CAPACITY>
bool OwnedMap<KeyType, ValueType, CAPACITY>::iterator::operator==(iterator const & other) const noexcept
{
    return mCurrentKey == other.mCurrentKey;
}

//==============================================================================
template<typename KeyType, typename ValueType, size_t CAPACITY>
bool OwnedMap<KeyType, ValueType, CAPACITY>::iterator::operator!=(iterator const & other) const noexcept
{
    return mCurrentKey != other.mCurrentKey;
}

//==============================================================================
template<typename KeyType, typename ValueType, size_t CAPACITY>
bool OwnedMap<KeyType, ValueType, CAPACITY>::iterator::operator<(iterator const & other) const noexcept
{
    return mCurrentKey < other.mCurrentKey;
}

//==============================================================================
template<typename KeyType, typename ValueType, size_t CAPACITY>
typename OwnedMap<KeyType, ValueType, CAPACITY>::iterator::reference
    OwnedMap<KeyType, ValueType, CAPACITY>::iterator::operator*()
{
    return mOwnedMap->getNode(mCurrentKey);
}

//==============================================================================
template<typename KeyType, typename ValueType, size_t CAPACITY>
typename OwnedMap<KeyType, ValueType, CAPACITY>::iterator::reference
    OwnedMap<KeyType, ValueType, CAPACITY>::iterator::operator*() const
{
    return mOwnedMap->getNode(mCurrentKey);
}

//==============================================================================
template<typename KeyType, typename ValueType, size_t CAPACITY>
typename OwnedMap<KeyType, ValueType, CAPACITY>::iterator::pointer
    OwnedMap<KeyType, ValueType, CAPACITY>::iterator::operator->()
{
    return &mOwnedMap->getNode(mCurrentKey);
}

//==============================================================================
template<typename KeyType, typename ValueType, size_t CAPACITY>
typename OwnedMap<KeyType, ValueType, CAPACITY>::iterator::pointer
    OwnedMap<KeyType, ValueType, CAPACITY>::iterator::operator->() const
{
    return &mOwnedMap->getNode(mCurrentKey);
}

//==============================================================================
template<typename KeyType, typename ValueType, size_t CAPACITY>
typename OwnedMap<KeyType, ValueType, CAPACITY>::iterator &
    OwnedMap<KeyType, ValueType, CAPACITY>::iterator::operator++() noexcept
{
    mCurrentKey = mOwnedMap->getNextUsedKey(mCurrentKey);
    return *this;
}

//==============================================================================
template<typename KeyType, typename ValueType, size_t CAPACITY>
typename OwnedMap<KeyType, ValueType, CAPACITY>::iterator
    OwnedMap<KeyType, ValueType, CAPACITY>::iterator::operator++(int) noexcept
{
    auto const temp{ *this };
    ++(*this);
    return temp;
}

//==============================================================================
template<typename KeyType, typename ValueType, size_t CAPACITY>
OwnedMap<KeyType, ValueType, CAPACITY>::const_iterator::const_iterator(OwnedMap const & map, KeyType const key)
    : mOwnedMap(&map)
    , mCurrentKey(key)
{
}

//==============================================================================
template<typename KeyType, typename ValueType, size_t CAPACITY>
bool OwnedMap<KeyType, ValueType, CAPACITY>::const_iterator::operator==(const_iterator const & other) const noexcept
{
    return mCurrentKey == other.mCurrentKey;
}

//==============================================================================
template<typename KeyType, typename ValueType, size_t CAPACITY>
bool OwnedMap<KeyType, ValueType, CAPACITY>::const_iterator::operator!=(const_iterator const & other) const noexcept
{
    return mCurrentKey != other.mCurrentKey;
}

//==============================================================================
template<typename KeyType, typename ValueType, size_t CAPACITY>
bool OwnedMap<KeyType, ValueType, CAPACITY>::const_iterator::operator<(const_iterator const & other) const noexcept
{
    return mCurrentKey < other.mCurrentKey;
}

//==============================================================================
template<typename KeyType, typename ValueType, size_t CAPACITY>
typename OwnedMap<KeyType, ValueType, CAPACITY>::const_iterator::reference
    OwnedMap<KeyType, ValueType, CAPACITY>::const_iterator::operator*()
{
    return mOwnedMap->getConstNode(mCurrentKey);
}

//==============================================================================
template<typename KeyType, typename ValueType, size_t CAPACITY>
typename OwnedMap<KeyType, ValueType, CAPACITY>::const_iterator::reference
    OwnedMap<KeyType, ValueType, CAPACITY>::const_iterator::operator*() const
{
    return mOwnedMap->getConstNode(mCurrentKey);
}

//==============================================================================
template<typename KeyType, typename ValueType, size_t CAPACITY>
typename OwnedMap<KeyType, ValueType, CAPACITY>::const_iterator::pointer
    OwnedMap<KeyType, ValueType, CAPACITY>::const_iterator::operator->()
{
    return &mOwnedMap->getConstNode(mCurrentKey);
}

//==============================================================================
template<typename KeyType, typename ValueType, size_t CAPACITY>
typename OwnedMap<KeyType, ValueType, CAPACITY>::const_iterator::pointer
    OwnedMap<KeyType, ValueType, CAPACITY>::const_iterator::operator->() const
{
    return &mOwnedMap->getConstNode(mCurrentKey);
}

//==============================================================================
template<typename KeyType, typename ValueType, size_t CAPACITY>
typename OwnedMap<KeyType, ValueType, CAPACITY>::const_iterator &
    OwnedMap<KeyType, ValueType, CAPACITY>::const_iterator ::operator++() noexcept
{
    mCurrentKey = mOwnedMap->getNextUsedKey(mCurrentKey);
    return *this;
}

//==============================================================================
template<typename KeyType, typename ValueType, size_t CAPACITY>
typename OwnedMap<KeyType, ValueType, CAPACITY>::const_iterator
    OwnedMap<KeyType, ValueType, CAPACITY>::const_iterator::operator++(int) noexcept
{
    auto const temp{ *this };
    ++(*this);
    return temp;
}

//==============================================================================
template<typename KeyType, typename ValueType, size_t CAPACITY>
typename OwnedMap<KeyType, ValueType, CAPACITY>::iterator OwnedMap<KeyType, ValueType, CAPACITY>::begin() noexcept
{
    return iterator{ *this, getFirstUsedKey() };
}

//==============================================================================
template<typename KeyType, typename ValueType, size_t CAPACITY>
typename OwnedMap<KeyType, ValueType, CAPACITY>::iterator OwnedMap<KeyType, ValueType, CAPACITY>::end() noexcept
{
    return iterator{ *this, KeyType{ narrow<typename KeyType::type>(CAPACITY) + KeyType::OFFSET } };
}

//==============================================================================
template<typename KeyType, typename ValueType, size_t CAPACITY>
typename OwnedMap<KeyType, ValueType, CAPACITY>::const_iterator
    OwnedMap<KeyType, ValueType, CAPACITY>::begin() const noexcept
{
    return const_iterator{ *this, getFirstUsedKey() };
}

//==============================================================================
template<typename KeyType, typename ValueType, size_t CAPACITY>
typename OwnedMap<KeyType, ValueType, CAPACITY>::const_iterator
    OwnedMap<KeyType, ValueType, CAPACITY>::end() const noexcept
{
    return const_iterator{ *this, KeyType{ narrow<typename KeyType::type>(CAPACITY) + KeyType::OFFSET } };
}

//==============================================================================
template<typename KeyType, typename ValueType, size_t CAPACITY>
typename OwnedMap<KeyType, ValueType, CAPACITY>::const_iterator
    OwnedMap<KeyType, ValueType, CAPACITY>::cbegin() const noexcept
{
    return const_iterator{ *this, getFirstUsedKey() };
}

//==============================================================================
template<typename KeyType, typename ValueType, size_t CAPACITY>
typename OwnedMap<KeyType, ValueType, CAPACITY>::const_iterator
    OwnedMap<KeyType, ValueType, CAPACITY>::cend() const noexcept
{
    return const_iterator{ *this, KeyType{ narrow<typename KeyType::type>(CAPACITY) + KeyType::OFFSET } };
}

} // namespace gris
