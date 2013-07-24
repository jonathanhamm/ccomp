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

typedef struct hash_s hash_s;
typedef struct hrecord_s hrecord_s;

struct hrecord_s
{
    void *key;
    void *data;
    union {
        bool isoccupied;
        hrecord_s *next;
    };
};

struct hash_s
{
    hash_f hash;
    isequal_f isequal;
    hrecord_s table[HTABLE_SIZE];
};

extern u_char *readfile (const char *file);

extern void free_llist (void *list);

extern void println (uint16_t no, u_char *buf);

extern hash_s *hash_(hash_f hashf, isequal_f isequalf);
extern bool hashinsert (hash_s *hash, void *key, void *data);
extern void *hashlookup (hash_s *hash, void *key);

#endif
