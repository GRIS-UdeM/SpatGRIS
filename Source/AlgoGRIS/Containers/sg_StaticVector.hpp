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

#include "../Data/sg_Narrow.hpp"
#include <JuceHeader.h>
#include <array>
#include <type_traits>

namespace gris
{
//==============================================================================
/** A stack-allocated fixed-capacity vector of objects.
 *
 * Values have to be trivial since the destructor is sometimes omitted.
 */
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
    [[nodiscard]] T & operator[](int index);
    [[nodiscard]] T const & operator[](int index) const;
    [[nodiscard]] T & operator[](size_t index);
    [[nodiscard]] T const & operator[](size_t index) const;
    //==============================================================================
    void push_back(T const & value);
    void push_back(T && value);
    [[nodiscard]] T pop_back() noexcept;
    //==============================================================================
    void clear();
    void resize(size_t newSize);
    //==============================================================================
    [[nodiscard]] T * data() noexcept;
    [[nodiscard]] T const * data() const noexcept;
    //==============================================================================
    [[nodiscard]] bool isEmpty() const;
    [[nodiscard]] bool isFull() const;
    [[nodiscard]] size_t size() const;
    //==============================================================================
    [[nodiscard]] iterator begin();
    [[nodiscard]] iterator end();
    [[nodiscard]] const_iterator begin() const;
    [[nodiscard]] const_iterator end() const;
    [[nodiscard]] const_iterator cbegin() const;
    [[nodiscard]] const_iterator cend() const;
    //==============================================================================
    T & front();
    T const & front() const;
    //==============================================================================
    T & back();
    T const & back() const;
    //==============================================================================
    void erase(const_iterator const & it);

private:
    //==============================================================================
    JUCE_LEAK_DETECTOR(StaticVector)
};

//==============================================================================
template<typename T, size_t CAPACITY>
T & StaticVector<T, CAPACITY>::operator[](int const index)
{
    auto const index_u{ narrow<size_t>(index) };
    jassert(index_u < mSize);
    return mData[index_u];
}

//==============================================================================
template<typename T, size_t CAPACITY>
T const & StaticVector<T, CAPACITY>::operator[](int const index) const
{
    auto const index_u{ narrow<size_t>(index) };
    jassert(index_u < mSize);
    return mData[index_u];
}

//==============================================================================
template<typename T, size_t CAPACITY>
T & StaticVector<T, CAPACITY>::operator[](size_t const index)
{
    jassert(index < mSize);
    return mData[index];
}

//==============================================================================
template<typename T, size_t CAPACITY>
T const & StaticVector<T, CAPACITY>::operator[](size_t const index) const
{
    jassert(index < mSize);
    return mData[index];
}

//==============================================================================
template<typename T, size_t CAPACITY>
void StaticVector<T, CAPACITY>::push_back(T const & value)
{
    jassert(!isFull());
    mData[mSize++] = value;
}

//==============================================================================
template<typename T, size_t CAPACITY>
void StaticVector<T, CAPACITY>::push_back(T && value)
{
    jassert(!isFull());
    mData[mSize++] = std::forward<T>(value);
}

//==============================================================================
template<typename T, size_t CAPACITY>
T StaticVector<T, CAPACITY>::pop_back() noexcept
{
    jassert(!isEmpty());
    return std::move(mData[--mSize]);
}

//==============================================================================
template<typename T, size_t CAPACITY>
void StaticVector<T, CAPACITY>::clear()
{
    mSize = 0;
}

//==============================================================================
template<typename T, size_t CAPACITY>
void StaticVector<T, CAPACITY>::resize(size_t const newSize)
{
    jassert(newSize <= CAPACITY);
    mSize = newSize;
}

//==============================================================================
template<typename T, size_t CAPACITY>
T * StaticVector<T, CAPACITY>::data() noexcept
{
    return mData.data();
}

//==============================================================================
template<typename T, size_t CAPACITY>
T const * StaticVector<T, CAPACITY>::data() const noexcept
{
    return mData.data();
}

//==============================================================================
template<typename T, size_t CAPACITY>
bool StaticVector<T, CAPACITY>::isEmpty() const
{
    return mSize == 0;
}

//==============================================================================
template<typename T, size_t CAPACITY>
bool StaticVector<T, CAPACITY>::isFull() const
{
    return mSize == CAPACITY;
}

//==============================================================================
template<typename T, size_t CAPACITY>
size_t StaticVector<T, CAPACITY>::size() const
{
    return mSize;
}

//==============================================================================
template<typename T, size_t CAPACITY>
typename StaticVector<T, CAPACITY>::iterator StaticVector<T, CAPACITY>::begin()
{
    return mData.begin();
}

//==============================================================================
template<typename T, size_t CAPACITY>
typename StaticVector<T, CAPACITY>::iterator StaticVector<T, CAPACITY>::end()
{
    return mData.begin() + mSize;
}

//==============================================================================
template<typename T, size_t CAPACITY>
typename StaticVector<T, CAPACITY>::const_iterator StaticVector<T, CAPACITY>::begin() const
{
    return mData.cbegin();
}

//==============================================================================
template<typename T, size_t CAPACITY>
typename StaticVector<T, CAPACITY>::const_iterator StaticVector<T, CAPACITY>::end() const
{
    return mData.cbegin() + mSize;
}

//==============================================================================
template<typename T, size_t CAPACITY>
typename StaticVector<T, CAPACITY>::const_iterator StaticVector<T, CAPACITY>::cbegin() const
{
    return mData.cbegin();
}

//==============================================================================
template<typename T, size_t CAPACITY>
typename StaticVector<T, CAPACITY>::const_iterator StaticVector<T, CAPACITY>::cend() const
{
    return mData.cbegin() + mSize;
}

//==============================================================================
template<typename T, size_t CAPACITY>
T & StaticVector<T, CAPACITY>::front()
{
    jassert(!isEmpty());
    return mData.front();
}

//==============================================================================
template<typename T, size_t CAPACITY>
T const & StaticVector<T, CAPACITY>::front() const
{
    jassert(!isEmpty());
    return mData.front();
}

//==============================================================================
template<typename T, size_t CAPACITY>
T & StaticVector<T, CAPACITY>::back()
{
    jassert(!isEmpty());
    return mData[mSize - 1];
}

//==============================================================================
template<typename T, size_t CAPACITY>
T const & StaticVector<T, CAPACITY>::back() const
{
    jassert(!isEmpty());
    return mData[mSize - 1];
}

//==============================================================================
template<typename T, size_t CAPACITY>
void StaticVector<T, CAPACITY>::erase(const_iterator const & it)
{
    jassert(it != cend());
    jassert(!isEmpty());
    *it = back();
    --mSize;
}

} // namespace gris
