#include "general.h"
#include <stdio.h>
#include <stdlib.h>

#define INITFBUF_SIZE 128

u_char *readfile (const char *file)
{
    FILE *f;
    size_t bsize, nbytes;
    u_char *buf;
    
    bsize = INITFBUF_SIZE;
    f = fopen (file, "r");
    if (!f) {
        perror("File IO Error");
        return NULL;
    }
    buf = malloc(INITFBUF_SIZE);
    if (!buf) {
        perror("Heap Allocation Error");
        fclose(f);
        return NULL;
    }
    for (nbytes = 0; (buf[nbytes] = fgetc(f)) != UEOF; nbytes++) {
        if (nbytes+1 == bsize) {
            bsize *= 2;
            buf = realloc(buf, bsize);
            if (!buf) {
                perror("Memory Allocation Error");
                fclose(f);
                return NULL;
            }
        }
    }
    fclose(f);
    if (nbytes < bsize)
        buf = realloc(buf, nbytes+1);
    return buf;
}

void llpush (llist_s **list, void *ptr)
{
    llist_s *node;
    
    node = malloc(sizeof(*node));
    if (!node) {
        perror("Memory Allocation Error");
        exit(EXIT_FAILURE);
    }
    node->ptr = ptr;
    node->next = *list;
    *list = node;
}

llist_s *llpop (llist_s **list)
{
    llist_s *backup;
    
    backup = *list;
    *list = (*list)->next;
    return backup;
}

bool llcontains (llist_s *list, void *ptr)
{
    while (list) {
        if (list->ptr == ptr)
            return true;
        list = list->next;
    }
    return false;
}

llist_s *llconcat (llist_s *first, llist_s *second)
{
    llist_s *head;
    
    if (!first)
        return second;
    for (head = first; first->next; first = first->next);
    first->next = second;
    return head;
}

void free_llist (void *list)
{
    void *backup;
    
    while (list) {
        backup = list;
        list = *(void **)list;
        free(backup);
    }
}

void println (uint16_t no, u_char *buf)
{
    printf("%6d: ", no);
    do {
        if (*buf == UEOF)
            return;
        putchar(*buf);
    }
    while (*buf++ != '\n');
}

hash_s *hash_(hash_f hashf, isequal_f isequalf)
{
    hash_s *hash;
    
    hash = calloc(1, sizeof(*hash));
    if (!hash) {
        perror("Memory Allocation Error");
        exit(EXIT_FAILURE);
    }
    hash->hash = hashf;
    hash->isequal = isequalf;
    return hash;
}

bool hashinsert (hash_s *hash, void *key, void *data)
{
    hrecord_s *record, *new, *iter;
    
    record = &hash->table[hash->hash(key)];
    if (!record->isoccupied) {
        record->key = key;
        record->data = data;
        record->isoccupied = true;
    }
    else {
        if (hash->isequal(record->key, key))
            return false;
        new = malloc(sizeof(*new));
        if (!new) {
            perror("Memory Allocation Error");
            exit(EXIT_FAILURE);
        }
        new->key = key;
        new->data = data;
        if (record->longint == true) {
            new->next = NULL;
            record->next = new;
        }
        else {
            for (iter = record->next; iter; iter = iter->next) {
                if (hash->isequal(iter->key, key))
                    return false;
            }
            new->next = record->next;
            record->next = new;
        }
    }
    hash->nitems++;
    return true;
}

void *hashlookup (hash_s *hash, void *key)
{
    hrecord_s *record;
    
    record = &hash->table[hash->hash(key)];
    if (!record->isoccupied)
        return NULL;
    if (hash->isequal(record->key, key))
        return record->data;
    else {
        while (record) {
            if (hash->isequal(record->key, key))
                return record->data;
            record = record->next;
        }
    }
    return NULL;
}

hashiterator_s *hashiterator_(hash_s *hash)
{
    hashiterator_s *iterator;
    
    iterator = malloc(sizeof(*iterator));
    if (!iterator) {
        perror("Memory Allocation Error");
        exit(EXIT_FAILURE);
    }
    iterator->hash = hash;
    iterator->curr = NULL;
    iterator->index = -1;
    return iterator;
}

hrecord_s *hashnext (hashiterator_s *iterator)
{
    int i;
    hash_s *hash;

    if (!iterator->curr || !iterator->curr->next || iterator->curr->longint == true) {
        hash = iterator->hash;
        for (i = iterator->index + 1; i < HTABLE_SIZE; i++) {
            if (hash->table[i].isoccupied) {
                iterator->index = i;
                iterator->curr = &hash->table[i];
                return iterator->curr;
            }
        }
    }
    else {
        iterator->curr = iterator->curr->next;
        return iterator->curr;
    }
    return NULL;
}

inline void hiterator_reset (hashiterator_s *iterator)
{
    iterator->index = -1;
    iterator->curr = NULL;
}