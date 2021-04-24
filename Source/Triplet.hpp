#pragma once

#include "StrongTypes.hpp"

struct Triplet {
    output_patch_t id1{};
    output_patch_t id2{};
    output_patch_t id3{};

    [[nodiscard]] constexpr bool contains(output_patch_t const outputPatch) const noexcept
    {
        return id1 == outputPatch || id2 == outputPatch || id3 == outputPatch;
    }

    [[nodiscard]] constexpr bool isSameAs(Triplet const & other) const noexcept
    {
        return contains(other.id1) && contains(other.id2) && contains(other.id3);
    }
};