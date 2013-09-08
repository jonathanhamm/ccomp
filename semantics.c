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
    SEMTYPE_THEN,
    SEMTYPE_ELSE,
    SEMTYPE_FI,
    SEMTYPE_AND,
    SEMTYPE_OR,
    SEMTYPE_NOT,
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
    SEMTYPE_CROSS,
    SEMTYPE_ADDOP,
    SEMTYPE_MULOP
};

/*
<start>->
<statements>

<statements>->
<statement>
<statements>
|
ε

<statement>->
nonterm . id assignop <expression>
|
if <expression> then <statements> <else>
<else>->
else <statements> fi
|
fi

<expression> ->
<simple_expression> <expression'>
<expression'> ->
relop <simple_expression>
|
ε

<simple_expression> ->
<sign> <term> <simple_expression'>
|
<term> <simple_expression'>

<simple_expression'> ->
addop <term> <simple_expression'>
|
ε

<term> ->
<factor> <term'>

<term'> ->
mulop <factor> <term'>
|
ε

<factor> ->
id <factor'>
|
num
|
not <factor>

<factor'> ->
[ <expression> ]
|
ε

<sign> ->
\+ | \-*/

static void sem_start (token_s **curr);
static void sem_statements (token_s **curr);
static void sem_statement (token_s **curr);
static void sem_else (token_s **curr);
static void sem_expression (token_s **curr);
static void sem_expression_ (token_s **curr);
static void sem_simple_expression (token_s **curr);
static void sem_simple_expression_ (token_s **curr);
static void sem_term (token_s **curr);
static void sem_term_ (token_s **curr);
static void sem_factor (token_s **curr);
static void sem_factor_ (token_s **curr);
static void sem_sign (token_s **curr);


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

void sem_start (token_s **curr)
{
    
}

void sem_statements (token_s **curr)
{
    
}

void sem_statement (token_s **curr)
{
    
}

void sem_else (token_s **curr)
{
    
}

void sem_expression (token_s **curr)
{
    
}

void sem_expression_ (token_s **curr)
{
    
}

void sem_simple_expression (token_s **curr)
{
    
}

void sem_simple_expression_ (token_s **curr)
{
    
}

void sem_term (token_s **curr)
{
    
}

void sem_term_ (token_s **curr)
{
    
}

void sem_factor (token_s **curr)
{
    
}

void sem_factor_ (token_s **curr)
{
    
}

void sem_sign (token_s **curr)
{
    
}