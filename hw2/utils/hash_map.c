/* contributed by jw910731 */
#include "hash_map.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

static void entry_free(Entry *e, void (*free_callback)(void *), int d) {
    for (int i = 0; i < (1 << LAY_SIZE); ++i) {
        if (e->t[i] != NULL) {
            if (d < LAYER - 1) {
                entry_free((Entry *)e->t[i], free_callback, d + 1);
            } else {
                Item *p = (Item *)(e->t[i]);
                while (p != NULL) {
                    Item *tmp = p->next;
                    if (free_callback != NULL)
                        free_callback(p->data);
                    else
                        free(p->data);
                    free(p);
                    p = tmp;
                }
            }
        }
    }
    free(e);
}

// these implementation of hash is copied in following link
// https://byvoid.com/zht/blog/string-hash-compare/
uint32_t hash_str(const char *s) {
    // rs hash
    uint32_t b = 378551;
    uint32_t a = 63689;
    uint32_t hash = 0;
    while (*s) {
        hash = hash * a + (*s++);
        a *= b;
    }
    return (hash & 0x7FFFFFFF);
}

uint32_t backup_hash(const char *s) {
    // js hash
    uint32_t hash = 1315423911;
    while (*s) {
        hash ^= ((hash << 5) + (*s++) + (hash >> 2));
    }
    return (hash & 0x7FFFFFFF);
}

HashTable *table_create(void (*free_callback)(void *)) {
    HashTable *ret = calloc(1, sizeof(HashTable));
    ret->e = calloc(1, sizeof(Entry));
    ret->free_callback = free_callback;
    return ret;
}

void table_free(HashTable **t) {
    if (t == NULL || *t == NULL)
        return;
    entry_free((*t)->e, (*t)->free_callback, 0);
    free(*t);
    *t = NULL;
}

void **entry_allocate(HashTable *t, uint32_t hash) {
    if (t == NULL)
        return NULL;
    Entry *e = t->e;
    void **ret;
    for (int i = 0; i < LAYER; ++i) {
        if (e->t[MASK(hash, LAYER - 1 - i)] == NULL && i < LAYER - 1) {
            e->t[MASK(hash, LAYER - 1 - i)] = calloc(1, sizeof(Entry));
        }
        if (i < LAYER - 1) {
            e = (Entry *)e->t[MASK(hash, LAYER - 1 - i)];
        } else {
            ret = &(e->t[MASK(hash, LAYER - 1 - i)]);
        }
    }
    return ret;
}

void **entry_query(HashTable *t, uint32_t hash) {
    if (t == NULL)
        return NULL;
    Entry *e = t->e;
    void **ret;
    for (int i = 0; i < LAYER; ++i) {
        if (e->t[MASK(hash, LAYER - 1 - i)] == NULL && i < LAYER - 1) {
            return NULL;
        }
        if (i < LAYER - 1) {
            e = (Entry *)e->t[MASK(hash, LAYER - 1 - i)];
        } else {
            ret = &(e->t[MASK(hash, LAYER - 1 - i)]);
        }
    }
    return ret;
}

void table_emplace(HashTable *t, const char *key, const void *data,
                   const size_t size) {
    if (t == NULL)
        return;
    uint32_t hash = hash_str(key);
    uint32_t backup = backup_hash(key);
    // points to array pointer
    void **p = entry_allocate(t, hash);
    Item *item;
    // special case to NULL item in hash table (No collision)
    if (*p == NULL) {
        *p = calloc(1, sizeof(Item));
        item = (Item *)*p;
    } else { // on collision
        // copied of pointer
        item = (Item *)(*p);
        // Pointer reference for create new node in next
        Item **ptr = &item;
        while (item != NULL) {
            if (item->backup_hash == backup) {
                break;
            }
            ptr = &item->next;
            item = item->next;
        }
        if (*ptr == NULL) {
            // update prev -> next
            item = *ptr = calloc(1, sizeof(Item));
        }
    }
    if (item->data != NULL) {
        if (t->free_callback != NULL)
            t->free_callback(item->data);
        else
            free(item->data);
    }
    item->data = calloc(1, size);
    assert(item->data != NULL);
    memcpy(item->data, data, size);
    if (item->backup_hash != backup) {
        item->backup_hash = backup;
    }
}

Item *table_query(HashTable *t, const char *key) {
    if (t == NULL)
        return NULL;
    void **p = entry_allocate(t, hash_str(key));
    uint32_t backup = backup_hash(key);
    if (*p == NULL)
        return *p;
    Item *i = (Item *)*p;
    while (i->backup_hash != backup) {
        if (i->next != NULL) {
            i = i->next;
        } else {
            return NULL;
        }
    }
    return i;
}
