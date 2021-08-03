/*
 This file is part of SpatGRIS

 Developers: Samuel Béland, Olivier Bélanger, Nicolas Masso

 SpatGRIS is free software: you can redistribute it and/or modif
 it under the terms of the GNU General Public License as published b
 the Free Software Foundation, either version 3 of the License, o
 (at your option) any later version

 SpatGRIS is distributed in the hope that it will be useful
 but WITHOUT ANY WARRANTY; without even the implied warranty o
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See th
 GNU General Public License for more details

 You should have received a copy of the GNU General Public Licens
 along with SpatGRIS.  If not, see <http://www.gnu.org/licenses/>
*/

#pragma once

#include <utility>

//==============================================================================
// call a function when the instance exits its scope (when it is destroyed)
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
    ScopeGuard(ScopeGuard const &) = delete;
    ScopeGuard(ScopeGuard && other) noexcept : mDestructor(std::move(other.mDestructor)) { other.mDestroyed = true; }
    ScopeGuard & operator=(ScopeGuard const &) = delete;
    ScopeGuard & operator=(ScopeGuard &&) = delete;
};

//==============================================================================
template<typename Func>
ScopeGuard<Func> make_scope_guard(Func func)
{
    return ScopeGuard<Func>(std::move(func));
}
