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
    parse_s *parse;
    mach_s *machs;
    hash_s *table;
};

extern semantics_s *semantics_s_(parse_s *parse, mach_s *machs);

extern lex_s *semant_init(void);
extern uint32_t cfg_annotate (token_s **tlist, char *buf, uint32_t *lineno, void *data);
extern semantics_s *sem_start (semantics_s *in, parse_s *parse, production_s *prod, mach_s *machs, pda_s *pda, pnode_s *par, pnode_s *pnode);

#endif
