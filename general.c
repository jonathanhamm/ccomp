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
        if (record->next == (hrecord_s *)true) {
            new->next = NULL;
            record->next = new;
        }
        else {
            for (iter = record->next; iter; iter = iter->next) {
                if (hash->isequal(record->key, key))
                    return false;
            }
            new->next = record->next;
            record->next = new;
        }
    }
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