/*
  ==============================================================================

    StrongTypes.hpp
    Created: 25 Feb 2021 6:32:48pm
    Author:  samue

  ==============================================================================
*/

#pragma once

#include <cstdint>

template<typename T, typename Dummy>
class StrongIndex
{
    T mValue;

public:
    using type = T;

    StrongIndex() = default;
    explicit constexpr StrongIndex(T const & value) : mValue(value) {}

    [[nodiscard]] constexpr bool operator==(StrongIndex const & other) const { return mValue == other.mValue; }
    [[nodiscard]] constexpr bool operator!=(StrongIndex const & other) const { return mValue != other.mValue; }
    [[nodiscard]] constexpr bool operator<(StrongIndex const & other) const { return mValue < other.mValue; }
    [[nodiscard]] constexpr bool operator>(StrongIndex const & other) const { return mValue > other.mValue; }
    [[nodiscard]] constexpr bool operator<=(StrongIndex const & other) const { return mValue <= other.mValue; }
    [[nodiscard]] constexpr bool operator>=(StrongIndex const & other) const { return mValue >= other.mValue; }

    [[nodiscard]] constexpr T const & get() const { return mValue; }
    StrongIndex & operator++()
    {
        ++mValue;
        return *this;
    }
    StrongIndex operator++(int) { return StrongIndex{ mValue++ }; }
    StrongIndex & operator--()
    {
        --mValue;
        return *this;
    }
};
using speaker_index_t = StrongIndex<int, struct SpeakerIndexT>;
using speaker_id_t = StrongIndex<int, struct SpeakerIdT>;
using port_id_t = StrongIndex<uint32_t, struct PortIdT>;
using output_patch_t = StrongIndex<int, struct OutputPatchT>;