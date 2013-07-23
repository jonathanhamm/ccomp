#ifndef GENERAL_H_
#define GENERAL_H_

#include <stdint.h>

#ifndef u_char
typedef unsigned char u_char;
#endif

#define HTABLE_SIZE 119
#define UEOF (u_char)EOF

typedef enum bool {false = 0, true = 1} bool;

typedef struct hash_s hash_s;
typedef struct hrecord_s hrecord_s;

struct hrecord_s
{
    int key;
    u_char *data;
    union {
        bool isoccupied;
        hrecord_s *next;
    };
};

struct hash_s
{
    hrecord_s table[HTABLE_SIZE];
};

extern u_char *readfile (const char *file);

extern void free_llist (void *list);

extern void println (uint16_t no, u_char *buf);

extern inline hash_s *hash_(void);
extern bool hashinsert (hash_s *hash, int key, u_char *data);
extern u_char *hashlookup (hash_s *hash, int key);

#endif
