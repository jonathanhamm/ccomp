#ifndef GENERAL_H_
#define GENERAL_H_

#include <stdint.h>

#ifndef u_char
typedef unsigned char u_char;
#endif

#define STR_TO_LONG(str) (unsigned long)*(uint8_t *)str

#define HTABLE_SIZE 119
#define UEOF (u_char)EOF

typedef enum bool {false = 0, true = 1} bool;

typedef uint16_t (*hash_f)(void *key);
typedef bool (*isequal_f)(void *key1, void *key2);

typedef struct llist_s llist_s;

typedef struct hash_s hash_s;
typedef struct hrecord_s hrecord_s;
typedef struct hashiterator_s hashiterator_s;

struct llist_s
{
    llist_s *next;
    void *ptr;
};

struct hrecord_s
{
    void *key;
    void *data;
    union {
        bool isoccupied;
        unsigned long longint;
        hrecord_s *next;
    };
};

struct hash_s
{
    hash_f hash;
    isequal_f isequal;
    uint16_t nitems;
    hrecord_s table[HTABLE_SIZE];
};

struct hashiterator_s
{
    hash_s *hash;
    int index;
    hrecord_s *curr;
};

extern u_char *readfile (const char *file);

extern void free_llist (void *list);

extern void println (uint16_t no, u_char *buf);

extern void llpush (llist_s **list, void *ptr);
extern llist_s *llpop (llist_s **list);
extern bool llcontains (llist_s *list, void *ptr);
extern llist_s *llconcat (llist_s *first, llist_s *second);

extern hash_s *hash_(hash_f hashf, isequal_f isequalf);
extern bool hashinsert (hash_s *hash, void *key, void *data);
extern void *hashlookup (hash_s *hash, void *key);
extern hashiterator_s *hashiterator_(hash_s *hash);
extern hrecord_s *hashnext (hashiterator_s *iterator);
extern inline void hiterator_reset (hashiterator_s *iterator);

#endif
