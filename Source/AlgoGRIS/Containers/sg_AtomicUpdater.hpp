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

#include "../Data/sg_Macros.hpp"
#include "sg_StaticVector.hpp"

namespace gris
{
//==============================================================================
/** This class implements a thread-safe lock-free single-writer/single-reader communication interface.
 *
 * It provides a reader thread with the most recent data that made available by the writer thread.
 *
 * Note : simply using std::atomic<WholeDataStructure> was not an option because :
 *
 * - std::atomic are only lock-free with specific architecture-dependent primitive data types.
 * - the writer must be able to push new data at any time, without waiting, without disturbing the reader and without
 * having to rely on a queue to acquire or release tokens
 *
 * The writer thread uses acquire() to get an update token. This token can be used to retrieve dedicated data that can
 * be written to. When the writing is done, the writer thread calls setMostRecent(token) in order to update the
 * updater. If the writer does multiple updates before the reader accesses them, the older writes are simply tossed
 * away.
 *
 * The reader thread uses getMostRecent(token) to update it's local pointer to the most recently pushed token.
 */
template<typename T>
class AtomicUpdater
{
    // The number of tokens to use. The safe capacity is probably 4 or 5, but we still set it to 6 just to be extra
    // safe...
    static constexpr size_t CAPACITY = 6;

public:
    //==============================================================================
    /** An update token. Use the get() method to access its data. */
    class Token
    {
        friend AtomicUpdater;

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
    AtomicUpdater() = default;
    ~AtomicUpdater() = default;
    SG_DELETE_COPY_AND_MOVE(AtomicUpdater)
    //==============================================================================
    /** @returns a pointer to previously unused token. Use setMostRecent() to return the data to the updater after
     * writing to it. */
    Token * acquire() noexcept;
    /** Updates a local token pointer to the most recently pushed token.
     *
     * @param[out] tokenToUpdate the token address that should be updated (or not). This has to be nullptr when called
     * for the first time. Subsequent writes must always put in the address to the last token that they got through the
     * getMostRecent() call so that the updater knows when the data isn't in use anymore.
     */
    void getMostRecent(Token *& tokenToUpdate) noexcept;
    /** Sets a token as the most recent one.
     *
     * @param newMostRecent the address of a token acquired with acquire() that should replace the reader's token ASAP.
     */
    void setMostRecent(Token * newMostRecent) noexcept;
};

//==============================================================================
template<typename T>
typename AtomicUpdater<T>::Token * AtomicUpdater<T>::acquire() noexcept
{
    auto expected{ true };
    // Find a free token
    for (auto & token : mData) {
        token.mIsFree.compare_exchange_strong(expected, false);
        if (expected) {
            // found a free token
            return &token;
        }
        expected = true;
    }
    // This should never happen
    jassertfalse;
    return nullptr;
}

//==============================================================================
template<typename T>
void AtomicUpdater<T>::getMostRecent(Token *& tokenToUpdate) noexcept
{
    // Query most recent token
    auto * mostRecent{ mMostRecent.exchange(nullptr) };
    if (mostRecent == nullptr) {
        // No new token : tokenToUpdate is already the most recent one
        return;
    }
    // Found a newer token
    jassert(mostRecent != tokenToUpdate);
    jassert(!mostRecent->mIsFree.load());
    if (tokenToUpdate) {
        // Initial pointer was not nullptr and should be freed
        jassert(!tokenToUpdate->mIsFree.load());
        tokenToUpdate->mIsFree.store(true);
    }
    // actual update
    tokenToUpdate = mostRecent;
}

//==============================================================================
template<typename T>
void AtomicUpdater<T>::setMostRecent(Token * newMostRecent) noexcept
{
    jassert(newMostRecent);
    jassert(!newMostRecent->mIsFree.load());
    auto * oldMostRecent{ mMostRecent.exchange(newMostRecent) };
    jassert(newMostRecent != oldMostRecent);
    if (oldMostRecent == nullptr) {
        // This was the first token pushed since the last read
        return;
    }
    jassert(!oldMostRecent->mIsFree.load());
    // This update replaced an unread token: free it
    oldMostRecent->mIsFree.store(true);
}

} // namespace gris
