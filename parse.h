/*
 parse.h
 Author: Jonathan Hamm
 
 Description: 
    Library for parser generator. This reads in a specified Backus-Naur
    form and source file. The source file's syntax is checked in 
    conformance to the specified LL(1) grammar. 
 */

#ifndef PARSE_H_
#define PARSE_H_

#include "lex.h"
#include "general.h"

#define PDATABLE_SIZE 19

typedef struct parse_s parse_s;
typedef struct pda_s pda_s;
typedef struct production_s production_s;
typedef struct pnode_s pnode_s;
typedef struct parsetable_s parsetable_s;

struct parse_s
{
    pda_s *start;
    hash_s *phash;
    parsetable_s *parse_table;
};

struct pda_s
{
    token_s *nterm;
    uint16_t nproductions;
    production_s *productions;
    llist_s *firsts;
    llist_s *follows;
};

struct production_s
{
    pnode_s *start;
};

struct pnode_s
{
    token_s *token;
    pnode_s *next;
    pnode_s *prev;
};

struct parsetable_s
{
    uint16_t n_terminals;
    uint16_t n_nonterminals;
    token_s **terms;
    token_s **nterms;
    int32_t **table;
};

extern parse_s *build_parse (const char *file, lextok_s lextok);
extern pda_s *get_pda (parse_s *parser, u_char *name);
extern bool hash_pda (parse_s *parser, u_char *name, pda_s *pda);

#endif
