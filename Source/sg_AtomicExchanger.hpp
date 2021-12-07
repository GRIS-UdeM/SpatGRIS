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

#include "sg_StaticVector.hpp"

//==============================================================================
/** This class implements a single-writer/single-reader thread-safe lockless communication interface.
 *
 * The writer thread uses acquire() to get an exchange token. This token can be used to retrieve dedicated data that can
 * be written to. When the writing is done, the writer thread calls setMostRecent(token) in order to update the
 * exchanger.
 *
 * The reader thread uses getMostRecent(token) to update a local pointer to the most recently pushed token.
 *
 * This class could be updated to a multiple-writers/single-reader with very little effort, but I'm not sure that would
 * be very useful...
 */
template<typename T>
class AtomicExchanger
{
    // The number of tokens to use. While the real safe capacity is probably 4 or 5, we set this to 6 just to be extra
    // safe.
    static constexpr size_t CAPACITY = 6;

public:
    //==============================================================================
    class Token
    {
        friend AtomicExchanger;

        // The actual data
        T mValue{};
        std::atomic<bool> mIsFree{ true };

    public:
        [[nodiscard]] T & get() noexcept { return mValue; }
        [[nodiscard]] T const & get() const noexcept { return mValue; }
    };
    static_assert(std::atomic<Token *>::is_always_lock_free);
    static_assert(std::atomic<bool>::is_always_lock_free);

private:
    //==============================================================================
    std::array<Token, CAPACITY> mData{};
    std::atomic<Token *> mMostRecent{};

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
    /** @returns a pointer to a token that can be written to. Use setMostRecent() to signal the exchanger when the write
     * is done. */
    Token * acquire() noexcept;
    /** Updates a local pointer to the most recently edited token.
     *
     * @param[out] tokenToUpdate the token address that should be updated (or not). Can be nullptr ONCE. A reader MUST
     * keep this pointer intact between calls and ALWAYS re-use its value for its next call so that the exchanger knows
     * when to return a token into its pool.
     */
    void getMostRecent(Token *& tokenToUpdate) noexcept;
    /** Sets a token as the most recent.
     *
     * @param newMostRecent the address of a token acquired with acquire() that should replace the reader's token ASAP.
     */
    void setMostRecent(Token * newMostRecent) noexcept;

private:
    //==============================================================================
    JUCE_LEAK_DETECTOR(AtomicExchanger)
};

//==============================================================================
template<typename T>
typename AtomicExchanger<T>::Token * AtomicExchanger<T>::acquire() noexcept
{
    auto expected{ true };
    for (auto & token : mData) {
        token.mIsFree.compare_exchange_strong(expected, false);
        if (expected) {
            return &token;
        }
        expected = true;
    }
    jassertfalse;
    return nullptr;
}

//==============================================================================
template<typename T>
void AtomicExchanger<T>::getMostRecent(Token *& tokenToUpdate) noexcept
{
    auto * mostRecent{ mMostRecent.exchange(nullptr) };
    if (mostRecent == nullptr) {
        return;
    }
    jassert(mostRecent != tokenToUpdate);
    jassert(!mostRecent->mIsFree.load());
    if (tokenToUpdate) {
        jassert(!tokenToUpdate->mIsFree.load());
        tokenToUpdate->mIsFree.store(true);
    }
    tokenToUpdate = mostRecent;
}

//==============================================================================
template<typename T>
void AtomicExchanger<T>::setMostRecent(Token * newMostRecent) noexcept
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
