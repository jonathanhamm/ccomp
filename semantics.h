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
    char *id;
    hash_s *table;
    semantics_s *child;
};

extern semantics_s *semantics_s_(char *id);

extern lex_s *semant_init(void);
extern uint32_t cfg_annotate (token_s **tlist, char *buf, uint32_t *lineno, void *data);
extern void sem_start (token_s **curr);

#endif
