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

#define TABLE_NEW_SIG(T, Name) Name##Table Name##Table_new();
#define TABLE_NEW(T, Name)                                                     \
    Name##Table Name##Table_new() {                                            \
        return (Name##Table){                                                  \
            .count = 0,                                                        \
            .capacity = 0,                                                     \
            .entries = NULL,                                                   \
        };                                                                     \
    }

#define TABLE_FREE_SIG(T, Name) void Name##Table_free(Name##Table *table);
#define TABLE_FREE(T, Name)                                                    \
    void Name##Table_free(Name##Table *table) {                                \
        free(table->entries);                                                  \
        *table = Name##Table_new();                                            \
    }

static uint32_t fnv_1a_hash(String string) {
    uint32_t hash = 2166136261u;
    for (uint32_t i = 0; i < string.length; i++) {
        hash ^= (uint8_t)string.buffer[i];
        hash *= 16777619;
    }
    return hash;
}

#define TABLE_ENTRY_FIND_SIG(T, Name)                                          \
    static Name##Table_Entry *Name##Table_Entry_find(                          \
        Name##Table_Entry *entries, size_t capacity, String key);

// Pre-conditions:
//
// * `key` is an interned string
//
#define TABLE_ENTRY_FIND(T, Name)                                              \
    static Name##Table_Entry *Name##Table_Entry_find(                          \
        Name##Table_Entry *entries, size_t capacity, String key) {             \
        uint32_t index = fnv_1a_hash(key) % capacity;                          \
        Name##Table_Entry *tombstone = NULL;                                   \
        while (true) {                                                         \
            Name##Table_Entry *entry = &entries[index];                        \
            if (entry->key.buffer == NULL) {                                   \
                if (entry->key.length == 0) {                                  \
                    return tombstone != NULL ? tombstone : entry;              \
                } else {                                                       \
                    if (tombstone == NULL)                                     \
                        tombstone = entry;                                     \
                }                                                              \
            } else if (entry->key.buffer == key.buffer) {                      \
                return entry;                                                  \
            }                                                                  \
                                                                               \
            index = (index + 1) % capacity;                                    \
        }                                                                      \
    }

#define TABLE_ADJUST_CAPACITY_SIG(T, Name)                                     \
    static void Name##Table_adjust_capacity(Name##Table *table,                \
                                            size_t capacity);

// Resizes a table by simply allocating a new array of entries and copying over
// the old ones.
#define TABLE_ADJUST_CAPACITY(T, Name)                                         \
    static void Name##Table_adjust_capacity(Name##Table *table,                \
                                            size_t capacity) {                 \
        Name##Table_Entry *entries =                                           \
            reallocate(NULL, sizeof(Name##Table_Entry) * capacity);            \
        for (size_t i = 0; i < capacity; i++) {                                \
            entries[i].key = (String){.buffer = NULL, .length = 0};            \
        }                                                                      \
        table->count = 0;                                                      \
        for (size_t i = 0; i < table->capacity; i++) {                         \
            Name##Table_Entry *entry = &table->entries[i];                     \
            if (entry->key.buffer != NULL) {                                   \
                Name##Table_Entry *dest =                                      \
                    Name##Table_Entry_find(entries, capacity, entry->key);     \
                                                                               \
                dest->key = entry->key;                                        \
                dest->value = entry->value;                                    \
                table->count++;                                                \
            }                                                                  \
        }                                                                      \
        reallocate(table->entries, 0);                                         \
        table->entries = entries;                                              \
        table->capacity = capacity;                                            \
    }

#define TABLE_SET_SIG(T, Name)                                                 \
    bool Name##Table_set(Name##Table *table, String key, int value);

// Sets the value corresponding to `key` to `value`, overwriting an existing
// value or creating a new entry. This function will return true if a new entry
// was created.
#define TABLE_SET(T, Name)                                                     \
    bool Name##Table_set(Name##Table *table, String key, int value) {          \
        if (table->count + 1 > table->capacity * TABLE_MAX_LOAD_FACTOR) {      \
            size_t capacity = grow_allocation(table->capacity);                \
            Name##Table_adjust_capacity(table, capacity);                      \
        }                                                                      \
        Name##Table_Entry *entry =                                             \
            Name##Table_Entry_find(table->entries, table->capacity, key);      \
        bool key_is_new = entry->key.buffer == NULL;                           \
        if (key_is_new && entry->key.length == 0)                              \
            table->count++;                                                    \
                                                                               \
        entry->key = key;                                                      \
        entry->value = value;                                                  \
        return key_is_new;                                                     \
    }

#define TABLE_GET_SIG(T, Name)                                                 \
    bool Name##Table_get(Name##Table *table, String key, /* out */ int *value);

// Attempts to find the entry identified by `key`, returning `false` if not
// found, else, returning `true` and writing the corresponding value to `value`.
#define TABLE_GET(T, Name)                                                     \
    bool Name##Table_get(Name##Table *table, String key,                       \
                         /* out */ int *value) {                               \
        if (table->count == 0)                                                 \
            return false;                                                      \
                                                                               \
        Name##Table_Entry *entry =                                             \
            Name##Table_Entry_find(table->entries, table->capacity, key);      \
        if (entry->key.buffer != NULL) {                                       \
            *value = entry->value;                                             \
            return true;                                                       \
        } else                                                                 \
            return false;                                                      \
    }

#define TABLE_DELETE_SIG(T, Name)                                              \
    bool Name##Table_delete(Name##Table *table, String key);

// Attempts to delete an entry, replacing it with a tombstone entry. Returns
// `false` if the entry does not exist, and `true` if it was deleted.
#define TABLE_DELETE(T, Name)                                                  \
    bool Name##Table_delete(Name##Table *table, String key) {                  \
        if (table->count == 0)                                                 \
            return false;                                                      \
                                                                               \
        Name##Table_Entry *entry =                                             \
            Name##Table_Entry_find(table->entries, table->capacity, key);      \
        if (entry->key.buffer != NULL) {                                       \
            entry->key = (String){.buffer = NULL, .length = 1};                \
            return true;                                                       \
        } else                                                                 \
            return false;                                                      \
    }

#define DECL_TABLE(T, Name)                                                    \
    CREATE_ENTRY(T, Name)                                                      \
    CREATE_TABLE(T, Name)                                                      \
    TABLE_NEW_SIG(T, Name)                                                     \
    TABLE_FREE_SIG(T, Name)                                                    \
    TABLE_ENTRY_FIND_SIG(T, Name)                                              \
    TABLE_ADJUST_CAPACITY_SIG(T, Name)                                         \
    TABLE_SET_SIG(T, Name) TABLE_GET_SIG(T, Name) TABLE_DELETE_SIG(T, Name)

#define DEF_TABLE(T, Name)                                                     \
    TABLE_NEW(T, Name)                                                         \
    TABLE_FREE(T, Name)                                                        \
    TABLE_ENTRY_FIND(T, Name)                                                  \
    TABLE_ADJUST_CAPACITY(T, Name)                                             \
    TABLE_SET(T, Name) TABLE_GET(T, Name) TABLE_DELETE(T, Name)

CREATE_ENTRY(void *, String)
CREATE_TABLE(void *, String)

String StringTable_find(StringTable *table, String string, uint32_t hash);

#endif /* CLAM_HASHTABLE_H */
