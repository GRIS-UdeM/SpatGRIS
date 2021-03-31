#pragma once

#include "StrongTypes.hpp"

//==============================================================================
template<typename KeyType, typename ValueType>
class OwnedMap
{
public:
    using key_type = KeyType;
    using value_type = ValueType;
    struct Node {
        key_type key{};
        value_type * value{};
    };

private:
    static_assert(std::is_base_of_v<StrongIndexBase, key_type>);
    struct ExtendedHashFunctions : juce::DefaultHashFunctions {
        static int generateHash(key_type const key, int const upperLimit)
        {
            return juce::DefaultHashFunctions::generateHash(key.get(), upperLimit);
        }
    };
    using map_type = juce::HashMap<key_type, Node, ExtendedHashFunctions>;

public:
    using iterator_type = typename map_type::Iterator;

private:
    //==============================================================================
    juce::CriticalSection mCriticalSection;
    juce::OwnedArray<value_type> mItems;
    map_type mMap;

public:
    //==============================================================================
    OwnedMap() = default;
    ~OwnedMap() = default;
    //==============================================================================
    OwnedMap(OwnedMap const &) = delete;
    OwnedMap(OwnedMap &&) = delete;
    OwnedMap & operator=(OwnedMap const &) = delete;
    OwnedMap & operator=(OwnedMap &&) = delete;
    //==============================================================================
    [[nodiscard]] bool contains(key_type const key) const { return mMap.contains(key); }
    [[nodiscard]] int size() const { return mItems.size(); }
    [[nodiscard]] bool isEmpty() const { return mItems.isEmpty(); }
    //==============================================================================
    [[nodiscard]] value_type const & operator[](key_type const key) const
    {
        jassert(contains(key));
        return *mMap[key].value;
    }
    //==============================================================================
    [[nodiscard]] value_type & operator[](key_type const key)
    {
        jassert(contains(key));
        return *mMap[key].value;
    }
    //==============================================================================
    value_type & add(key_type const key, std::unique_ptr<value_type> value)
    {
        jassert(!contains(key));
        mItems.add(value.get());
        mMap.set(key, Node{ key, value.get() });
        return *value.release();
    }
    //==============================================================================
    void remove(key_type const key)
    {
        jassert(contains(key));
        mItems.removeObject(mMap[key].value);
        mMap.remove(key);
    }
    //==============================================================================
    [[nodiscard]] auto const & getCriticalSection() const { return mCriticalSection; }
    //==============================================================================
    [[nodiscard]] iterator_type begin() { return mMap.begin(); }
    [[nodiscard]] iterator_type end() { return mMap.end(); }
    [[nodiscard]] iterator_type begin() const { return mMap.begin(); }
    [[nodiscard]] iterator_type end() const { return mMap.end(); }
    [[nodiscard]] iterator_type cbegin() const { return mMap.begin(); }
    [[nodiscard]] iterator_type cend() const { return mMap.end(); }
    //==============================================================================
    void clear()
    {
        mItems.clearQuick(true);
        mMap.clear();
    }

private:
    //==============================================================================
    JUCE_LEAK_DETECTOR(OwnedMap)
};
