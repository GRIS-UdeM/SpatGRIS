#pragma once

#include <array>
#include <type_traits>

#include "macros.h"
DISABLE_WARNINGS
#include <JuceHeader.h>
ENABLE_WARNINGS

template<typename T, size_t CAPACITY>
class StaticVector
{
    static_assert(std::is_trivially_default_constructible_v<T>);
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
    void clear() { mSize = 0; }
    void resize(size_t const newSize)
    {
        jassert(newSize <= CAPACITY);
        mSize = newSize;
    }
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