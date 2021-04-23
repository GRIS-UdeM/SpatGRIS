#pragma once

#include "StaticVector.hpp"

//==============================================================================
template<typename T>
class AtomicExchanger
{
    static constexpr size_t CAPACITY = 6;

public:
    //==============================================================================
    struct Ticket {
        T value{};
        std::atomic<bool> isFree{ true };
    };

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
            ticket.isFree.compare_exchange_strong(expected, false);
            if (expected) {
                return &ticket;
            }
            expected = true;
        }
        jassertfalse;
        return nullptr;
    }
    //==============================================================================
    void setMostRecent(Ticket * ticket) noexcept
    {
        jassert(!ticket->isFree.load());
        mMostRecent.exchange(ticket);
        if (ticket != nullptr) {
            jassert(!ticket->isFree.load());
            ticket->isFree.store(true);
        }
    }
    //==============================================================================
    void updateTicket(Ticket *& oldTicket) noexcept
    {
        jassert(oldTicket ? !oldTicket->isFree.load() : true);
        Ticket * mostRecentTicket{};
        mMostRecent.exchange(mostRecentTicket);
        if (mostRecentTicket == nullptr) {
            return;
        }
        jassert(!mostRecentTicket->isFree.load());
        if (oldTicket) {
            oldTicket->isFree.store(true);
        }
        oldTicket = mostRecentTicket;
    }
};