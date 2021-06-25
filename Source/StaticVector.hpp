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

#include "Narrow.hpp"

#include <JuceHeader.h>

#include <array>
#include <type_traits>

//==============================================================================
template<typename T, size_t CAPACITY>
class StaticVector
{
    static_assert(std::is_default_constructible_v<T>);
    static_assert(std::is_trivially_destructible_v<T>);
    using container_t = std::array<T, CAPACITY>;
    //==============================================================================
    container_t mData;
    size_t mSize{};

public:
    //==============================================================================
    using iterator = typename container_t::iterator;
    using const_iterator = typename container_t::const_iterator;
    //==============================================================================
    T & operator[](int const index)
    {
        auto const index_u{ narrow<size_t>(index) };
        jassert(index_u < mSize);
        return mData[index_u];
    }
    T const & operator[](int const index) const
    {
        auto const index_u{ narrow<size_t>(index) };
        jassert(index_u < mSize);
        return mData[index_u];
    }
    //==============================================================================
    T & operator[](size_t const index)
    {
        jassert(index < mSize);
        return mData[index];
    }
    T const & operator[](size_t const index) const
    {
        jassert(index < mSize);
        return mData[index];
    }
    //==============================================================================
    void push_back(T const & value)
    {
        jassert(!isFull());
        mData[mSize++] = value;
    }
    void push_back(T && value)
    {
        jassert(!isFull());
        mData[mSize++] = std::move(value);
    }
    T pop_back() noexcept
    {
        jassert(!isEmpty());
        return std::move(mData[--mSize]);
    }
    //==============================================================================
    void clear() { mSize = 0; }
    //==============================================================================
    void resize(size_t const newSize)
    {
        jassert(newSize <= CAPACITY);
        mSize = newSize;
    }
    //==============================================================================
    T * data() { return mData.data(); }
    //==============================================================================
    [[nodiscard]] bool isEmpty() const { return mSize == 0; }
    [[nodiscard]] bool isFull() const { return mSize == CAPACITY; }
    [[nodiscard]] size_t size() const { return mSize; }
    //==============================================================================
    [[nodiscard]] iterator begin() { return mData.begin(); }
    [[nodiscard]] iterator end() { return mData.begin() + mSize; }
    [[nodiscard]] const_iterator begin() const { return mData.cbegin(); }
    [[nodiscard]] const_iterator end() const { return mData.cbegin() + mSize; }
    [[nodiscard]] const_iterator cbegin() const { return mData.cbegin(); }
    [[nodiscard]] const_iterator cend() const { return mData.cbegin() + mSize; }
    //==============================================================================
    T & front()
    {
        jassert(!isEmpty());
        return mData.front();
    }
    T const & front() const
    {
        jassert(!isEmpty());
        return mData.front();
    }
    //==============================================================================
    T & back()
    {
        jassert(!isEmpty());
        return mData[mSize - 1];
    }
    T const & back() const
    {
        jassert(!isEmpty());
        return mData[mSize - 1];
    }
    //==============================================================================
    void erase(const_iterator const & it)
    {
        jassert(it != cend());
        jassert(!isEmpty());
        *it = back();
        --mSize;
    }
};
