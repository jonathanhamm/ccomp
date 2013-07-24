#ifndef PARSE_H_
#define PARSE_H_

#include "lex.h"
#include "general.h"

#define PDATABLE_SIZE 19

typedef struct plrecord_s plrecord_s;
typedef struct pda_s pda_s;
typedef struct parse_s parse_s;
typedef struct pnode_s pnode_s;

struct plrecord_s
{
    u_char str[MAX_LEXLEN + 1];
    pda_s *pda;
    union {
        bool isoccupied;
        plrecord_s *next;
    };
};

struct pda_s
{
    token_s *nterm;
    pnode_s *start;
};

struct parse_s
{
    hash_s *phash;
};

struct pnode_s
{
    token_s *token;
    uint16_t nedges;
    pnode_s **edges;
};

extern parse_s *build_parse (const char *file);
extern pda_s *get_pda (parse_s *parser, u_char *name);
extern bool hash_pda (parse_s *parser, u_char *name, pda_s *pda);

#endif
