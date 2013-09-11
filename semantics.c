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

#define OPTYPE_NOP  -1
#define OPTYPE_MULT 0
#define OPTYPE_DIV  1
#define OPTYPE_ADD  2
#define OPTYPE_SUB  3

#define ATTYPE_NUMREAL  0
#define ATTYPE_NUMINT   1
#define ATTYPE_STR      2

#define ATTYPE_MULT 0
#define ATTYPE_DIV  1
#define ATTYPE_ADD  0
#define ATTYPE_SUB  1

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
typedef struct sem_type_s sem_type_s;
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

struct sem_type_s
{
    unsigned type;
    union {
        int int_;
        double real_;
        char *str;
    };
};

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
    sem_type_s value;
};

struct sem_expression__s
{
    
};

struct sem_simple_expression_s
{
    sem_type_s value;
};

struct sem_simple_expression__s
{
    unsigned op;
    sem_type_s value; 
};

struct sem_term_s
{
    sem_type_s value;
};

struct sem_term__s
{
    unsigned op;
    sem_type_s value;
};

struct sem_factor_s
{
    sem_type_s value;
};

struct sem_factor__s
{
    int index;
};

struct sem_dot_s
{
    char *id;
};

struct sem_idsuffix_s
{
    sem_factor__s factor_;
    sem_dot_s dot;
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

static void print_semtype(sem_type_s value);
static inline unsigned toaddop(unsigned val);
static inline unsigned tomulop(unsigned val);
static sem_type_s sem_op(sem_type_s v1, sem_type_s v2, int op);
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

void print_semtype(sem_type_s value)
{
    switch (value.type) {
        case ATTYPE_NUMINT:
            printf("value: %d\n", value.int_);
            break;
        case ATTYPE_NUMREAL:
            printf("value: %f\n", value.real_);
            break;
        case ATTYPE_STR:
            printf("value %s\n", value.str);
        default:
            /*illegal*/
            break;
    }
}

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

unsigned toaddop(unsigned val)
{
    if (val == ATTYPE_ADD)
        return OPTYPE_ADD;
    if (val == ATTYPE_SUB)
        return OPTYPE_SUB;
    perror("Op attribute mismatch");
    assert(false);
}

unsigned tomulop(unsigned val)
{
    if (val == ATTYPE_MULT)
        return OPTYPE_MULT;
    if (val == ATTYPE_DIV)
        return OPTYPE_DIV;
    perror("Op attribute mismatch");
    assert(false);
}

/* performs basic arithmetic operations with implicit type coercion */
sem_type_s sem_op(sem_type_s v1, sem_type_s v2, int op)
{
    sem_type_s result;
    
    switch(op) {
        case OPTYPE_MULT:
            if (v1.type == ATTYPE_STR || v2.type == ATTYPE_STR)
                printf("Type Error: String type incompatible with multiplication.\n");
            if (v1.type == ATTYPE_NUMINT && v2.type == ATTYPE_NUMINT) {
                result.type = ATTYPE_NUMINT;
                result.int_ = v1.int_ * v2.int_;
            }
            else if (v1.type == ATTYPE_NUMINT && v2.type == ATTYPE_NUMREAL) {
                result.type = ATTYPE_NUMREAL;
                result.real_ = (double)v1.int_ * v2.real_;
            }
            else if (v1.type == ATTYPE_NUMREAL && v2.type == ATTYPE_NUMINT) {
                result.type = ATTYPE_NUMREAL;
                result.real_ = v1.real_ * (double)v2.int_;
            }
            else {
                result.type = ATTYPE_NUMREAL;
                result.real_ = v1.real_ * v2.real_;
            }
            break;
        case OPTYPE_DIV:
            if (v1.type == ATTYPE_STR || v2.type == ATTYPE_STR)
                printf("Type Error: String type incompatible with division.\n");
            if (v1.type == ATTYPE_NUMINT && v2.type == ATTYPE_NUMINT) {
                result.type = ATTYPE_NUMREAL;
                result.real_ = (double)v1.int_ / (double)v2.int_;
            }
            else if (v1.type == ATTYPE_NUMINT && v2.type == ATTYPE_NUMREAL) {
                result.type = ATTYPE_NUMREAL;
                result.real_ = (double)v1.int_ / v2.real_;
            }
            else if (v1.type == ATTYPE_NUMREAL && v2.type == ATTYPE_NUMINT) {
                result.type = ATTYPE_NUMREAL;
                result.real_ = v1.real_ / (double)v2.int_;
            }
            else {
                result.type = ATTYPE_NUMREAL;
                result.real_ = v1.real_ / v2.real_;
            }
            break;
        case OPTYPE_ADD:
            if (v1.type == ATTYPE_STR || v2.type == ATTYPE_STR)
                printf("Type Error: Not yet implemented.\n");
            if (v1.type == ATTYPE_NUMINT && v2.type == ATTYPE_NUMINT) {
                result.type = ATTYPE_NUMINT;
                result.int_ = v1.int_ + v2.int_;
            }
            else if (v1.type == ATTYPE_NUMINT && v2.type == ATTYPE_NUMREAL) {
                result.type = ATTYPE_NUMREAL;
                result.real_ = (double)v1.int_ + v2.real_;
            }
            else if (v1.type == ATTYPE_NUMREAL && v2.type == ATTYPE_NUMINT) {
                result.type = ATTYPE_NUMREAL;
                result.real_ = v1.real_ + (double)v2.int_;
            }
            else {
                result.type = ATTYPE_NUMREAL;
                result.real_ = v1.real_ + v2.real_;
            }
            break;
        case OPTYPE_SUB:
            if (v1.type == ATTYPE_STR || v2.type == ATTYPE_STR)
                printf("Type Error: String type incompatible with subtraction.\n");
            if (v1.type == ATTYPE_NUMINT && v2.type == ATTYPE_NUMINT) {
                result.type = ATTYPE_NUMINT;
                result.int_ = v1.int_ - v2.int_;
            }
            else if (v1.type == ATTYPE_NUMINT && v2.type == ATTYPE_NUMREAL) {
                result.type = ATTYPE_NUMREAL;
                result.real_ = (double)v1.int_ - v2.real_;
            }
            else if (v1.type == ATTYPE_NUMREAL && v2.type == ATTYPE_NUMINT) {
                result.type = ATTYPE_NUMREAL;
                result.real_ = v1.real_ - (double)v2.int_;
            }
            else {
                result.type = ATTYPE_NUMREAL;
                result.real_ = v1.real_ - v2.real_;
            }
            break;
        case OPTYPE_NOP:
            result = v1;
            break;
        default:
            perror("Illegal State");
            assert(false);
            break;
    }
    return result;
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
            exit(EXIT_FAILURE);
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
    sem_simple_expression_s simple_expression;
    
    simple_expression = sem_simple_expression(curr);
    print_semtype(simple_expression.value);
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
            exit(EXIT_FAILURE);
            
    }
}

sem_simple_expression_s sem_simple_expression (token_s **curr)
{
    sem_sign_s sign;
    sem_simple_expression_s simple_expression;
    sem_term_s term;
    sem_simple_expression__s simple_expression_;
    
    switch((*curr)->type.val) {
        case SEMTYPE_ADDOP:
            sign = sem_sign(curr);
            simple_expression = sem_simple_expression(curr);
            if (sign.value == SEMSIGN_NEG) {
                if (simple_expression.value.type == ATTYPE_NUMINT)
                    simple_expression.value.int_ = -simple_expression.value.int_;
                else if (simple_expression.value.type == ATTYPE_NUMREAL)
                    simple_expression.value.real_ = - simple_expression.value.real_;
                else
                    printf("Type Error: Cannot negate id types\n");
            }
            break;
        case SEMTYPE_NOT:
        case SEMTYPE_NUM:
        case SEMTYPE_ID:
        case SEMTYPE_NONTERM:
        case SEMTYPE_OPENPAREN:
            term = sem_term(curr);
            simple_expression_ = sem_simple_expression_(curr);
            simple_expression.value = sem_op(term.value, simple_expression_.value, simple_expression_.op);
            break;
        default:
            printf("Syntax Error at line %d: Expected + - not number or identifier but got %s\n", (*curr)->lineno, (*curr)->lexeme);
            exit(EXIT_FAILURE);
    }
    return simple_expression;
}

sem_simple_expression__s sem_simple_expression_ (token_s **curr)
{
    sem_term_s term;
    sem_simple_expression__s simple_expression_, simple_expression_x;
    
    switch((*curr)->type.val) {
        case SEMTYPE_ADDOP:
            simple_expression_.op = toaddop((*curr)->type.attribute);
            *curr = (*curr)->next;
            term = sem_term(curr);
            simple_expression_x = sem_simple_expression_(curr);
            simple_expression_.value = sem_op(term.value, simple_expression_x.value, simple_expression_x.op);
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
            simple_expression_.op = OPTYPE_NOP;
            break;
        default:
            printf("Syntax Error at line %d: Expected + - ] = < > <> <= >= fi else then if nonterm or $ but got %s\n", (*curr)->lineno, (*curr)->lexeme);
            exit(EXIT_FAILURE);
    }
    return simple_expression_;
}

sem_term_s sem_term (token_s **curr)
{
    sem_term_s term;
    sem_factor_s factor;
    sem_term__s term_;
    
    factor = sem_factor(curr);
    term_ = sem_term_(curr);
    term.value = sem_op(factor.value, term_.value, term_.op);
    return term;
}

sem_term__s sem_term_ (token_s **curr)
{
    sem_factor_s factor;
    sem_term__s term_, term_x;
    
    switch((*curr)->type.val) {
        case SEMTYPE_MULOP:
            term_.op = tomulop((*curr)->type.attribute);
            *curr = (*curr)->next;
            factor = sem_factor(curr);
            term_x = sem_term_(curr);
            term_.value = sem_op(factor.value, term_x.value, term_x.op);
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
            term_.op = OPTYPE_NOP;
            break;
        default:
            printf("Syntax Error at line %d: Expected * / ] + - = < > <> <= >= fi else then if nonterm or $ but got %s\n", (*curr)->lineno, (*curr)->lexeme);
            exit(EXIT_FAILURE);
    }
    return term_;
}

sem_factor_s sem_factor (token_s **curr)
{
    char *id;
    sem_factor_s factor = {0};
    sem_idsuffix_s idsuffix;
    sem_expression_s expression;
    
    switch((*curr)->type.val) {
        case SEMTYPE_ID:
            id = (*curr)->lexeme;
            *curr = (*curr)->next;
            idsuffix = sem_idsuffix(curr);
            if (!idsuffix.dot.id || idsuffix.factor_.index < 0) {
                factor.value.type = ATTYPE_STR;
                factor.value.str = id;
            }
            break;
        case SEMTYPE_NONTERM:
            *curr = (*curr)->next;
            sem_idsuffix(curr);
            break;
        case SEMTYPE_NUM:
            if (!(*curr)->type.attribute) {
                factor.value.type = ATTYPE_NUMINT;
                factor.value.int_ = safe_atoui((*curr)->lexeme);
            }
            else {
                factor.value.type = ATTYPE_NUMREAL;
                factor.value.real_ = safe_atod((*curr)->lexeme);
            }
            *curr = (*curr)->next;
            break;
        case SEMTYPE_NOT:
            *curr = (*curr)->next;
            id = (*curr)->lexeme;
            factor = sem_factor(curr);
            switch(factor.value.type) {
                case ATTYPE_STR:
                    printf("Type Error: Cannot apply logical not to string type.");
                    break;
                case ATTYPE_NUMINT:
                    factor.value.int_ = !factor.value.int_;
                    break;
                case ATTYPE_NUMREAL:
                    factor.value.real_ = !factor.value.real_;
                    break;
                default:
                    perror("Illegal State");
                    assert(false);
                    break;
            }
            break;
        case SEMTYPE_OPENPAREN:
            *curr = (*curr)->next;
            expression = sem_expression(curr);
            sem_match(curr, SEMTYPE_CLOSEPAREN);
            factor.value = expression.value;
            break;
        default:
            printf("Syntax Error at line %d: Expected identifier nonterm number or not but got %s\n", (*curr)->lineno, (*curr)->lexeme);
            exit(EXIT_FAILURE);
    }
    return factor;
}

sem_factor__s sem_factor_ (token_s **curr)
{
    sem_factor__s factor_;
    sem_expression_s expression;
    
    switch((*curr)->type.val) {
        case SEMTYPE_OPENBRACKET:
            *curr = (*curr)->next;
            expression = sem_expression(curr);
            if (expression.value.type != ATTYPE_NUMINT)
                printf("Type Error: Index value must be an integer");
            factor_.index = expression.value.int_;
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
            factor_.index = -1;
            break;
        default:
            printf("Syntax Error at line %d: Expected [ ] * / + - = < > <> <= >= fi else then if nonterm or $ but got %s\n", (*curr)->lineno, (*curr)->lexeme);
            exit(EXIT_FAILURE);
    }
    return factor_;
}

sem_idsuffix_s sem_idsuffix (token_s **curr)
{
    sem_idsuffix_s idsuffix;
    
    idsuffix.factor_ = sem_factor_(curr);
    idsuffix.dot = sem_dot(curr);
    
    return idsuffix;
}

sem_dot_s sem_dot (token_s **curr)
{
    sem_dot_s dot;
    
    switch((*curr)->type.val) {
        case SEMTYPE_DOT:
            *curr = (*curr)->next;
            dot.id = (*curr)->lexeme;
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
            dot.id = NULL;
            break;
        default:
            printf("Syntax Error at line %d: Expected . ] * / + - = < > <> <= >= fi else then if nonterm or $ but got %s\n", (*curr)->lineno, (*curr)->lexeme);
            exit(EXIT_FAILURE);
    }
    return dot;
}

sem_sign_s sem_sign (token_s **curr)
{
    sem_sign_s sign;
    
    if ((*curr)->type.val == SEMTYPE_ADDOP) {
        sign.value = (*curr)->type.attribute;
        *curr = (*curr)->next;
        return sign;
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