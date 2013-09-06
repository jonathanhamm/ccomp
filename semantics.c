/*
 parse.h
 Author: Jonathan Hamm
 
 Description:
 Implementation of semantics analysis.
 
*/

#include "lex.h"
#include "semantics.h"
#include <stdio.h>
#include <stdlib.h>

#define REGEX_DECORATIONS_FILE "regex_decorations"

lex_s *semant_init(void)
{
    return buildlex(REGEX_DECORATIONS_FILE);
}

uint32_t cfg_annotate (token_s **tlist, char *buf, uint32_t *lineno, void *data)
{
    uint32_t i;
    lextok_s ltok;
    static unsigned anlineno = 1;
    
    
    for (i = 1; buf[i] != '}'; i++);
    buf[i] = EOF;
    
    ltok = lexf(data, &buf[1], anlineno, true);
    anlineno = ltok.lines;
    *lineno += ltok.lines;
            
    print_listing(((lex_s *)data)->listing, stdout);
    puts("\n\n\n");
    return i;
}
