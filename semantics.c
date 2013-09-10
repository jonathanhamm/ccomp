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
#define MACHID_START            35

#define SEMSIGN_POS 0
#define SEMSIGN_NEG 1

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
    /*gap for id types*/
    SEMTYPE_ID = MACHID_START,
    SEMTYPE_NONTERM,
    SEMTYPE_NUM,
    SEMTYPE_RELOP,
    SEMTYPE_ASSIGNOP,
    SEMTYPE_CROSS,
    SEMTYPE_ADDOP,
    SEMTYPE_MULOP,
};

typedef struct att_s att_s;

/* Return Structures */
typedef struct sem_statements_s sem_statements_s;
typedef struct sem_statement_s sem_statement_s;
typedef struct sem_else_s sem_else_s;
typedef struct sem_expression_s sem_expression_s;
typedef struct sem_expression__s sem_expression__s;
typedef struct sem_simple_expression_s sem_simple_expression_s;
typedef struct sem_simple_expression__s sem_simple_expression__s;
typedef struct sem_term_s sem_term_s;
typedef struct sem_term__s sem_term__s;
typedef struct sem_factor_s sem_factor_s;
typedef struct sem_factor__s sem_factor__s;
typedef struct sem_idsuffix_s sem_idsuffix_s;
typedef struct sem_dot_s sem_dot_s;
typedef struct sem_sign_s sem_sign_s;

struct sem_statements_s
{
    
};

struct sem_statement_s
{
    
};

struct sem_else_s
{
    
};

struct sem_expression_s
{
    bool type;
    union {
        double real_;
        int int_;
    };
};

struct sem_expression__s
{
    
};

struct sem_simple_expression_s
{
    
};

struct sem_simple_expression__s
{
    
};

struct sem_term_s
{
    
};

struct sem_term__s
{
    
};

struct sem_factor_s
{
    
};

struct sem_factor__s
{
    uint16_t index;;
};

struct sem_idsuffix_s
{
    
};

struct sem_dot_s
{
    char *id;
};

struct sem_sign_s
{
    unsigned value;
};

struct att_s
{
    unsigned tid;
    union {
        void *pdata;
        intptr_t idata;
    };
};

static sem_statements_s sem_statements (token_s **curr);
static sem_statement_s sem_statement (token_s **curr);
static sem_else_s sem_else (token_s **curr);
static sem_expression_s sem_expression (token_s **curr);
static sem_expression__s sem_expression_ (token_s **curr);
static sem_simple_expression_s sem_simple_expression (token_s **curr);
static sem_simple_expression__s sem_simple_expression_ (token_s **curr);
static sem_term_s sem_term (token_s **curr);
static sem_term__s sem_term_ (token_s **curr);
static sem_factor_s sem_factor (token_s **curr);
static sem_factor__s sem_factor_ (token_s **curr);
static sem_idsuffix_s sem_idsuffix (token_s **curr);
static sem_dot_s sem_dot (token_s **curr);
static sem_sign_s sem_sign (token_s **curr);
static bool sem_match (token_s **curr, int type);

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
    token_s *iter;
    static unsigned anlineno = 0;
        
    for (i = 1; buf[i] != '}'; i++);
    buf[i] = EOF;
        
    addtok(tlist, "generated {", *lineno, LEXTYPE_ANNOTATE, LEXATTR_DEFAULT);
    ltok = lexf(data, &buf[1], anlineno, true);
    anlineno = ltok.lines;
    *lineno += ltok.lines;
    (*tlist)->next = ltok.tokens;
    ltok.tokens->prev = *tlist;
    while ((*tlist)->next)
        *tlist = (*tlist)->next;
    return i;
}

void sem_start (token_s **curr)
{
    sem_statements(curr);
    sem_match(curr, LEXTYPE_EOF);
}

sem_statements_s sem_statements (token_s **curr)
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
            //exit(EXIT_FAILURE);
    }

}

sem_statement_s sem_statement (token_s **curr)
{
    switch((*curr)->type.val) {
        case SEMTYPE_NONTERM:
            *curr = (*curr)->next;
            printf("matching dot type\n");
            sem_match(curr, SEMTYPE_DOT);
            printf("matching id type %d %d\n", SEMTYPE_ID, (*curr)->type.val);
            sem_match(curr, SEMTYPE_ID);
            printf("matching assignop type\n");
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

sem_else_s sem_else (token_s **curr)
{
    switch((*curr)->type.val) {
        case SEMTYPE_ELSE:
            *curr = (*curr)->next;
            sem_statements(curr);
            sem_match(curr, SEMTYPE_FI);
            break;
        case SEMTYPE_FI:
            *curr = (*curr)->next;
            break;
        default:
            printf("Syntax Error at line %d: Expected else or fi but got %s\n", (*curr)->lineno, (*curr)->lexeme);
            break;
    }
}

sem_expression_s sem_expression (token_s **curr)
{
    sem_simple_expression(curr);
    sem_expression_(curr);
}

sem_expression__s sem_expression_ (token_s **curr)
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
        case SEMTYPE_CLOSEPAREN:
        case LEXTYPE_EOF:
            break;
        default:
            printf("Syntax Error at line %d: Expected relop ] fi else then if nonterm or $ but got %s\n", (*curr)->lineno, (*curr)->lexeme);
            asm("hlt");
            //exit(EXIT_FAILURE);
            
    }
}

sem_simple_expression_s sem_simple_expression (token_s **curr)
{
    switch((*curr)->type.val) {
        case SEMTYPE_ADDOP:
            sem_sign(curr);
            sem_simple_expression(curr);
            break;
        case SEMTYPE_NOT:
        case SEMTYPE_NUM:
        case SEMTYPE_ID:
        case SEMTYPE_NONTERM:
        case SEMTYPE_OPENPAREN:
            sem_term(curr);
            sem_simple_expression_(curr);
            break;
        default:
            printf("Syntax Error at line %d: Expected + - not number or identifier but got %s\n", (*curr)->lineno, (*curr)->lexeme);
            //exit(EXIT_FAILURE);
    }
}

sem_simple_expression__s sem_simple_expression_ (token_s **curr)
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
        case SEMTYPE_CLOSEPAREN:
        case LEXTYPE_EOF:
            break;
        default:
            printf("Syntax Error at line %d: Expected + - ] = < > <> <= >= fi else then if nonterm or $ but got %s\n", (*curr)->lineno, (*curr)->lexeme);
            //exit(EXIT_FAILURE);
    }
}

sem_term_s sem_term (token_s **curr)
{
    sem_factor(curr);
    sem_term_(curr);
}

sem_term__s sem_term_ (token_s **curr)
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
        case SEMTYPE_CLOSEPAREN:
        case LEXTYPE_EOF:
            break;
        default:
            printf("Syntax Error at line %d: Expected * / ] + - = < > <> <= >= fi else then if nonterm or $ but got %s\n", (*curr)->lineno, (*curr)->lexeme);
            //exit(EXIT_FAILURE);
    }
}

sem_factor_s sem_factor (token_s **curr)
{
    switch((*curr)->type.val) {
        case SEMTYPE_ID:
            *curr = (*curr)->next;
            sem_idsuffix(curr);
            break;
        case SEMTYPE_NONTERM:
            *curr = (*curr)->next;
            sem_idsuffix(curr);
            break;
        case SEMTYPE_NUM:
            *curr = (*curr)->next;
            break;
        case SEMTYPE_NOT:
            *curr = (*curr)->next;
            sem_factor(curr);
            break;
        case SEMTYPE_OPENPAREN:
            *curr = (*curr)->next;
            sem_expression(curr);
            sem_match(curr, SEMTYPE_CLOSEPAREN);
            break;
        default:
            printf("Syntax Error at line %d: Expected identifier nonterm number or not but got %s\n", (*curr)->lineno, (*curr)->lexeme);
            //exit(EXIT_FAILURE);
    }
}

sem_factor__s sem_factor_ (token_s **curr)
{
    sem_factor__s sem_factor_;
    
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
        case SEMTYPE_DOT:
        case SEMTYPE_CLOSEPAREN:
        case LEXTYPE_EOF:
            break;
        default:
            printf("Syntax Error at line %d: Expected [ ] * / + - = < > <> <= >= fi else then if nonterm or $ but got %s\n", (*curr)->lineno, (*curr)->lexeme);
            exit(EXIT_FAILURE);
    }
}

sem_idsuffix_s sem_idsuffix (token_s **curr)
{
    sem_idsuffix_s sem_idsuffix;
    
    sem_factor_(curr);
    sem_dot(curr);
    
    return sem_idsuffix;
}

sem_dot_s sem_dot (token_s **curr)
{
    sem_dot_s sem_dot;
    
    switch((*curr)->type.val) {
        case SEMTYPE_DOT:
            *curr = (*curr)->next;
            sem_dot.id = (*curr)->lexeme;
            sem_match(curr, SEMTYPE_ID);
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
        case SEMTYPE_CLOSEPAREN:
        case LEXTYPE_EOF:
            sem_dot.id = NULL;
            break;
        default:
            printf("Syntax Error at line %d: Expected . ] * / + - = < > <> <= >= fi else then if nonterm or $ but got %s\n", (*curr)->lineno, (*curr)->lexeme);
            exit(EXIT_FAILURE);
    }
    return sem_dot;
}

sem_sign_s sem_sign (token_s **curr)
{
    sem_sign_s sem_sign;
    
    if ((*curr)->type.val == SEMTYPE_ADDOP) {
        sem_sign.value = (*curr)->type.attribute;
        *curr = (*curr)->next;
        return sem_sign;
    }
    else {
        printf("Syntax Error at line %d: Expected + or - but got %s\n", (*curr)->lineno, (*curr)->lexeme);
        exit(EXIT_FAILURE);
    }
}

bool sem_match (token_s **curr, int type)
{
    if ((*curr)->type.val == type) {
        *curr = (*curr)->next;
        return true;
    }
    printf("Syntax Error at line %d: Got %s\n", (*curr)->lineno, (*curr)->lexeme);
    return false;
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