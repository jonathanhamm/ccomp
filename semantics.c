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
#define MACHID_START            5000

/*
 if
 else
 then
 and
 or
 \(
 \)
 .
 ,
 ;
 [
 ]
 
 */


enum semantic_types_ {
    SEMTYPE_IF = LEXTYPE_START,
    SEMTYPE_ELSE,
    SEMTYPE_AND,
    SEMTYPE_OR,
    SEMTYPE_OPENPAREN,
    SEMTYPE_CLOSEPAREN,
    SEMTYPE_DOT,
    SEMTYPE_COMMA,
    SEMTYPE_SEMICOLON,
    SEMTYPE_OPENBRACKET,
    SEMTYPE_CLOSEBRACKET,
    /*gap for id types*/
    SEMTYPE_NONTERM = 5000,
    SEMTYPE_NUM,
    SEMTYPE_RELOP,
    SEMTYPE_ASSIGNOP,
    SEMTYPE_CROSS
};

lex_s *semant_init(void)
{
    return buildlex(REGEX_DECORATIONS_FILE);
}

uint32_t cfg_annotate (token_s **tlist, char *buf, uint32_t *lineno, void *data)
{
    uint32_t i;
    lextok_s ltok;
    static unsigned anlineno = 0;
        
    for (i = 1; buf[i] != '}'; i++);
    buf[i] = EOF;
    
    ltok = lexf(data, &buf[1], anlineno, true);
    anlineno = ltok.lines;
    *lineno += ltok.lines;

    token_s *iter;
    puts("\n\ntokens:\n");
    for(iter =  ltok.tokens; iter; iter = iter->next)
        printf("%s %d\n", iter->lexeme, iter->type.val);
    
    return i;
}
