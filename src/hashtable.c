#include "hashtable.h"
#include "string.h"
#include <stdint.h>

String StringTable_find(StringTable *table, String string, uint32_t hash) {
    if (table->count == 0)
        return (String){.buffer = NULL, .length = 0};

    uint32_t index = hash % table->capacity;
    while (true) {
        StringTable_Entry *entry = &table->entries[index];
        if (entry->key.buffer == NULL) {
        }
    }
}
