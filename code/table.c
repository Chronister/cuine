
#error "TODO Incomplete"

#include <macro.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#ifndef TABLE_TYPE
#error "You must have TABLE_TYPE defined before including table.c."
#endif

#define t TABLE_TYPE

#ifndef table
#define table(T) IDCAT(T, _table)
#endif

typedef struct table_item(t) {
    const char* StoredKey;
    t* Value;

    struct table_item(t)* Next;
    struct table_item(t)* IterNext;
} table_item(t);

typedef struct {
    size_t BucketCount;
    table_item(t)* Buckets;
    table_item(t)* IterFirst;

    // Bookkeeping
    size_t Items;
} table(t);

extern size_t
ht_hash(const char* key) {
    size_t val = 0;
    for(key; *key; ++key) val += 37 * *key;
    return val;
}

extern struct hashtable*
ht_alloc(int nbuckets)
{
    struct hashtable* tab = (struct hashtable*)calloc(1, sizeof(struct hashtable));
    if (tab == NULL) { return NULL; }
    tab->nitems = 0;
    tab->nbuckets = nbuckets;
    tab->buckets = (struct ht_item*)calloc(nbuckets, sizeof(struct ht_item));
    if (tab->buckets == NULL) {
        free(tab);
        return NULL;
    }
    return tab;
}

extern float
ht_loadfactor(struct hashtable* tab) 
{ 
    return tab->nitems / tab->nbuckets; 
}

extern bool
ht_insert(struct hashtable* tab, const char* key, void* item)
{
    assert(tab != NULL);
    assert(key != NULL);

    uint bucket = ht_hash(key) % tab->nbuckets;
    struct ht_item* chain = &tab->buckets[bucket];
    if (chain == NULL) { return false; }

    if (chain->stored_key == NULL) {
        chain->stored_key = key;
        chain->value = item;
    }
    else {
        while (chain->next != NULL) { chain = chain->next; }
        struct ht_item* next = (struct ht_item*)calloc(1, sizeof(struct ht_item));
        if (next == NULL) { return false; }
        next->stored_key = key;
        next->value = item;
        chain->next = next;
        chain = next;
    }
    if (tab->iter_first == NULL) {
        tab->iter_first = chain;
    }
    else {
        chain->iter_next = tab->iter_first;
        tab->iter_first = chain;
    }
    tab->nitems++;
    return true;
}

extern void*
ht_lookup(struct hashtable* tab, const char* key) 
{
    assert(tab != NULL);
    assert(key != NULL);

    uint bucket = ht_hash(key) % tab->nbuckets;
    struct ht_item* chain = &tab->buckets[bucket];
    while (chain != NULL) {
        if (strcmp(chain->stored_key, key) == 0) return chain->value;
        chain = chain->next;
    }

    return NULL;
}

extern void ht_free(struct hashtable* tab)
{
    for (uint i = 0; i < tab->nbuckets; ++i)
    {
        struct ht_item* item = &tab->buckets[i];
        if (item->next != NULL) {
            struct ht_item* next = item->next;
            while (next != NULL) {
                item = next;
                next = item->next;
                free(item);
            }
        }
    }

    free(tab->buckets);
    free(tab);
}
