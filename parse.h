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
    lex_s *lex;
    pda_s *start;
    hash_s *phash;
    linetable_s *listing;
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
    token_s *annot;
    struct semantics_s *s;
};

struct pnode_s
{
    token_s *token;
    token_s *matched;
    token_s *annotation;
    pnode_s *next;
    pnode_s *prev;
    struct semantics_s *s;
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
extern pda_s *get_pda (parse_s *parser, char *name);
extern bool hash_pda (parse_s *parser, char *name, pda_s *pda);
extern void parse (parse_s *parse, lextok_s lex);

#endif
