#pragma once

// NEXA DSA #21 - Hash Tables (separate chaining AND linear probing)
// Fast incident / resource lookup by ID or code.
//   ChainedHashTable       : bucket array of singly linked chains
//   LinearProbingHashTable : open addressing with tombstones
// Both resize at load factor 0.75. Search / insert O(1) average, O(n) worst.

#include <cstddef>
#include <string>

namespace nexa::dsa
{

// FNV-1a for strings, SplitMix64 finaliser for integers.
inline std::size_t hashKey(const std::string& key)
{
    unsigned long long h = 1469598103934665603ULL;
    for (unsigned char c : key)
    {
        h ^= c;
        h *= 1099511628211ULL;
    }
    return static_cast<std::size_t>(h);
}

inline std::size_t hashKey(unsigned long long key)
{
    key += 0x9E3779B97F4A7C15ULL;
    key = (key ^ (key >> 30)) * 0xBF58476D1CE4E5B9ULL;
    key = (key ^ (key >> 27)) * 0x94D049BB133111EBULL;
    return static_cast<std::size_t>(key ^ (key >> 31));
}

template <typename Key, typename Value>
class ChainedHashTable
{
private:
    struct Entry
    {
        Key key;
        Value value;
        Entry* next;
    };

    Entry** buckets_ = nullptr;
    std::size_t bucketCount_ = 0;
    std::size_t size_ = 0;
    std::size_t collisions_ = 0;

    void rehash(std::size_t newCount)
    {
        Entry** fresh = new Entry*[newCount]{};
        for (std::size_t b = 0; b < bucketCount_; ++b)
        {
            Entry* e = buckets_[b];
            while (e)
            {
                Entry* next = e->next;
                const std::size_t index = hashKey(e->key) % newCount;
                e->next = fresh[index];
                fresh[index] = e;
                e = next;
            }
        }
        delete[] buckets_;
        buckets_ = fresh;
        bucketCount_ = newCount;
    }

public:
    explicit ChainedHashTable(std::size_t initialBuckets = 16)
    {
        bucketCount_ = initialBuckets < 4 ? 4 : initialBuckets;
        buckets_ = new Entry*[bucketCount_]{};
    }

    ~ChainedHashTable()
    {
        for (std::size_t b = 0; b < bucketCount_; ++b)
        {
            Entry* e = buckets_[b];
            while (e)
            {
                Entry* next = e->next;
                delete e;
                e = next;
            }
        }
        delete[] buckets_;
    }

    ChainedHashTable(const ChainedHashTable&) = delete;
    ChainedHashTable& operator=(const ChainedHashTable&) = delete;

    // Returns false if the key existed (value is replaced).
    bool put(const Key& key, const Value& value)
    {
        const std::size_t index = hashKey(key) % bucketCount_;
        for (Entry* e = buckets_[index]; e; e = e->next)
            if (e->key == key) { e->value = value; return false; }

        if (buckets_[index]) ++collisions_;
        buckets_[index] = new Entry{key, value, buckets_[index]};
        ++size_;
        if (size_ * 4 > bucketCount_ * 3)
            rehash(bucketCount_ * 2);
        return true;
    }

    const Value* get(const Key& key) const
    {
        for (Entry* e = buckets_[hashKey(key) % bucketCount_]; e; e = e->next)
            if (e->key == key)
                return &e->value;
        return nullptr;
    }

    bool remove(const Key& key)
    {
        Entry** link = &buckets_[hashKey(key) % bucketCount_];
        while (*link)
        {
            if ((*link)->key == key)
            {
                Entry* dead = *link;
                *link = dead->next;
                delete dead;
                --size_;
                return true;
            }
            link = &(*link)->next;
        }
        return false;
    }

    template <typename Visitor>
    void forEach(Visitor visit) const
    {
        for (std::size_t b = 0; b < bucketCount_; ++b)
            for (Entry* e = buckets_[b]; e; e = e->next)
                visit(e->key, e->value);
    }

    std::size_t size() const { return size_; }
    std::size_t bucketCount() const { return bucketCount_; }
    std::size_t collisions() const { return collisions_; }
    double loadFactor() const { return static_cast<double>(size_) / static_cast<double>(bucketCount_); }
};

template <typename Key, typename Value>
class LinearProbingHashTable
{
private:
    enum class SlotState : unsigned char { Empty, Occupied, Deleted };

    struct Slot
    {
        Key key{};
        Value value{};
        SlotState state = SlotState::Empty;
    };

    Slot* slots_ = nullptr;
    std::size_t capacity_ = 0;
    std::size_t size_ = 0;
    std::size_t used_ = 0; // occupied + tombstones
    std::size_t probes_ = 0;

    void rehash(std::size_t newCapacity)
    {
        Slot* old = slots_;
        const std::size_t oldCapacity = capacity_;
        slots_ = new Slot[newCapacity];
        capacity_ = newCapacity;
        size_ = used_ = 0;
        for (std::size_t i = 0; i < oldCapacity; ++i)
            if (old[i].state == SlotState::Occupied)
                put(old[i].key, old[i].value);
        delete[] old;
    }

public:
    explicit LinearProbingHashTable(std::size_t initialCapacity = 16)
    {
        capacity_ = initialCapacity < 4 ? 4 : initialCapacity;
        slots_ = new Slot[capacity_];
    }

    ~LinearProbingHashTable() { delete[] slots_; }

    LinearProbingHashTable(const LinearProbingHashTable&) = delete;
    LinearProbingHashTable& operator=(const LinearProbingHashTable&) = delete;

    bool put(const Key& key, const Value& value)
    {
        if ((used_ + 1) * 4 > capacity_ * 3)
            rehash(capacity_ * 2);

        std::size_t index = hashKey(key) % capacity_;
        std::size_t firstTombstone = capacity_;
        while (slots_[index].state != SlotState::Empty)
        {
            if (slots_[index].state == SlotState::Occupied && slots_[index].key == key)
            {
                slots_[index].value = value;
                return false;
            }
            if (slots_[index].state == SlotState::Deleted && firstTombstone == capacity_)
                firstTombstone = index;
            index = (index + 1) % capacity_;
            ++probes_;
        }
        if (firstTombstone != capacity_)
            index = firstTombstone;
        else
            ++used_;
        slots_[index].key = key;
        slots_[index].value = value;
        slots_[index].state = SlotState::Occupied;
        ++size_;
        return true;
    }

    const Value* get(const Key& key) const
    {
        std::size_t index = hashKey(key) % capacity_;
        for (std::size_t step = 0; step < capacity_ && slots_[index].state != SlotState::Empty; ++step)
        {
            if (slots_[index].state == SlotState::Occupied && slots_[index].key == key)
                return &slots_[index].value;
            index = (index + 1) % capacity_;
        }
        return nullptr;
    }

    bool remove(const Key& key)
    {
        std::size_t index = hashKey(key) % capacity_;
        for (std::size_t step = 0; step < capacity_ && slots_[index].state != SlotState::Empty; ++step)
        {
            if (slots_[index].state == SlotState::Occupied && slots_[index].key == key)
            {
                slots_[index].state = SlotState::Deleted;
                --size_;
                return true;
            }
            index = (index + 1) % capacity_;
        }
        return false;
    }

    std::size_t size() const { return size_; }
    std::size_t capacity() const { return capacity_; }
    std::size_t probes() const { return probes_; }
};

} // namespace nexa::dsa
