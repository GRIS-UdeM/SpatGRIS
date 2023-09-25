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

#include "sg_Macros.hpp"

#include <utility>

namespace gris
{
//==============================================================================
// call a function when the object goes out of scope
template<typename Func>
class ScopeGuard
{
    Func mDestructor;
    bool mDestroyed{ false };

public:
    //==============================================================================
    explicit ScopeGuard(Func destructor) : mDestructor(std::move(destructor)) {}
    ~ScopeGuard()
    {
        if (!mDestroyed) {
            mDestructor();
        }
    }
    //==============================================================================
    ScopeGuard(ScopeGuard && other) noexcept : mDestructor(std::move(other.mDestructor)) { other.mDestroyed = true; }
    ScopeGuard & operator=(ScopeGuard &&) = delete;
    SG_DELETE_COPY(ScopeGuard)
};

//==============================================================================
template<typename Func>
ScopeGuard<Func> make_scope_guard(Func func)
{
    return ScopeGuard<Func>(std::move(func));
}

} // namespace gris
