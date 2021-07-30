/*
 This file is part of SpatGRIS.

 Developers: Samuel Béland, Olivier Bélanger, Nicolas Masson

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

#include "StaticVector.hpp"

//==============================================================================
template<typename T>
class AtomicExchanger
{
    static constexpr size_t CAPACITY = 6;

public:
    //==============================================================================
    class Ticket
    {
        friend AtomicExchanger;

        T mValue{};
        std::atomic<bool> mIsFree{ true };

    public:
        [[nodiscard]] T & get() noexcept { return mValue; }
        [[nodiscard]] T const & get() const noexcept { return mValue; }
    };
    static_assert(std::atomic<Ticket *>::is_always_lock_free);
    static_assert(std::atomic<bool>::is_always_lock_free);

private:
    //==============================================================================
    std::array<Ticket, CAPACITY> mData{};
    std::atomic<Ticket *> mMostRecent{};

public:
    //==============================================================================
    AtomicExchanger() = default;
    ~AtomicExchanger() = default;
    //==============================================================================
    AtomicExchanger(AtomicExchanger const &) = delete;
    AtomicExchanger(AtomicExchanger &&) = delete;
    AtomicExchanger & operator=(AtomicExchanger const &) = delete;
    AtomicExchanger & operator=(AtomicExchanger &&) = delete;
    //==============================================================================
    Ticket * acquire() noexcept
    {
        auto expected{ true };
        for (auto & ticket : mData) {
            ticket.mIsFree.compare_exchange_strong(expected, false);
            if (expected) {
                return &ticket;
            }
            expected = true;
        }
        jassertfalse;
        return nullptr;
    }
    //==============================================================================
    void getMostRecent(Ticket *& ticketToUpdate) noexcept
    {
        auto * mostRecent{ mMostRecent.exchange(nullptr) };
        if (mostRecent == nullptr) {
            return;
        }
        jassert(mostRecent != ticketToUpdate);
        jassert(!mostRecent->mIsFree.load());
        if (ticketToUpdate) {
            jassert(!ticketToUpdate->mIsFree.load());
            ticketToUpdate->mIsFree.store(true);
        }
        ticketToUpdate = mostRecent;
    }
    //==============================================================================
    void setMostRecent(Ticket * newMostRecent) noexcept
    {
        jassert(newMostRecent);
        jassert(!newMostRecent->mIsFree.load());
        auto * oldMostRecent{ mMostRecent.exchange(newMostRecent) };
        jassert(newMostRecent != oldMostRecent);
        if (oldMostRecent == nullptr) {
            return;
        }
        jassert(!oldMostRecent->mIsFree.load());
        if (oldMostRecent) {
            oldMostRecent->mIsFree.store(true);
        }
    }
};