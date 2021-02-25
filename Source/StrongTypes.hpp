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
    explicit StrongIndex(T const & value) : mValue(value) {}

    [[nodiscard]] bool operator==(StrongIndex const & other) const { return mValue == other.mValue; }
    [[nodiscard]] bool operator!=(StrongIndex const & other) const { return mValue != other.mValue; }
    [[nodiscard]] bool operator<(StrongIndex const & other) const { return mValue < other.mValue; }
    [[nodiscard]] bool operator>(StrongIndex const & other) const { return mValue > other.mValue; }
    [[nodiscard]] bool operator<=(StrongIndex const & other) const { return mValue <= other.mValue; }
    [[nodiscard]] bool operator>=(StrongIndex const & other) const { return mValue >= other.mValue; }

    [[nodiscard]] T const & get() const { return mValue; }
    StrongIndex & operator++()
    {
        ++mValue;
        return *this;
    }
};
using speaker_index_t = StrongIndex<int, struct SpeakerIndexT>;
using speaker_id_t = StrongIndex<int, struct SpeakerIdT>;
using port_id_t = StrongIndex<uint32_t, struct PortIdT>;