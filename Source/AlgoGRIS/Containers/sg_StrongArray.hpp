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

namespace gris
{
//==============================================================================
/** A stack-allocated fixed-capacity vector of objects accessed using a strongly-typed index.
 *
 * Values have to be trivial since the destructor is sometimes omitted.
 */
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
    [[nodiscard]] ValueType * data() noexcept { return mData.data(); }
    [[nodiscard]] ValueType const * data() const noexcept { return mData.data(); }
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

} // namespace gris
