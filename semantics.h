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

struct semantics_s
{
    pda_s *pda;
    production_s *prod;
    pnode_s *pnode;
    mach_s *machs;
    hash_s *table;
    unsigned nchildren;
    semantics_s *parent;
};

extern semantics_s *semantics_s_(semantics_s **parent, mach_s *machs, pda_s *pda, production_s *prod, pnode_s *pnode);

extern lex_s *semant_init(void);
extern uint32_t cfg_annotate (token_s **tlist, char *buf, uint32_t *lineno, void *data);
extern semantics_s *sem_start (semantics_s **parent, token_s *curr, mach_s *machs, pda_s *pda, production_s *prod, pnode_s *pnode);

#endif
