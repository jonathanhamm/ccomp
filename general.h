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
#include <string.h>
#include <stdbool.h>
#include <stddef.h>

#define HTABLE_SIZE 53
#define FS_INTWIDTH_DEC(num) ((size_t)ceil(log10((num)+1)))

typedef unsigned long ulong_bool;

typedef uint16_t (*hash_f)(void *key);
typedef bool (*isequal_f)(void *key1, void *key2);

typedef struct llist_s llist_s;
typedef struct hash_s hash_s;
typedef struct hrecord_s hrecord_s;
typedef struct hashiterator_s hashiterator_s;

typedef struct ltablerec_s ltablerec_s;
typedef struct linetable_s linetable_s;
typedef struct sem_type_s sem_type_s;
typedef struct queue_s queue_s;

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

struct ltablerec_s
{
    char *line;
    llist_s *errors;
    llist_s *tail;
};

struct linetable_s
{
    unsigned nlines;
    unsigned size;
    ltablerec_s table[];
};

struct sem_type_s
{
    unsigned type;
    bool isvar;
    char *lexeme;
    union {
        long int_;
        double real_;
        char *str_;
        char *tuple;
        queue_s *q;
    };
    struct token_s *tok;
    long low, high;
    int mark;
};

struct queue_s
{
    llist_s *head;
    llist_s *tail;
};

enum atypes {
    ATTYPE_NULL,
    ATTYPE_NUMREAL,
    ATTYPE_NUMINT,
    ATTYPE_STR,
    ATTYPE_CODE,
    ATTYPE_RANGE,
    ATTYPE_ARRAY,
    ATTYPE_ID,
    ATTYPE_NOT_EVALUATED,
    ATTYPE_VOID,
    ATTYPE_ERROR,
    ATTYPE_ARGLIST_FORMAL,
    ATTYPE_ARGLIST_ACTUAL,
    ATTYPE_TEMP,
    ATTYPE_LABEL,
};

extern long safe_atol (char *str);
extern double safe_atod (char *str);
extern char *readfile(const char *file);

extern llist_s *llist_ (void *iptr);
extern void free_llist (void *list);

extern void println (unsigned no, char *buf, void *stream);

extern void llpush (llist_s **list, void *ptr);
extern llist_s *llpop (llist_s **list);
extern void *llremove(llist_s **list, void *key);
extern void *llremove_(llist_s **list, isequal_f eq, void *key);
extern void llreverse (llist_s **list);
extern bool llcontains (llist_s *list, void *ptr);
extern llist_s *llconcat (llist_s *first, llist_s *second);
extern llist_s *llcopy (llist_s *node);

extern size_t nlstrlen(const char *str);

extern hash_s *hash_(hash_f hashf, isequal_f isequalf);
extern bool hashinsert (hash_s *hash, void *key, void *data);
extern void hashinsert_ (hash_s *hash, void *key, void *data);
extern void *hashlookup (hash_s *hash, void *key);
extern hashiterator_s *hashiterator_(hash_s *hash);
extern hrecord_s *hashnext (hashiterator_s *iterator);
extern inline void hiterator_reset (hashiterator_s *iterator);
extern uint16_t basic_hashf(void *key);
extern bool basic_isequalf(void *key1, void *key2);
extern uint16_t pjw_hashf(void *key);
extern bool str_isequalf(void *key1, void *key2);
extern void free_hash(hash_s *hash);
extern void print_hash(hash_s *hash, void (*callback)(void *, void *));

extern inline linetable_s *linetable_s_(void);
extern void addline(linetable_s **linelist_ptr, char *line);
extern void adderror(linetable_s *listing, char *message, unsigned lineno);
extern bool check_listing(linetable_s *listing, unsigned lineno, char *str);

extern void print_listing(linetable_s *table, void *stream);
extern void print_listing_nonum(linetable_s *table, void *stream);
extern bool check_listing(linetable_s *table, unsigned lineno, char *str);
extern void free_listing(linetable_s *table);

extern void safe_addstring(char **buf, char *str);
extern void safe_addint(char **buf, long val);
extern void safe_adddouble(char **buf, double val);

extern queue_s *queue_s_(void);
extern void enqueue(queue_s *q, void *ptr);
extern void *dequeu(queue_s *q);
extern void free_queue(queue_s *q);

extern bool is_allocated(const void *ptr);

#endif
