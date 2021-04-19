#pragma once

#include "StrongTypes.hpp"

//==============================================================================
template<typename KeyType, typename ValueType, size_t CAPACITY>
class StrongArray
{
    static_assert(std::is_base_of_v<StrongIndexBase, KeyType>);
#ifdef NDEBUG
    static_assert(std::is_trivially_destructible_v<ValueType>);
#endif

    using container_type = std::array<ValueType, CAPACITY>;
    using iterator = typename container_type::iterator;
    using const_iterator = typename container_type::const_iterator;
    //==============================================================================
    container_type mData{};

public:
    //==============================================================================
    [[nodiscard]] ValueType & operator[](KeyType const & key) noexcept { return mData[getIndex(key)]; }
    [[nodiscard]] ValueType const & operator[](KeyType const & key) const noexcept { return mData[getIndex(key)]; }
    //==============================================================================
    [[nodiscard]] iterator begin() noexcept { return mData.begin(); }
    [[nodiscard]] iterator end() noexcept { return mData.end(); }
    [[nodiscard]] const_iterator begin() const noexcept { return mData.cbegin(); }
    [[nodiscard]] const_iterator end() const noexcept { return mData.cend(); }
    [[nodiscard]] const_iterator cbegin() const noexcept { return mData.cbegin(); }
    [[nodiscard]] const_iterator cend() const noexcept { return mData.cend(); }

private:
    //==============================================================================
    static size_t getIndex(KeyType const & key) noexcept
    {
        auto const index{ narrow<size_t>(key.get() - KeyType::OFFSET) };
        jassert(index < CAPACITY);
        return index;
    }
};