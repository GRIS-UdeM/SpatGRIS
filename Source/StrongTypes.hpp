/*
  ==============================================================================

    StrongTypes.hpp
    Created: 25 Feb 2021 6:32:48pm
    Author:  samue

  ==============================================================================
*/

#pragma once

template <typename T, typename Dummy>
class StrongIndex
{
    T mValue;
public:
    using type = T;

    [[nodiscard]] bool operator==(StrongIndex const& other) const { return mValue == other.mValue; }
    [[nodiscard]] bool operator!=(StrongIndex const& other) const { return mValue != other.mValue; }
    [[nodiscard]] bool operator<(StrongIndex const& other) const { return mValue < other.mValue; }
    [[nodiscard]] bool operator > (StrongIndex const& other) const { return mValue > other.mValue; }
    [[nodiscard]] bool operator<=(StrongIndex const& other) const { return mValue <= other.mValue; }
    [[nodiscard]] bool operator>=(StrongIndex const& other) const { return mValue >= other.mValue; }

    [[nodiscard]] T const& get() const { return mValue; }
    StrongIndex& operator++() { ++mValue; return *this; }
};
using speaker_index_t = StrongIndex<int, struct SpeakerIndexT>;
using speaker_id_t = StrongIndex<int, struct SpeakerIdT>;