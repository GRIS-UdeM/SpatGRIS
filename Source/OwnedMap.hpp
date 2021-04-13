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
    // using iterator_type = typename map_type::Iterator;
    class iterator_type
    {
        map_type::Iterator mMapIterator;

    public:
        using iterator_category = std::forward_iterator_tag;
        using difference_type = int;
        using value_type = Node;
        using pointer = Node *;
        using reference = Node &;

        explicit iterator_type(typename map_type::Iterator iterator) : mMapIterator(std::move(iterator)) {}

        value_type operator*() const { return *mMapIterator; }

        iterator_type & operator++()
        {
            ++mMapIterator;
            return *this;
        }

        bool operator==(iterator_type const & other) const { return !(mMapIterator != other.mMapIterator); }
        bool operator!=(iterator_type const & other) const { return mMapIterator != other.mMapIterator; }
    };

private:
    //==============================================================================
    juce::OwnedArray<value_type> mItems;
    map_type mMap;

public:
    //==============================================================================
    OwnedMap() = default;
    ~OwnedMap() = default;
    //==============================================================================
    OwnedMap(OwnedMap const &) = delete;
    OwnedMap(OwnedMap && other) noexcept : mItems(std::move(other.mItems)) { mMap.swapWith(other.mMap); }
    OwnedMap & operator=(OwnedMap const &) = delete;
    OwnedMap & operator=(OwnedMap && other) noexcept
    {
        mItems = std::move(other.mItems);
        mMap.swapWith(other.mMap);
        return *this;
    }
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
    [[nodiscard]] iterator_type begin() { return iterator_type{ mMap.begin() }; }
    [[nodiscard]] iterator_type end() { return iterator_type{ mMap.end() }; }
    [[nodiscard]] iterator_type begin() const { return iterator_type{ mMap.begin() }; }
    [[nodiscard]] iterator_type end() const { return iterator_type{ mMap.end() }; }
    [[nodiscard]] iterator_type cbegin() const { return iterator_type{ mMap.begin() }; }
    [[nodiscard]] iterator_type cend() const { return iterator_type{ mMap.end() }; }
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
