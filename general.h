/*
 general.h
 Author: Jonathan Hamm
 
 Description:
    Libary of general functions and data structures used by the compiler.
 */

#ifndef GENERAL_H_
#define GENERAL_H_

//#define NDEBUG

#include <assert.h>
#include <stdint.h>
#include <limits.h>
#include <stdbool.h>

typedef unsigned long ulong_bool;

#define HTABLE_SIZE 53

typedef uint16_t (*hash_f)(void *key);
typedef bool (*isequal_f)(void *key1, void *key2);

typedef struct llist_s llist_s;
typedef struct hash_s hash_s;
typedef struct hrecord_s hrecord_s;
typedef struct hashiterator_s hashiterator_s;

typedef struct linenode_s linenode_s;
typedef struct linelist_s linelist_s;

struct llist_s
{
    llist_s *next;
    void *ptr;
};

struct hrecord_s
{
    void *key;
    void *data;
    hrecord_s *next;
};

struct hash_s
{
    hash_f hash;
    isequal_f isequal;
    uint16_t nitems;
    uint64_t indices;
    hrecord_s *table[HTABLE_SIZE];
};

struct hashiterator_s
{
    hash_s *hash;
    uint64_t state;
    hrecord_s *curr;
};

struct linenode_s
{
    char *line;
    llist_s *errors;
    linenode_s *next;
};

struct linelist_s
{
    linenode_s *list;
    hash_s *hash;
};

extern unsigned int safe_atoui (char *str);

extern char *readfile (const char *file);

extern llist_s *llist_ (void *iptr);
extern void free_llist (void *list);

extern void println (unsigned no, char *buf);

extern void llpush (llist_s **list, void *ptr);
extern llist_s *llpop (llist_s **list);
extern bool llcontains (llist_s *list, void *ptr);
extern llist_s *llconcat (llist_s *first, llist_s *second);
extern llist_s *llcopy (llist_s *node);


extern hash_s *hash_(hash_f hashf, isequal_f isequalf);
extern bool hashinsert (hash_s *hash, void *key, void *data);
extern void *hashlookup (hash_s *hash, void *key);
extern hashiterator_s *hashiterator_(hash_s *hash);
extern hrecord_s *hashnext (hashiterator_s *iterator);
extern inline void hiterator_reset (hashiterator_s *iterator);
extern uint16_t basic_hashf(void *key);
extern bool basic_isequalf(void *key1, void *key2);
extern uint16_t pjw_hashf(void *key);

extern bool is_allocated (const void *ptr);

#endif
