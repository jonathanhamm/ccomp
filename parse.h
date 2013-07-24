#ifndef PARSE_H_
#define PARSE_H_

#include "lex.h"
#include "general.h"

#define PDATABLE_SIZE 19

typedef struct parse_s parse_s;
typedef struct pda_s pda_s;
typedef struct production_s production_s;
typedef struct pnode_s pnode_s;

struct parse_s
{
    pda_s *start;
    hash_s *phash;
};

struct pda_s
{
    token_s *nterm;
    uint16_t nproductions;
    production_s *productions;
};

struct production_s
{
    pnode_s *start;
};

struct pnode_s
{
    token_s *token;
    pnode_s *next;
};

extern parse_s *build_parse (const char *file);
extern pda_s *get_pda (parse_s *parser, u_char *name);
extern bool hash_pda (parse_s *parser, u_char *name, pda_s *pda);

#endif
