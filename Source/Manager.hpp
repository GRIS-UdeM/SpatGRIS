#pragma once

#include "StrongTypes.hpp"

//==============================================================================
template<typename T, typename U>
class Manager
{
public:
    using value_type = T;
    using key_type = U;

private:
    //==============================================================================
    static_assert(std::is_base_of_v<StrongIndexBase, key_type>);
    //==============================================================================
    struct ExtendedHashFunctions : juce::DefaultHashFunctions {
        static int generateHash(key_type const index, int const upperLimit)
        {
            return juce::DefaultHashFunctions::generateHash(index.get(), upperLimit);
        }
    };
    //==============================================================================
    juce::CriticalSection mLock;
    juce::OwnedArray<value_type> mItems;
    juce::HashMap<key_type, value_type *, ExtendedHashFunctions> mMap;

public:
    //==============================================================================
    Manager() = default;
    ~Manager() = default;
    //==============================================================================
    Manager(Manager const &) = delete;
    Manager(Manager &&) = delete;
    Manager & operator=(Manager const &) = delete;
    Manager & operator=(Manager &&) = delete;
    //==============================================================================
    [[nodiscard]] bool contains(key_type const key) const { return mMap.contains(key); }
    [[nodiscard]] int size() const { return mItems.size(); }
    [[nodiscard]] bool isEmpty() const { return mItems.isEmpty(); }
    //==============================================================================
    [[nodiscard]] value_type const & get(key_type const key) const
    {
        jassert(contains(key));
        return *mMap[key];
    }
    //==============================================================================
    [[nodiscard]] value_type & get(key_type const key)
    {
        jassert(contains(key));
        return *mMap[key];
    }
    //==============================================================================
    value_type & add(key_type const key, std::unique_ptr<value_type> value)
    {
        jassert(!contains(key));
        mItems.add(value.get());
        mMap.set(key, value.get());
        return *value.release();
    }
    //==============================================================================
    void remove(key_type const key)
    {
        jassert(contains(key));
        mItems.removeObject(mMap[key]);
        mMap.remove(key);
    }
    //==============================================================================
    [[nodiscard]] auto const & getLock() const { return mLock; }
    //==============================================================================
    [[nodiscard]] value_type * const * begin() { return mItems.data(); }
    [[nodiscard]] value_type * const * end() { return mItems.data() + size(); }
    [[nodiscard]] value_type const * const * begin() const { return mItems.data(); }
    [[nodiscard]] value_type const * const * end() const { return mItems.data() + size(); }
    [[nodiscard]] value_type const * const * cbegin() const { return mItems.data(); }
    [[nodiscard]] value_type const * const * cend() const { return mItems.data() + size(); }
    //==============================================================================
    void clear()
    {
        mItems.clearQuick(true);
        mMap.clear();
    }

private:
    //==============================================================================
    JUCE_LEAK_DETECTOR(Manager)
};
