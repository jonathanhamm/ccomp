/*
 lex.h
 Author: Jonathan Hamm
 
 Description:
 Library for semantics analysis. 
 
 */

#ifndef SEMANTICS_H_
#define SEMANTICS_H_

#include "general.h"
#include <stdint.h>

typedef struct semantics_s semantics_s;
typedef struct pna_s pna_s;

struct semantics_s
{
    pnode_s *n;
    parse_s *parse;
    mach_s *machs;
    hash_s *table;
};

struct pna_s
{
    int size;
    pnode_s *curr;
    pnode_s array[];
};

extern semantics_s *semantics_s_(parse_s *parse, mach_s *machs);
extern void free_sem(semantics_s *s);

extern lex_s *semant_init(void);
extern uint32_t cfg_annotate (token_s **tlist, char *buf, uint32_t *lineno, void *data);
//extern llist_s *sem_start (semantics_s *in, parse_s *parse, mach_s *machs, pda_s *pda, production_s *prod, pna_s *pn, pna_s *syn);

extern llist_s *sem_start(parse_s *parse, pda_s *pda, production_s *prod, pna_s *pn, semantics_s *in, semantics_s *syn);

extern semantics_s *get_il(llist_s *l, pnode_s *p);


#endif
