/*
 general.c
 Author: Jonathan Hamm
 
 Description: 
    Implementation of general functions and data structures used by the compiler.  
 */

#include "general.h"
#include <stdio.h>
#include <stdlib.h>

#if ((defined(__APPLE__) && defined(__MACH__)) || defined(__FreeBSD__))
    #include <malloc/malloc.h>
#endif

#define INITFBUF_SIZE 128

unsigned int safe_atoui (char *str)
{
    unsigned long i;
    
    i = strtoul(str, NULL, 10);
    if (i > UINT_MAX) {
        perror("Specified Attributed out of Range");
        exit(EXIT_FAILURE);
    }
    return (int)i;
}

char *readfile (const char *file)
{
    FILE *f;
    size_t bsize, nbytes;
    char *buf;
    
    bsize = INITFBUF_SIZE;
    f = fopen (file, "r");
    if (!f) {
        perror("File IO Error");
        printf("Could not read %s\n", file);
        assert(f);
        return NULL;
    }
    buf = malloc(INITFBUF_SIZE);
    if (!buf) {
        perror("Memory Allocation Error");
        fclose(f);
        return NULL;
    }
    for (nbytes = 0; (buf[nbytes] = fgetc(f)) != EOF; nbytes++) {
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
    if (!second)
        return first;
    for (head = first; first->next; first = first->next);
    first->next = second;
    return head;
}

llist_s *llcopy (llist_s *node)
{
    llist_s *copy;
    
    copy = malloc(sizeof(*copy));
    if (!copy) {
        perror("Memory Allocation Error");
        exit(EXIT_FAILURE);
    }
    copy->ptr = node->ptr;
    copy->next = NULL;
    return copy;
}

llist_s *llist_ (void *iptr)
{
    llist_s *list;
    
    list = malloc(sizeof(*list));
    if (!list) {
        perror("Memory Allocation Error");
        exit(EXIT_FAILURE);
    }
    list->ptr = iptr;
    list->next = NULL;
    return list;
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

void println (uint16_t no, char *buf)
{
    printf("%6d: ", no);
    do {
        if (*buf == EOF)
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
        if (record->isoccupied == true) {
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
    else if (record->isoccupied != true) {
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

    if (!iterator->curr || !iterator->curr->next || iterator->curr->isoccupied == true) {
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

bool is_allocated (const void *ptr)
{
    if (!ptr)
        return false;
#if ((defined(__APPLE__) && defined(__MACH__)) || defined(__FreeBSD__)) 
    if (malloc_zone_from_ptr(ptr))
        return true;
    return false;
#else
    return false;
#endif
}