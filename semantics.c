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
#define MACHID_START            36

#define ATT_TNUM    1
#define ATT_TSTR    2

enum semantic_types_ {
    SEMTYPE_IF = LEXID_START+1,
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
    SEMTYPE_ID,
    /*gap for id types*/
    SEMTYPE_NONTERM = MACHID_START,
    SEMTYPE_NUM,
    SEMTYPE_RELOP,
    SEMTYPE_ASSIGNOP,
    SEMTYPE_CROSS,
    SEMTYPE_ADDOP,
    SEMTYPE_MULOP,
};

typedef struct att_s att_s;

struct att_s
{
    unsigned tid;
    union {
        void *pdata;
        intptr_t idata;
    };
};

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
static void sem_match (token_s **curr, int type);

static att_s *att_s_ (void *data, unsigned tid);
static void attadd (semantics_s *s, char *id, void *data);

semantics_s *semantics_s_(char *id)
{
    semantics_s *s;
    
    s = malloc(sizeof(*s));
    if (!s) {
        perror("Memory Allocation Error");
        exit(EXIT_FAILURE);
    }
    s->id = id;
    s->table = hash_(pjw_hashf, str_isequalf);
    s->child = NULL;
    return s;
}

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
    printf("\n\ntokens %d:\n", SEMTYPE_NONTERM);
    for(iter =  ltok.tokens; iter; iter = iter->next)
        printf("%s %d\n", iter->lexeme, iter->type.val);
    
    return i;
}

void sem_start (token_s **curr)
{
    sem_statements(curr);
}

void sem_statements (token_s **curr)
{
    switch((*curr)->type.val) {
        case SEMTYPE_IF:
        case SEMTYPE_NONTERM:
            sem_statement(curr);
            sem_statements(curr);
        case SEMTYPE_FI:
        case SEMTYPE_ELSE:
        case LEXTYPE_EOF:
            break;
        default:
            printf("Syntax Error at line %d: Expected if nonterm fi else or $ but got %s\n", (*curr)->lineno, (*curr)->lexeme);
            exit(EXIT_FAILURE);
    }

}

void sem_statement (token_s **curr)
{
    switch((*curr)->type.val) {
        case SEMTYPE_NONTERM:
            *curr = (*curr)->next;
            sem_match(curr, SEMTYPE_DOT);
            sem_match(curr, SEMTYPE_ID);
            sem_match(curr, SEMTYPE_ASSIGNOP);
            sem_expression(curr);
            break;
        case SEMTYPE_IF:
            *curr = (*curr)->next;
            sem_expression(curr);
            sem_match(curr, SEMTYPE_THEN);
            sem_statements(curr);
            sem_else(curr);
            break;
        default:
            printf("Syntax Error at line %d: Expected nonterm or if but got %s", (*curr)->lineno, (*curr)->lexeme);
            break;
    }
}

void sem_else (token_s **curr)
{
    switch((*curr)->type.val) {
        case SEMTYPE_ELSE:
            *curr = (*curr)->next;
            sem_expression(curr);
            break;
        case SEMTYPE_FI:
            *curr = (*curr)->next;
            break;
        default:
            printf("Syntax Error at line %d: Expected else or fi but got %s\n", (*curr)->lineno, (*curr)->lexeme);
            break;
    }
}

void sem_expression (token_s **curr)
{
    switch((*curr)->type.val) {
        case SEMTYPE_ADDOP:
        case SEMTYPE_NOT:
        case SEMTYPE_NUM:
        case SEMTYPE_ID:
            sem_simple_expression(curr);
            sem_expression_(curr);
            break;
        default:
            printf("Syntax Error at line %d: Expected addop not num or id but got %s\n", (*curr)->lineno, (*curr)->lexeme);
            break;
    }
}

void sem_expression_ (token_s **curr)
{
    switch((*curr)->type.val) {
        case SEMTYPE_RELOP:
            *curr = (*curr)->next;
            sem_simple_expression(curr);
            break;
        case SEMTYPE_CLOSEBRACKET:
        case SEMTYPE_FI:
        case SEMTYPE_ELSE:
        case SEMTYPE_THEN:
        case SEMTYPE_IF:
        case SEMTYPE_NONTERM:
        case LEXTYPE_EOF:
            break;
        default:
            printf("Syntax Error at line %d: Expected relop ] fi else then if nonterm or $ but got %s\n", (*curr)->lineno, (*curr)->lexeme);
            exit(EXIT_FAILURE);
            
    }
}

void sem_simple_expression (token_s **curr)
{
    switch((*curr)->type.val) {
        case SEMTYPE_ADDOP:
            *curr = (*curr)->next;
            sem_simple_expression(curr);
            break;
        case SEMTYPE_NOT:
        case SEMTYPE_NUM:
        case SEMTYPE_ID:
            sem_term(curr);
            break;
        default:
            printf("Syntax Error at line %d: Expected + - not number or identifier but got %s\n", (*curr)->lineno, (*curr)->lexeme);
            exit(EXIT_FAILURE);
    }
}

void sem_simple_expression_ (token_s **curr)
{
    switch((*curr)->type.val) {
        case SEMTYPE_ADDOP:
            *curr = (*curr)->next;
            sem_term(curr);
            sem_simple_expression_(curr);
            break;
        case SEMTYPE_CLOSEBRACKET:
        case SEMTYPE_RELOP:
        case SEMTYPE_FI:
        case SEMTYPE_ELSE:
        case SEMTYPE_THEN:
        case SEMTYPE_IF:
        case SEMTYPE_NONTERM:
        case LEXTYPE_EOF:
            break;
        default:
            printf("Syntax Error at line %d: Expected + - ] = < > <> <= >= fi else then if nonterm or $ but got %s\n", (*curr)->lineno, (*curr)->lexeme);
            exit(EXIT_FAILURE);
    }
}

void sem_term (token_s **curr)
{
    sem_factor(curr);
    sem_term_(curr);
}

void sem_term_ (token_s **curr)
{
    switch((*curr)->type.val) {
        case SEMTYPE_MULOP:
            *curr = (*curr)->next;
            sem_factor(curr);
            sem_term_(curr);
            break;
        case SEMTYPE_CLOSEBRACKET:
        case SEMTYPE_ADDOP:
        case SEMTYPE_RELOP:
        case SEMTYPE_FI:
        case SEMTYPE_ELSE:
        case SEMTYPE_THEN:
        case SEMTYPE_IF:
        case SEMTYPE_NONTERM:
        case LEXTYPE_EOF:
            break;
        default:
            printf("Syntax Error at line %d: Expected * / ] + - = < > <> <= >= fi else then if nonterm or $ but got %s\n", (*curr)->lineno, (*curr)->lexeme);
            exit(EXIT_FAILURE);
    }
}

void sem_factor (token_s **curr)
{
    switch((*curr)->type.val) {
        case SEMTYPE_ID:
            *curr = (*curr)->next;
            sem_factor_(curr);
            break;
        case SEMTYPE_NUM:
            *curr = (*curr)->next;
            break;
        case SEMTYPE_NOT:
            *curr = (*curr)->next;
            sem_factor(curr);
            break;
        default:
            printf("Syntax Error at line %d: Expected identifier number or not but got %s\n", (*curr)->lineno, (*curr)->lexeme);
            exit(EXIT_FAILURE);
    }
}

void sem_factor_ (token_s **curr)
{
    switch((*curr)->type.val) {
        case SEMTYPE_OPENBRACKET:
            *curr = (*curr)->next;
            sem_expression(curr);
            sem_match(curr, SEMTYPE_CLOSEBRACKET);
            break;
        case SEMTYPE_CLOSEBRACKET:
        case SEMTYPE_MULOP:
        case SEMTYPE_ADDOP:
        case SEMTYPE_RELOP:
        case SEMTYPE_FI:
        case SEMTYPE_ELSE:
        case SEMTYPE_THEN:
        case SEMTYPE_IF:
        case SEMTYPE_NONTERM:
        case LEXTYPE_EOF:
            break;
        default:
            printf("Syntax Error at line %d: Expected [ ] * / + - = < > <> <= >= fi else then if nonterm or $ but got %s\n", (*curr)->lineno, (*curr)->lexeme);
            exit(EXIT_FAILURE);
    }
}

void sem_sign (token_s **curr)
{
    if ((*curr)->type.val == SEMTYPE_ADDOP) {
        *curr = (*curr)->next;
        
    }
    else {
        printf("Syntax Error at line %d: Expected + or - but got %s\n", (*curr)->lineno, (*curr)->lexeme);
        exit(EXIT_FAILURE);
    }
}

void sem_match (token_s **curr, int type)
{
    if ((*curr)->type.val == type)
        *curr = (*curr)->next;
    else {
        printf("Syntax Error at line %d: Got %s\n", (*curr)->lineno, (*curr)->lexeme);
        exit(EXIT_FAILURE);
    }
}

att_s *att_s_ (void *data, unsigned tid)
{
    att_s *att;
    
    att = malloc(sizeof(*att));
    if (!att) {
        perror("Memory Allocation Error");
        exit(EXIT_FAILURE);
    }
    att->tid = tid;
    att->pdata = data;
    return att;
}

void attadd (semantics_s *s, char *id, void *data)
{
    hashinsert_(s->table, id, data);
}