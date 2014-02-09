/*
 general.c
 Author: Jonathan Hamm
 
 Description: 
    Implementation of general functions and data structures used by the compiler.  
 */

#include "general.h"
#include "lex.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <math.h>
#include <ctype.h>

#if ((defined(__APPLE__) && defined(__MACH__)) || defined(__FreeBSD__))
    #include <malloc/malloc.h>
#endif

#define INITLINETABLE_SIZE 64
#define INITFBUF_SIZE 128

static void printline(char *buf, FILE *stream);
static void llpush_(llist_s **list, llist_s *node);
static bool default_eq(void *k1, void *k2);
static int ndouble_digits(double val);

long safe_atol (char *str)
{
    long i;
    
    i = strtol(str, NULL, 10);
    if (errno) {
        perror("Error parsing number");
        exit(EXIT_FAILURE);
    }
    return i;
}

double safe_atod (char *str)
{
    double d;
    
    d = strtod(str, NULL);
    if(d == HUGE_VAL || d == HUGE_VALF || d == HUGE_VALL || errno) {
        perror("Error parsing number");
        exit(EXIT_FAILURE);
    }
    return d;
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
    llpush_(list, node);
}

void llpush_(llist_s **list, llist_s *node)
{
    node->next = *list;
    *list = node;
}

bool default_eq(void *k1, void *k2)
{
    return k1 == k2;
}


llist_s *llpop (llist_s **list)
{
    llist_s *backup;
    
    if(*list) {
        backup = *list;
        *list = (*list)->next;
        return backup;
    }
    return NULL;
}

void *llremove(llist_s **list, void *key)
{
    return llremove_(list, default_eq, key);
}

void *llremove_(llist_s **list, isequal_f eq, void *k)
{
    void *r = NULL;
    llist_s *l, *ll;
    
    l = *list;
    if(!l)
        return NULL;
    
    if(eq(l->ptr, k)) {
        
        ll = llpop(list);
        r = ll->ptr;
        free(ll);
    }
    else {
        while(l) {
            if(eq(l->ptr, k)) {
                r = l->ptr;
                ll->next = l->next;
                free(l);
                break;
            }
            ll = l;
            l = l->next;
        }
    }
    return r;
}

void llreverse(llist_s **list)
{
    llist_s *l = NULL;
    
    while (*list)
        llpush_(&l, llpop(list));
    *list = l;
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

size_t nlstrlen(const char *str)
{
    const char *ptr = str;
    
    while (*ptr++ != '\n');
    return ptr - str;
}

void println (unsigned no, char *buf, void *stream)
{
    fprintf(stream, "%6d: ", no);
    printline(buf, stream);
}

void printline(char *buf, FILE *stream)
{
    do {
        if (*buf == EOF) {
            fputc('\n', stream);
            return;
        }
        fputc(*buf, stream);
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
    hash->indices |= (1llu << (entry - hash->table));
    hash->nitems++;
    return true;
}

void hashinsert_ (hash_s *hash, void *key, void *data)
{
    hrecord_s **entry, *new, *iter;
    
    entry = &hash->table[hash->hash(key)];
    if (*entry) {
        for (iter = *entry; iter; iter = iter->next) {
            if (hash->isequal(iter->key, key)) {
                iter->data = data;
                return;
            }
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
    hash->indices |= (1llu << (entry - hash->table));
    hash->nitems++;
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

bool str_isequalf(void *key1, void *key2)
{
    int i;
    
    for (i = 0; i < (MAX_LEXLEN + 1) / sizeof(uint64_t); i++) {
        if (((uint64_t *)key1)[i] != ((uint64_t *)key2)[i])
            return false;
    }
    return true;
}

void free_hash(hash_s *hash)
{
    hrecord_s *iter, *b;
    uint64_t i, indices = hash->indices;
    
    for(i = ffsl(indices); i; indices &= ~(1llu << i), i = ffsl(indices)) {
        i--;
        for(iter = hash->table[i]; iter; iter = b) {
            b = iter->next;
            free(iter);
        }
    }
    free(hash);
}

void print_hash(hash_s *hash, void (*callback)(void *, void *))
{
    hrecord_s *iter;
    uint64_t i, indices = hash->indices;

    for(i = ffsl(indices); i; indices &= ~(1llu << i), i = ffsl(indices)) {
        i--;
        for(iter = hash->table[i]; iter; iter = iter->next)
            callback(iter->key, iter->data);
    }
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
    
    assert(lineno <= linelist->nlines);
    n = llist_(message);
    list = linelist->table[--lineno].tail;
    if (!list)
        linelist->table[lineno].errors = n;
    else
        list->next = n;
    linelist->table[lineno].tail = n;
}

bool check_listing(linetable_s *linelist, unsigned lineno, char *str)
{
    llist_s *list;
    
    if(!(lineno <= linelist->nlines))
        assert(false);
    list = linelist->table[--lineno].errors;
    while(list) {
        if(!strcmp(list->ptr, str))
            return true;
        list = list->next;
    }
    return false;
}

void print_listing(linetable_s *table, void *stream)
{
    unsigned i;
    llist_s *errors;
    
    for (i = 0; i < table->nlines; i++) {
        println(i+1, table->table[i].line, stream);
        for (errors = table->table[i].errors; errors; errors = errors->next) {
            printline(errors->ptr, stream);
        }
    }
}

void print_listing_nonum(linetable_s *table, void *stream)
{
    unsigned i;
    
    for(i = 0; i < table->nlines; i++)
        fprintf((FILE *)stream, "%s\n", table->table[i].line);
}


void free_listing(linetable_s *table)
{
    unsigned i;
    llist_s *errors, *backup;
    
    for (i = 0; i < table->nlines; i++) {
        for (errors = table->table[i].errors; errors; errors = backup) {
            backup = errors->next;
            free(errors->ptr);
            free(errors);
        }
    }
    free(table);
}

void safe_addstring(char **buf, char *str)
{
    char *base;
    size_t currlen, newlen;
    
    int i;
    
    printf("last called addstring\n");

    newlen = strlen(str)+1;
    if(*buf && strlen(*buf))
    for(i = 0; i < strlen(*buf)-1; i++) {
        if(((*buf)[i] >= 127 || (*buf)[i] < '\t' || (*buf)[i] == '@')) {
            printf("%u %u", (*buf)[i], '_');
            asm("hlt");
        }
    }

    if(!*buf) {
        currlen = 0;
        *buf = malloc(newlen);
        if(!*buf){
            perror("Memory Allocation Error");
            exit(EXIT_FAILURE);
        }
        base = *buf;
    }
    else {
        currlen = strlen(*buf)+1;
        newlen += currlen-1;
        *buf = realloc(*buf, newlen);
        if(!*buf){
            perror("Memory Allocation Error");
            exit(EXIT_FAILURE);
        }
        base = &(*buf)[currlen-1];
    }
    sprintf(base, "%s", str);
    for(i = 0; i < newlen-1; i++) {
        if(((*buf)[i] >= 127 || (*buf)[i] < '\t' || (*buf)[i] == '@')) {
            printf("%u %u", (*buf)[i], '_');
            asm("hlt");
        }
    }

    
}

void safe_addint(char **buf, long val)
{
    int i;
    char *base;
    size_t currlen, newlen;
    
    printf("last called addint\n");
    newlen = FS_INTWIDTH_DEC(val)+1;
    if(!*buf) {
        currlen = 0;
        *buf = malloc(newlen);
        if(!*buf){
            perror("Memory Allocation Error");
            exit(EXIT_FAILURE);
        }
        base = *buf;
    }
    else {
        currlen = strlen(*buf)+1;
        newlen += currlen-1;
        *buf = realloc(*buf, newlen);
        if(!*buf){
            perror("Memory Alocation Error");
            exit(EXIT_FAILURE);
        }
        base = &(*buf)[currlen-1];
    }
    sprintf(base, "%ld", val);
    for(i = 0; i < newlen-1; i++) {
        if(((*buf)[i] >= 127 || (*buf)[i] < '\t' || (*buf)[i] == '@')) {
            printf("%u %u", (*buf)[i], '_');
            asm("hlt");
        }
    }

}

void safe_adddouble(char **buf, double val)
{
    int i;
    char *base;
    size_t currlen, newlen;
    
    printf("last called adddouble\n");
    
    newlen = ndouble_digits(val);
    if(!*buf) {
        currlen = 0;
        *buf = malloc(newlen);
        if(!*buf){
            perror("Memory Allocation Error");
            exit(EXIT_FAILURE);
        }
        base = *buf;
    }
    else {
        currlen = strlen(*buf)+1;
        newlen += currlen-1;
        *buf = realloc(*buf, newlen);
        if(!*buf){
            perror("Memory Allocation Error");
            exit(EXIT_FAILURE);
        }
        base = &(*buf)[currlen-1];
    }
    sprintf(base, "%f", val);
    for(i = 0; i < newlen-1; i++) {
        if(((*buf)[i] >= 127 || (*buf)[i] < '\t' || (*buf)[i] == '@')) {
            printf("%u %u", (*buf)[i], '_');
            asm("hlt");
        }
    }

}

int ndouble_digits(double val)
{
    char buf[128];
    
    return sprintf(buf, "%f", val);
}

queue_s *queue_s_(void)
{
    queue_s *q;
    
    q = calloc(1, sizeof(*q));
    if(!q) {
        perror("Memory Allocation Error");
        exit(EXIT_FAILURE);
    }
    return q;
}

void enqueue(queue_s *q, void *ptr)
{
    llist_s *l;
    
    l = malloc(sizeof(*l));
    if(!l) {
        perror("Memory Allocation Error");
        exit(EXIT_FAILURE);
    }
    l->ptr = ptr;
    l->next = NULL;
    if(!q->head) {
        q->head = l;
        q->tail = l;
    }
    else {
        q->tail->next = l;
        q->tail = l;
    }
}

void *dequeu(queue_s *q)
{
    void *ptr;
    llist_s *l;
    
    if(!q->head)
        return NULL;
    l = q->head;
    q->head = q->head->next;
    ptr = l->ptr;
    free(l);
    return ptr;
}

void free_queue(queue_s *q)
{
    while(dequeu(q));
    free(q);
}

/* 
 Substandard function for debugging purposes.
 Mainly needed for an ugly hack for differentiating 
 strings of static duration and character buffers 
 stored in dynamically allocated memory. This is needed
 for freeing a particular linked list when parsing command
 line arguments.
 */
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