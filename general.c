/*
 general.c
 Author: Jonathan Hamm
 
 Description: 
    Implementation of general functions and data structures used by the compiler.  
 */

#include "general.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if ((defined(__APPLE__) && defined(__MACH__)) || defined(__FreeBSD__))
    #include <malloc/malloc.h>
#endif

#define INITLINETABLE_SIZE 64
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
        exit(EXIT_FAILURE);
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

void println (unsigned no, char *buf)
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
    hrecord_s **entry, *new, *iter;
    
    entry = &hash->table[hash->hash(key)];
    if (*entry) {
        for (iter = *entry; iter; iter = iter->next) {
            if (hash->isequal(iter->key, key))
                return false;
        }
    }
    new = malloc(sizeof(*new));
    if (!new) {
        perror("Memory Allocation Error");
        exit(EXIT_FAILURE);
    }
    new->key = key;
    new->data = data;
    new->next = *entry;
    *entry = new;
    hash->indices |= (1llu << entry - hash->table);
    hash->nitems++;
    return true;
}

void *hashlookup (hash_s *hash, void *key)
{
    hrecord_s **entry, *iter;
    
    entry = &hash->table[hash->hash(key)];
    if (*entry) {
        for (iter = *entry; iter; iter = iter->next) {
            if (hash->isequal(iter->key, key))
                return iter->data;
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
    iterator->state = hash->indices;
    return iterator;
}

hrecord_s *hashnext (hashiterator_s *iterator)
{
    uint64_t i;
    hrecord_s *ret;
    
    if (iterator->curr) {
        ret = iterator->curr;
        iterator->curr = iterator->curr->next;
        return ret;
    }
    i = ffsl(iterator->state);
    if (!i)
        return NULL;
    iterator->state &= ~(1llu << --i);
    ret = iterator->hash->table[i];
    iterator->curr = ret->next;
    return ret;
}

inline void hiterator_reset (hashiterator_s *iterator)
{
    iterator->state = iterator->hash->indices;
    iterator->curr = NULL;
}

uint16_t basic_hashf (void *key)
{
    return (unsigned long)key % HTABLE_SIZE;
}

bool basic_isequalf(void *key1, void *key2)
{
    return key1 == key2;
}

/*
 PJW hash function from textbook
 */
uint16_t pjw_hashf(void *key)
{
    char *str = key;
    uint32_t h = 0;
    uint32_t g;
    
    while (*str) {
        h = (h << 4) + *str++;
        if ((g = h & (uint32_t)0xf0000000) != 0)
            h = (h ^ (g >> 24)) ^ g;
    }
    
    return (uint16_t)(h % HTABLE_SIZE);
}

inline linetable_s *linetable_s_(void)
{
    linetable_s *table;
    
    table = malloc(sizeof(*table) + sizeof(ltablerec_s)*INITLINETABLE_SIZE);
    if (!table) {
        printf("Memory Allocaton Error");
        exit(EXIT_FAILURE);
    }
    table->nlines = 0;
    table->size = INITLINETABLE_SIZE;
    return table;
}

void addline(linetable_s **linelist_ptr, char *line)
{
    linetable_s *linelist = *linelist_ptr;
    
    if (linelist->nlines == linelist->size) {
        linelist->size *= 2;
        linelist = realloc(linelist, sizeof(*linelist) + sizeof(ltablerec_s)*linelist->size);
        if (!linelist) {
            perror("Memory Allocation Error");
            exit(EXIT_FAILURE);
        }
        *linelist_ptr = linelist;
    }
    linelist->table[linelist->nlines].line = line;
    linelist->table[linelist->nlines].errors = NULL;
    linelist->table[linelist->nlines].tail = NULL;
    linelist->nlines++;
}

void adderror(linetable_s *linelist, char *message, unsigned lineno)
{
    llist_s *n, *list;
    
    n = llist_(message);
    list = linelist->table[--lineno].tail;
    if (!list)
        linelist->table[lineno].errors = n;
    else
        list->next = n;
    linelist->table[lineno].tail = n;
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