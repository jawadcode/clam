#ifndef CLAM_HASHTABLE_H
#define CLAM_HASHTABLE_H

/* This file is definitely a punishable offense in several countries. */

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "src/memory.h"
#include "string.h"

constexpr double TABLE_MAX_LOAD_FACTOR = 0.75;

#define CREATE_ENTRY(T, Name)                                                  \
    typedef struct Name##Table_Entry {                                         \
        String key;                                                            \
        T value;                                                               \
    } Name##Table_Entry;

#define CREATE_TABLE(T, Name)                                                  \
    typedef struct Name##Table {                                               \
        size_t count;                                                          \
        size_t capacity;                                                       \
        Name##Table_Entry *entries;                                            \
    } Name##Table;

CREATE_ENTRY(int, Int)
CREATE_TABLE(int, Int)

IntTable IntTable_new() {
    return (IntTable){
        .count = 0,
        .capacity = 0,
        .entries = NULL,
    };
}

void IntTable_free(IntTable *table) {
    free(table->entries);
    *table = IntTable_new();
}

static uint32_t fnv_1a_hash(String string) {
    uint32_t hash = 2166136261u;
    for (uint32_t i = 0; i < string.length; i++) {
        hash ^= (uint8_t)string.buffer[i];
        hash *= 16777619;
    }
    return hash;
}

// Pre-conditions:
//
// * `key` is an interned string
//
static IntTable_Entry *IntTable_Entry_find(IntTable_Entry *entries,
                                           size_t capacity, String key) {
    uint32_t index = fnv_1a_hash(key) % capacity;
    IntTable_Entry *tombstone = NULL;
    while (true) {
        IntTable_Entry *entry = &entries[index];
        if (entry->key.buffer == NULL) {
            // Empty entry
            if (entry->key.length == 0) {
                return tombstone != NULL ? tombstone : entry;
            }
            // Tombstone entry
            else /* if (entry->key.length == 1) */ {
                if (tombstone == NULL)
                    tombstone = entry;
            }
        }
        // We can perform a simple pointer comparison rather than a `memcmp`
        // because the strings are assumed to be interned.
        else if (entry->key.buffer == key.buffer) {
            return entry;
        }

        index = (index + 1) % capacity;
    }
}

// Resizes a table by simply allocating a new array of entries and copying over
// the old ones.
static void IntTable_adjust_capacity(IntTable *table, size_t capacity) {
    IntTable_Entry *entries =
        reallocate(NULL, sizeof(IntTable_Entry) * capacity);
    for (size_t i = 0; i < capacity; i++) {
        // The `value` is left uninitialised as it will never be accessed
        // without first being overwritten
        // A `length` of 0 indicates an empty entry
        entries[i].key = (String){.buffer = NULL, .length = 0};
    }

    // The count is reset to 0 as we don't know how many of the entries are
    // tombstones (which will not be copied over)
    table->count = 0;
    // Re-insert all the old entries
    for (size_t i = 0; i < table->capacity; i++) {
        IntTable_Entry *entry = &table->entries[i];
        if (entry->key.buffer != NULL) {
            IntTable_Entry *dest =
                IntTable_Entry_find(entries, capacity, entry->key);
            dest->key = entry->key;
            dest->value = entry->value;
            table->count++;
        }
    }
    reallocate(table->entries, 0);
    table->entries = entries;
    table->capacity = capacity;
}

// Sets the value corresponding to `key` to `value`, overwriting an existing
// value or creating a new entry. This function will return true if a new entry
// was created.
bool IntTable_set(IntTable *table, String key, int value) {
    if (table->count + 1 > table->capacity * TABLE_MAX_LOAD_FACTOR) {
        size_t capacity = grow_allocation(table->capacity);
        IntTable_adjust_capacity(table, capacity);
    }
    IntTable_Entry *entry =
        IntTable_Entry_find(table->entries, table->capacity, key);
    bool key_is_new = entry->key.buffer == NULL;
    // Tombstones are counted as non-empty entries so that `XTable_Entry_find`
    // does not end up in an infinite loop
    if (key_is_new && entry->key.length == 0)
        table->count++;

    entry->key = key;
    entry->value = value;
    return key_is_new;
}

// Attempts to find the entry identified by `key`, returning `false` if not
// found, else, returning `true` and writing the corresponding value to `value`.
bool IntTable_get(IntTable *table, String key,
                  /* out */ int *value) {
    if (table->count == 0)
        return false;

    IntTable_Entry *entry =
        IntTable_Entry_find(table->entries, table->capacity, key);
    if (entry->key.buffer != NULL) {
        *value = entry->value;
        return true;
    } else
        return false;
}

// Attempts to delete an entry, replacing it with a tombstone entry. Returns
// `false` if the entry does not exist, and `true` if it was deleted.
bool IntTable_delete(IntTable *table, String key) {
    if (table->count == 0)
        return false;

    IntTable_Entry *entry =
        IntTable_Entry_find(table->entries, table->capacity, key);
    if (entry->key.buffer != NULL) {
        // The `value` is once again left uninitialised as it will not be read
        // before being overwritten
        // A `length` of 1 indicates a tombstone value
        entry->key = (String){.buffer = NULL, .length = 1};
        return true;
    } else
        return false;
}

#endif /* CLAM_HASHTABLE_H */
