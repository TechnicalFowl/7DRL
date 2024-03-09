#pragma once

#include <bit>
#include <cstring>
#include <vector>

#include "util/string.h"

namespace hash {
    u32 fnv1a(const u8* data, size_t len);

    template <typename T>
    struct hash {
        u32 operator()(const T& key) const noexcept;
    };

    template <>
    struct hash<const char*>
    {
        u32 operator()(const char* key) const noexcept
        {
            return fnv1a((const u8*)key, strlen(key));
        }
    };

    template <>
    struct hash<sstring>
    {
        u32 operator()(const sstring& key) const noexcept
        {
            return fnv1a((const u8*)key.c_str(), key.size());
        }
    };

    template <>
    struct hash<u8>
    {
        u32 operator()(u8 key) const noexcept
        {
            u64 h = (14695981039346656037U ^ key) * 1099511628211U;
            u32 xorshifted = (u32)(((h >> 18u) ^ h) >> 27u);
            int rot = (int)(h >> 59u);
            return (xorshifted >> rot) | (xorshifted << ((-rot) & 31)) | 0x80000000;
        }
    };

    template <>
    struct hash<s8>
    {
        u32 operator()(s8 key) const noexcept
        {
            u64 h = (14695981039346656037U ^ key) * 1099511628211U;
            u32 xorshifted = (u32)(((h >> 18u) ^ h) >> 27u);
            int rot = (int)(h >> 59u);
            return (xorshifted >> rot) | (xorshifted << ((-rot) & 31)) | 0x80000000;
        }
    };

    template <>
    struct hash<u16>
    {
        u32 operator()(u16 key) const noexcept
        {
            u64 h = (14695981039346656037U ^ key) * 1099511628211U;
            u32 xorshifted = (u32)(((h >> 18u) ^ h) >> 27u);
            int rot = (int)(h >> 59u);
            return (xorshifted >> rot) | (xorshifted << ((-rot) & 31)) | 0x80000000;
        }
    };

    template <>
    struct hash<s16>
    {
        u32 operator()(s16 key) const noexcept
        {
            u64 h = (14695981039346656037U ^ key) * 1099511628211U;
            u32 xorshifted = (u32)(((h >> 18u) ^ h) >> 27u);
            int rot = (int)(h >> 59u);
            return (xorshifted >> rot) | (xorshifted << ((-rot) & 31)) | 0x80000000;
        }
    };

    template <>
    struct hash<u32>
    {
        u32 operator()(u32 key) const noexcept
        {
            return key | 0x80000000;
        }
    };

    template <>
    struct hash<s32>
    {
        u32 operator()(s32 key) const noexcept
        {
            u64 h = (14695981039346656037U ^ key) * 1099511628211U;
            u32 xorshifted = (u32)(((h >> 18u) ^ h) >> 27u);
            int rot = (int)(h >> 59u);
            return (xorshifted >> rot) | (xorshifted << ((-rot) & 31)) | 0x80000000;
        }
    };


    template <>
    struct hash<u64>
    {
        u32 operator()(u64 key) const noexcept
        {
            u64 h = 14695981039346656037U;
            h = (h ^ key & 0xFFFFFFFF) * 1099511628211U;
            h = (h ^ (key >> 32) & 0xFFFFFFFF) * 1099511628211U;
            u32 xorshifted = (u32)(((h >> 18u) ^ h) >> 27u);
            int rot = (int)(h >> 59u);
            return (xorshifted >> rot) | (xorshifted << ((-rot) & 31)) | 0x80000000;
        }
    };

    template <>
    struct hash<s64>
    {
        u32 operator()(s64 key) const noexcept
        {
            u64 h = 14695981039346656037U;
            h = (h ^ key & 0xFFFFFFFF) * 1099511628211U;
            h = (h ^ (key >> 32) & 0xFFFFFFFF) * 1099511628211U;
            u32 xorshifted = (u32)(((h >> 18u) ^ h) >> 27u);
            int rot = (int)(h >> 59u);
            return (xorshifted >> rot) | (xorshifted << ((-rot) & 31)) | 0x80000000;
        }
    };

    template <>
    struct hash<void*>
    {
        u32 operator()(void* key) const noexcept
        {
            return hash<u64>()((u64)key);
        }
    };

    template <typename T>
    struct compare {
        bool operator()(const T& a, const T& b) const noexcept { return a == b; }
    };

    template <>
    struct compare<const char*>
    {
        bool operator()(const char* a, const char* b) const noexcept
        {
            return ::strings::equals(a, b);
        }
    };
}

template <typename K, typename V>
struct map_entry {
    const K& key;
    V& value;

    map_entry(const K& key, V& value) : key(key), value(value) {}
};

// This is a linear probing hash map for keys and arbitrary values. The slot in
// the table for an entry is determined by taking the hash of the key modulo
// the table size. If the ideal slot for an entries is taken then it is
// attempted to place it into the next slot (probing). If an insertion fails to
// find an available slot within a maximum set of probes (log2 of the table
// size) then the table size is increased and all entries are re-inserted.
// 
// This table priorizes speed over size to a fairly significant extent but
// as a result it is significantly faster than std::unordered_map. In some
// cases with pathologically bad hashes it can grow to quite an extreme degree.
//
// The table size is always a power of 2 and is resized by doubling the size.
// The actual size of the table is increased by the maximum probe length. The
// starting slot to check is always within the table but the probe is allowed
// to set entries into the extended region. This allows us to remove all
// bounds checks from the insertion routine as the maximum probe length check
// will keep us in bounds.
//
// The idea to cap the maximum probe length and expand the table size by this
// in order to be able to remove bound checks came from:
// https://probablydance.com/2017/02/26/i-wrote-the-fastest-hashtable/
//
// I tested robin hood hashing but did not see an improvement in the
// benchmarks.
template <typename K, typename E, typename Hash = hash::hash<K>, typename Compare = hash::compare<K>>
struct linear_map
{

    struct LPMapEntry
    {
        K key;
        E value;
    };

    u32* hash_table;
    LPMapEntry* values;
    u32 table_size;
    u32 actual_table_size;
    u32 item_count;
    u32 max_probe_length;

    Hash hash;
    Compare compare;

    class value_t
    {
    public:
        E& value;
        bool found;

        value_t() : value((E&)*this), found(false) {}
        value_t(E& value, bool found) noexcept : value(value), found(found) {}

        value_t(const value_t&) = delete;
        value_t& operator=(const value_t&) = delete;

        operator bool() const {
            return found;
        }

        friend bool operator!(const value_t& v) { return !v.found; }

        E& operator->() {
            return value;
        }
    };

    linear_map() noexcept : linear_map(16) {}
    linear_map(u32 size) noexcept {
        table_size = std::bit_ceil(size);
        max_probe_length = (u32)log2(table_size);
        actual_table_size = table_size + max_probe_length;
        item_count = 0;

        hash_table = (u32*) malloc(actual_table_size * sizeof(u32));
        memset(hash_table, 0, actual_table_size * sizeof(u32));
        values = (LPMapEntry*)malloc(actual_table_size * sizeof(LPMapEntry));
        memset(values, 0, actual_table_size * sizeof(LPMapEntry));
    }

    linear_map(const linear_map& o) noexcept {
        table_size = o.table_size;
        max_probe_length = o.max_probe_length;
        actual_table_size = o.actual_table_size;
        item_count = 0;

        hash_table = (u32*)malloc(actual_table_size * sizeof(u32));
        memset(hash_table, 0, actual_table_size * sizeof(u32));
        values = (LPMapEntry*)malloc(actual_table_size * sizeof(LPMapEntry));
        memset(values, 0, actual_table_size * sizeof(LPMapEntry));

        for (u32 i = 0; i < actual_table_size; i++) {
            if (o.hash_table[i] != 0) {
                LPMapEntry* value_entry = &o.values[i];
                insert(value_entry->key, value_entry->value);
            }
        }
    }

    linear_map(linear_map&& o) noexcept {
        table_size = o.table_size;
        max_probe_length = o.max_probe_length;
        actual_table_size = o.actual_table_size;
        item_count = o.item_count;
        hash_table = o.hash_table;
        values = o.values;

        o.table_size = 0;
        o.max_probe_length = 0;
        o.actual_table_size = 0;
        o.item_count = 0;
        o.hash_table = nullptr;
        o.values = nullptr;
    }

    linear_map& operator=(const linear_map& o) noexcept {
        if (this != &o) {
            if (hash_table != nullptr) {
                // Walk the table to free the key if it is a copy and call the
                // destructors on the values.
                for (u32 i = 0; i < actual_table_size; i++) {
                    if (hash_table[i] != 0) {
                        LPMapEntry* value_entry = &values[i];
                        value_entry->key.~K();
                        value_entry->value.~E();
                    }
                }
                ::free(hash_table);
                ::free(values);
            }
            table_size = o.table_size;
            max_probe_length = o.max_probe_length;
            actual_table_size = o.actual_table_size;
            item_count = 0;

            hash_table = (u32*)malloc(actual_table_size * sizeof(u32));
            memset(hash_table, 0, actual_table_size * sizeof(u32));
            values = (LPMapEntry*)malloc(actual_table_size * sizeof(LPMapEntry));
            memset(values, 0, actual_table_size * sizeof(LPMapEntry));

            for (u32 i = 0; i < actual_table_size; i++) {
                if (o.hash_table[i] != 0) {
                    LPMapEntry* value_entry = &o.values[i];
                    insert(value_entry->key, value_entry->value);
                }
            }
        }
        return *this;
    }
    linear_map& operator=(linear_map&& o) noexcept {
        if (this != &o) {
            table_size = o.table_size;
            max_probe_length = o.max_probe_length;
            actual_table_size = o.actual_table_size;
            item_count = o.item_count;
            hash_table = o.hash_table;
            values = o.values;

            o.table_size = 0;
            o.max_probe_length = 0;
            o.actual_table_size = 0;
            o.item_count = 0;
            o.hash_table = nullptr;
            o.values = nullptr;
        }
        return *this;
    }

    ~linear_map() noexcept {
        if (hash_table == nullptr) {
            return;
        }
        // Walk the table to free the key if it is a copy and call the
        // destructors on the values.
        for (u32 i = 0; i < actual_table_size; i++) {
            if (hash_table[i] != 0) {
                LPMapEntry* value_entry = &values[i];
                value_entry->key.~K();
                value_entry->value.~E();
            }
        }
        ::free(hash_table); hash_table = nullptr;
        ::free(values); values = nullptr;
    }

    u32 size() const noexcept {
        return item_count;
    }

    u32 capacity() const noexcept {
        return table_size;
    }

    bool empty() const noexcept {
        return item_count == 0;
    }

    const E& operator[](const K& key) const {
        auto it = find(key);
        debug_assert(it.found);
        return it.value;
    }
    E& operator[](const K& key) {
        auto it = find(key);
        debug_assert(it.found);
        return it.value;
    }

    const E& at(const K& key) const {
        auto it = find(key);
        debug_assert(it.found);
        return it.value;
    }
    E& at(const K& key) {
        auto it = find(key);
        debug_assert(it.found);
        return it.value;
    }

    value_t find(const K& key) const noexcept {
        // Searching is done by finding the initial slot where the entry may
        // have been inserted (given by the hash modulo the table size) and
        // then checking up to the max probe length for a matching hash.
        u32 h = hash(key);
        u32 start = h & (table_size - 1);
        u32 end = start + max_probe_length;
        for (u32 i = start; i < end; i++) {
            if (hash_table[i] == h) {
                // If the hash matches then we compare the key contents.
                LPMapEntry* value_entry = &values[i];
                if (compare(key, value_entry->key)) {
                    return value_t(value_entry->value, true);
                }
            }
        }
        return value_t(values[0].value, false);
    }

    bool hasKey(const K& key) const noexcept {
        u32 h = hash(key);
        u32 start = h & (table_size - 1);
        u32 end = start + max_probe_length;
        for (u32 i = start; i < end; i++) {
            if (hash_table[i] == h) {
                LPMapEntry* value_entry = &values[i];
                if (compare(key, value_entry->key)) {
                    return true;
                }
            }
        }
        return false;
    }

    void insert(const K& key, const E& val) noexcept {
        E cpy = val;
        do_insert(key, std::move(cpy));
    }
    void insert(const K& key, E&& val) noexcept {
        do_insert(key, std::move(val));
    }

    value_t check_insert(const K& key, const E& val) noexcept {
        u32 h = hash(key);
        u32 start = h & (table_size - 1);
        u32 end = start + max_probe_length;
        u32 insertion_index = UINT32_MAX;
        for (u32 i = start; i < end; i++) {
            u32 entry_hash = hash_table[i];
            if (entry_hash == h) {
                LPMapEntry* value_entry = &values[i];
                if (compare(key, value_entry->key)) {
                    return value_t(value_entry->value, true);
                }
            }
            else if (entry_hash == 0) {
                insertion_index = i;
            }
        }
        if (insertion_index != UINT32_MAX) {
            item_count++;
            LPMapEntry* value_entry = &values[insertion_index];
            new (&value_entry->key) K(key);
            new (&value_entry->value) E(std::move(val));
            hash_table[insertion_index] = h;
            return value_t(value_entry->value, false);
        }
        // If we fail to insert within the max probe length then we need to
        // resize the map to be larger then then attempt to reinsert. We will
        // never need to resize more than once to insert a value because even
        // in the worst case where we are just inserting colliding keys the
        // resize will double the table size which will increase the max probe
        // length by one and allow our new entry to be inserted.
        resize();
        return check_insert(key, std::move(val));
    }

    bool erase(const K& key) noexcept {
        u32 h = hash(key);
        u32 start = h & (table_size - 1);
        u32 end = start + max_probe_length;
        for (u32 i = start; i < end; i++) {
            if (hash_table[i] == h) {
                LPMapEntry* value_entry = &values[i];
                if (compare(key, value_entry->key)) {
                    item_count--;
                    value_entry->key.~K();
                    value_entry->value.~E();
                    hash_table[i] = 0;
                    memset(value_entry, 0, sizeof(LPMapEntry));
                    return true;
                }
            }
        }
        return false;
    }

    void clear() noexcept {
        for (u32 i = 0; i < actual_table_size; i++) {
            if (hash_table[i] != 0) {
                LPMapEntry* value_entry = &values[i];
                value_entry->key.~K();
                value_entry->value.~E();
                hash_table[i] = 0;
                memset(value_entry, 0, sizeof(LPMapEntry));
            }
        }
        item_count = 0;
    }

    std::vector<K> keys() const {
        std::vector<K> result;
        result.reserve(item_count);
        for (size_t i = 0; i < actual_table_size; i++) {
            if (hash_table[i] != 0) {
                result.emplace_back(values[i].key);
            }
        }
        return result;
    }

    std::vector<E> get_values() const {
        std::vector<E> result;
        result.reserve(item_count);
        for (size_t i = 0; i < actual_table_size; i++) {
            if (hash_table[i] != 0) {
                result.push_back(values[i].value);
            }
        }
        return result;
    }

    // A helper function for inserting an entry, returns the entry struct for
    // the new value.
    LPMapEntry* do_insert(const K& key, E&& val) noexcept {
        u32 h = hash(key);
        u32 start = h & (table_size - 1);
        u32 end = start + max_probe_length;
        u32 insertion_index = UINT32_MAX;
        for (u32 i = start; i < end; i++) {
            u32 entry_hash = hash_table[i];
            if (entry_hash == h) {
                LPMapEntry* value_entry = &values[i];
                if (compare(key, value_entry->key)) {
                    value_entry->value = std::move(val);
                    return value_entry;
                }
            }
            else if (entry_hash == 0 && insertion_index == UINT32_MAX) {
                // If we find an empty space then track it for later. We still
                // need to check out to the max probe size to ensure the value
                // does not already exist.
                insertion_index = i;
            }
        }
        if (insertion_index != UINT32_MAX) {
            // We didn't find an existing entry for this key within the max
            // probe length, but we did fine an empty space where we can fit
            // it in.
            item_count++;
            LPMapEntry* value_entry = &values[insertion_index];
            new (&value_entry->key) K(key);
            new (&value_entry->value) E(std::move(val));
            hash_table[insertion_index] = h;
            return value_entry;
        }
        // If we fail to insert within the max probe length then we need to
        // resize the map to be larger than then attempt to reinsert. We will
        // never need to resize more than once to insert a value because even
        // in the worst case where we are just inserting colliding keys the
        // resize will double the table size which will increase the max probe
        // length by one and allow our new entry to be inserted.
        resize();
        return do_insert(key, std::move(val));
    }

    // Resizes the map to be twice as large and re-inserts all entries to the
    // new table.
    void resize() noexcept {
        u32* old_table = hash_table;
        LPMapEntry* old_values = values;
        u32 old_size = actual_table_size;
        table_size *= 2;
        max_probe_length = (u32)log2(table_size);
        actual_table_size = table_size + max_probe_length;
        item_count = 0;
        hash_table = (u32*) malloc(actual_table_size * sizeof(u32));
        memset(hash_table, 0, actual_table_size * sizeof(u32));
        values = (LPMapEntry*) malloc(actual_table_size * sizeof(LPMapEntry));
        memset(values, 0, actual_table_size * sizeof(LPMapEntry));

        for (u32 i = 0; i < old_size; i++) {
            u32 entry_hash = old_table[i];
            if (entry_hash != 0) {
                LPMapEntry* value_entry = &old_values[i];
                do_insert(value_entry->key, std::move(value_entry->value));
            }
        }

        ::free(old_table);
        ::free(old_values);
    }

    struct linear_map_const_iterator {
        const linear_map<K, E>* data;
        u32 index;

        explicit linear_map_const_iterator() : data(nullptr), index(UINT32_MAX) {}
        explicit linear_map_const_iterator(const linear_map<K, E>& d) : data(&d) {
            index = 0;
            if (data != nullptr)
            {
                while (index < data->actual_table_size)
                {
                    if (data->hash_table[index] != 0) break;
                    index++;
                }
            }
        }
        linear_map_const_iterator(const linear_map<K, E>& d, u32 i) : data(&d), index(i) {}

        friend bool operator==(const linear_map_const_iterator& a,
            const linear_map_const_iterator& b) {
            return a.data == b.data && a.index == b.index;
        }
        friend bool operator!=(const linear_map_const_iterator& a,
            const linear_map_const_iterator& b) {
            return a.data != b.data || a.index != b.index;
        }

        void advance() {
            if (data == nullptr || index >= data->actual_table_size)
                return;
            index++;
            while (index < (s32)data->actual_table_size)
            {
                if (data->hash_table[index] != 0) break;
                index++;
            }
        }

        bool has() const {
            if (data == nullptr || index >= data->actual_table_size)
                return false;
            return index < data->actual_table_size
                && index >= 0
                && data->hash_table[index] != 0;
        }

        bool hasNext() {
            if (data == nullptr || index >= data->actual_table_size)
                return false;
            u32 next = index + 1;
            while (next < data->actual_table_size)
            {
                if (data->hash_table[next] != 0) break;
                next++;
            }
            return next < data->actual_table_size
                && next >= 0
                && data->hash_table[next] != 0;
        }

        const K& key() {
            if (!has()) {
                if (!hasNext()) {
                    abort();
                }
            }
            return data->values[index].key;
        }

        E& value() {
            if (!has()) {
                if (!hasNext()) {
                    abort();
                }
            }
            return data->values[index].value;
        }

        map_entry<K, E> operator*() {
            if (!has()) {
                if (!hasNext()) {
                    abort();
                }
            }
            return map_entry(data->values[index].key, data->values[index].value);
        }

        map_entry<K, E> operator->() {
            if (!has()) {
                if (!hasNext()) {
                    abort();
                }
            }
            return map_entry(data->values[index].key, data->values[index].value);
        }

        linear_map_const_iterator& operator++() {
            advance();
            return *this;
        }

        linear_map_const_iterator& operator--() {
            if (data == nullptr) {
                return *this;
            }
            if (index >= data->actual_table_size) {
                index = data->actual_table_size;
            }
            while (index >= 0) {
                index--;
                if (data->hash_table[index] != 0) {
                    break;
                }
            }
            return *this;
        }

        friend bool operator<(const linear_map_const_iterator& a,
            const linear_map_const_iterator& b) {
            return a.data == b.data && a.index < b.index;
        }
        friend bool operator<=(const linear_map_const_iterator& a,
            const linear_map_const_iterator& b) {
            return a.data == b.data && a.index <= b.index;
        }
        friend bool operator>(const linear_map_const_iterator& a,
            const linear_map_const_iterator& b) {
            return a.data == b.data && a.index > b.index;
        }
        friend bool operator>=(const linear_map_const_iterator& a,
            const linear_map_const_iterator& b) {
            return a.data == b.data && a.index >= b.index;
        }

    };

    linear_map_const_iterator begin() const noexcept {
        return linear_map_const_iterator(*this);
    }

    linear_map_const_iterator cbegin() const noexcept {
        return linear_map_const_iterator(*this);
    }

    linear_map_const_iterator end() const noexcept {
        return linear_map_const_iterator(*this, actual_table_size);
    }

    linear_map_const_iterator cend() const noexcept {
        return linear_map_const_iterator(*this, actual_table_size);
    }

};
