/*
 parse.h
 Author: Jonathan Hamm
 
 Description:
 Implementation of semantics analysis.
 
*/

#include "lex.h"
#include "parse.h"
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
#define OPTYPE_L    4
#define OPTYPE_G    5
#define OPTYPE_LE   6
#define OPTYPE_GE   7
#define OPTYPE_EQ   8
#define OPTYPE_NE   9

#define ATTYPE_NULL     0
#define ATTYPE_NUMREAL  1
#define ATTYPE_NUMINT   2
#define ATTYPE_STR      3
#define ATTYPE_CODE     4

#define ATTYPE_MULT 0
#define ATTYPE_DIV  1
#define ATTYPE_ADD  0
#define ATTYPE_SUB  1
#define ATTYPE_EQ   0
#define ATTYPE_NE   1
#define ATTYPE_L    2
#define ATTYPE_LE   3
#define ATTYPE_GE   4
#define ATTYPE_G    5

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
    SEMTYPE_CODE,
    SEMTYPE_CONCAT
};

typedef struct att_s att_s;

/* Return Structures */
typedef struct access_s access_s;
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
typedef struct sem_paramlist_s sem_paramlist_s;
typedef struct sem_paramlist__s sem_paramlist__s;
typedef struct sem_sign_s sem_sign_s;

struct access_s
{
    char *base;
    long offset;
    char *attribute;
};

struct sem_type_s
{
    unsigned type;
    char *lexeme;
    union {
        long int_;
        double real_;
        char *str_;
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
    unsigned op;
    sem_type_s value;
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
    access_s access;
};

struct sem_factor__s
{
    unsigned index;
};

struct sem_dot_s
{
    char *id;
};

struct sem_paramlist_s
{
    llist_s *pstack;
};

struct sem_paramlist__s
{
    llist_s *pstack;
};

struct sem_idsuffix_s
{
    sem_factor__s factor_;
    sem_dot_s dot;
};

struct sem_sign_s
{
    long value;
};

struct att_s
{
    unsigned tid;
    char *lexeme;
};

static sem_type_s sem_type_s_(token_s *token);
static void print_semtype(sem_type_s value);
static int test_semtype(sem_type_s value);
static inline unsigned toaddop(unsigned val);
static inline unsigned tomulop(unsigned val);
static inline unsigned torelop(unsigned val);
static pnode_s *getpnode_token(semantics_s *s, char *lexeme, unsigned index);
static pnode_s *getpnode_nterm(semantics_s *s, char *lexeme, unsigned index);
static sem_type_s sem_op(sem_type_s v1, sem_type_s v2, int op);
static sem_statements_s sem_statements (token_s **curr, semantics_s *s, pda_s *pda, bool evaluate);
static sem_statement_s sem_statement (token_s **curr, semantics_s *s, pda_s *pda, bool evaluate);
static sem_else_s sem_else (token_s **curr, semantics_s *s, pda_s *pda, bool evaluate);
static sem_expression_s sem_expression (token_s **curr, semantics_s *s, pda_s *pda);
static sem_expression__s sem_expression_ (token_s **curr, semantics_s *s, pda_s *pda);
static sem_simple_expression_s sem_simple_expression (token_s **curr, semantics_s *s, pda_s *pda);
static sem_simple_expression__s sem_simple_expression_ (token_s **curr, semantics_s *s, sem_type_s *accum, pda_s *pda);
static sem_term_s sem_term (token_s **curr, semantics_s *s, pda_s *pda);
static sem_term__s sem_term_ (token_s **curr, semantics_s *s, sem_type_s *accum, pda_s *pda);
static sem_factor_s sem_factor (token_s **curr, semantics_s *s, pda_s *pda);
static sem_factor__s sem_factor_ (token_s **curr, semantics_s *s);
static sem_idsuffix_s sem_idsuffix (token_s **curr, semantics_s *s, pda_s *pda);
static sem_dot_s sem_dot (token_s **curr, semantics_s *s);
static sem_paramlist_s sem_paramlist (token_s **curr, semantics_s *s, pda_s *pda);
static void sem_paramlist_ (token_s **curr, semantics_s *s, sem_paramlist_s *list, pda_s *pda);
static sem_sign_s sem_sign (token_s **curr);
static bool sem_match (token_s **curr, int type);

static sem_type_s *alloc_semt(sem_type_s value);
static att_s *att_s_ (void *data, unsigned tid);
static void setatt(semantics_s *s, char *id, sem_type_s *data);
static sem_type_s getatt(semantics_s *s, char *id);

sem_type_s sem_type_s_(token_s *token)
{
    sem_type_s s;
    
    s.lexeme = token->lexeme;
    if (token->stype) {
        if (!strcmp(token->stype, "integer")) {
            s.type = ATTYPE_NUMINT;
            s.int_ = safe_atol(token->lexeme);
            return s;
        }
        else if (!strcmp(token->stype, "real")) {
            s.type = ATTYPE_NUMREAL;
            s.real_ = safe_atod(token->lexeme);
            return s;
        }
    }
    s.type = ATTYPE_STR;
    s.str_ = token->lexeme;
    return s;
}

void print_semtype(sem_type_s value)
{
    return;
    switch (value.type) {
        case ATTYPE_NUMINT:
            printf("value: %ld\n", value.int_);
            break;
        case ATTYPE_NUMREAL:
            printf("value: %f\n", value.real_);
            break;
        case ATTYPE_STR:
            printf("value: %s\n", value.str_);
            break;
        case ATTYPE_NULL:
            printf("value: null\n");
            break;
        default:
            /*illegal*/
            break;
    }
}

int test_semtype(sem_type_s value)
{
    return value.int_ || value.real_ || value.str_;
}

semantics_s *semantics_s_(parse_s *parse, mach_s *machs, pda_s *pda, pnode_s *pnode)
{
    semantics_s *s;
    
    s = malloc(sizeof(*s));
    if (!s) {
        perror("Memory Allocation Error");
        exit(EXIT_FAILURE);
    }
    s->pass = false;
    s->parse = parse;
    s->machs = machs;
    s->pda = pda;
    s->pnode = pnode;
    s->table = hash_(pjw_hashf, str_isequalf);
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
        
    addtok(tlist, "generated {", *lineno, LEXTYPE_ANNOTATE, LEXATTR_DEFAULT, NULL);
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

unsigned torelop(unsigned val)
{
    switch(val) {
        case ATTYPE_EQ:
            return OPTYPE_EQ;
        case ATTYPE_NE:
            return OPTYPE_NE;
        case ATTYPE_L:
            return OPTYPE_L;
        case ATTYPE_LE:
            return OPTYPE_LE;
        case ATTYPE_GE:
            return OPTYPE_GE;
        case ATTYPE_G:
            return OPTYPE_G;
        default:
            perror("Illegal State");
            assert(false);
            break;
    }
}

pnode_s *getpnode_token(semantics_s *s, char *lexeme, unsigned index)
{
    unsigned i;
    long ltype = -1;
    pnode_s *iter;
    mach_s *miter;
    
    assert(s->prod);
    for (miter = s->machs; miter; miter = miter->next) {
        if (!ntstrcmp (miter->nterm->lexeme, lexeme)) {
            ltype = miter->nterm->type.val;
        }
    }
    if (ltype < 0) {
        printf("Error: Type %s not a token type.\n", lexeme);
        assert(false);
    }
    for (iter = s->prod->start, i = 1; iter; iter = iter->next) {
        printf("Attempting to fetch: %s %u %ld\n", lexeme, iter->token->type.val, ltype);
        if (iter->token->type.val == ltype) {
            if (i == index)
                return iter;
            i++;
        }
    }
    return NULL;
}

pnode_s *getpnode_nterm(semantics_s *s, char *lexeme, unsigned index)
{
    unsigned i;
    pnode_s *iter;
    
    assert(s->prod);
    for (iter = s->prod->start, i = 1; iter; iter = iter->next) {
        printf("%s %s\n", iter->token->lexeme, lexeme);
        if (!strcmp(iter->token->lexeme, lexeme)) {
            printf("passed: %u %u\n", i, index);
            if (i == index)
                return iter;
            i++;
        }
    }
    return NULL;
}

/* performs basic arithmetic operations with implicit type coercion */
sem_type_s sem_op(sem_type_s v1, sem_type_s v2, int op)
{
    sem_type_s result;
    
    switch(op) {
        case OPTYPE_MULT:
            printf("multiplying:\n");
            print_semtype(v1);
            print_semtype(v2);
            
            
            assert (v1.int_ != 28);
            if (v1.type == ATTYPE_STR || v2.type == ATTYPE_STR)
                printf("Type Error: String type incompatible with multiplication.\n");
            if (v1.type == ATTYPE_NUMINT && v2.type == ATTYPE_NUMINT) {
                result.type = ATTYPE_NUMINT;
                result.int_ = v1.int_ * v2.int_;
            }
            else if (v1.type == ATTYPE_NUMINT && v2.type == ATTYPE_NUMREAL) {
                result.type = ATTYPE_NUMREAL;
                result.real_ = v1.int_ * v2.real_;
            }
            else if (v1.type == ATTYPE_NUMREAL && v2.type == ATTYPE_NUMINT) {
                result.type = ATTYPE_NUMREAL;
                result.real_ = v1.real_ * v2.int_;
            }
            else {
                result.type = ATTYPE_NUMREAL;
                result.real_ = v1.real_ * v2.real_;
            }
            break;
        case OPTYPE_DIV:
            printf("dividing:\n");
            print_semtype(v1);
            print_semtype(v2);

            if (v1.type == ATTYPE_STR || v2.type == ATTYPE_STR)
                printf("Type Error: String type incompatible with division.\n");
            if (v1.type == ATTYPE_NUMINT && v2.type == ATTYPE_NUMINT) {
                result.type = ATTYPE_NUMREAL;
                result.real_ = v1.int_ / v2.int_;
            }
            else if (v1.type == ATTYPE_NUMINT && v2.type == ATTYPE_NUMREAL) {
                result.type = ATTYPE_NUMREAL;
                result.real_ = v1.int_ / v2.real_;
            }
            else if (v1.type == ATTYPE_NUMREAL && v2.type == ATTYPE_NUMINT) {
                result.type = ATTYPE_NUMREAL;
                result.real_ = v1.real_ / v2.int_;
            }
            else {
                result.type = ATTYPE_NUMREAL;
                result.real_ = v1.real_ / v2.real_;
            }
            break;
        case OPTYPE_ADD:
            printf("adding:\n");
            print_semtype(v1);
            print_semtype(v2);

            if (v1.type == ATTYPE_STR || v2.type == ATTYPE_STR)
                printf("Type Error: Not yet implemented.\n");
            if (v1.type == ATTYPE_NUMINT && v2.type == ATTYPE_NUMINT) {
                result.type = ATTYPE_NUMINT;
                result.int_ = v1.int_ + v2.int_;
            }
            else if (v1.type == ATTYPE_NUMINT && v2.type == ATTYPE_NUMREAL) {
                result.type = ATTYPE_NUMREAL;
                result.real_ = v1.int_ + v2.real_;
            }
            else if (v1.type == ATTYPE_NUMREAL && v2.type == ATTYPE_NUMINT) {
                result.type = ATTYPE_NUMREAL;
                result.real_ = v1.real_ + v2.int_;
            }
            else {
                result.type = ATTYPE_NUMREAL;
                result.real_ = v1.real_ + v2.real_;
            }
            break;
        case OPTYPE_SUB:
            printf("subtracting:\n");
            print_semtype(v1);
            print_semtype(v2);

            if (v1.type == ATTYPE_STR || v2.type == ATTYPE_STR)
                printf("Type Error: String type incompatible with subtraction.\n");
            if (v1.type == ATTYPE_NUMINT && v2.type == ATTYPE_NUMINT) {
                result.type = ATTYPE_NUMINT;
                result.int_ = v1.int_ - v2.int_;
            }
            else if (v1.type == ATTYPE_NUMINT && v2.type == ATTYPE_NUMREAL) {
                result.type = ATTYPE_NUMREAL;
                result.real_ = v1.int_ - v2.real_;
            }
            else if (v1.type == ATTYPE_NUMREAL && v2.type == ATTYPE_NUMINT) {
                result.type = ATTYPE_NUMREAL;
                result.real_ = v1.real_ - v2.int_;
            }
            else {
                result.type = ATTYPE_NUMREAL;
                result.real_ = v1.real_ - v2.real_;
            }
            break;
        case OPTYPE_L:
            if (v1.type == ATTYPE_STR || v2.type == ATTYPE_STR)
                printf("Type Error: String type incompatible with subtraction.\n");
            result.type = ATTYPE_NUMINT;
            if (v1.type == ATTYPE_NUMINT && v2.type == ATTYPE_NUMINT)
                result.int_ = v1.int_ < v2.int_;
            else if (v1.type == ATTYPE_NUMINT && v2.type == ATTYPE_NUMREAL)
                result.int_ = v1.int_ < v2.real_;
            else if (v1.type == ATTYPE_NUMREAL && v2.type == ATTYPE_NUMINT)
                result.int_ = v1.real_ < v2.int_;
            else
                result.int_ = v1.real_ < v2.real_;
            break;
        case OPTYPE_G:
            if (v1.type == ATTYPE_STR || v2.type == ATTYPE_STR)
                printf("Type Error: String type incompatible with subtraction.\n");
            result.type = ATTYPE_NUMINT;
            if (v1.type == ATTYPE_NUMINT && v2.type == ATTYPE_NUMINT)
                result.int_ = v1.int_ > v2.int_;
            else if (v1.type == ATTYPE_NUMINT && v2.type == ATTYPE_NUMREAL)
                result.int_ = v1.int_ > v2.real_;
            else if (v1.type == ATTYPE_NUMREAL && v2.type == ATTYPE_NUMINT)
                result.int_ = v1.real_ > v2.int_;
            else
                result.int_ = v1.real_ > v2.real_;
            break;
        case OPTYPE_LE:
            if (v1.type == ATTYPE_STR || v2.type == ATTYPE_STR)
                printf("Type Error: String type incompatible with subtraction.\n");
            result.type = ATTYPE_NUMINT;
            if (v1.type == ATTYPE_NUMINT && v2.type == ATTYPE_NUMINT)
                result.int_ = v1.int_ <= v2.int_;
            else if (v1.type == ATTYPE_NUMINT && v2.type == ATTYPE_NUMREAL)
                result.int_ = v1.int_ <= v2.real_;
            else if (v1.type == ATTYPE_NUMREAL && v2.type == ATTYPE_NUMINT)
                result.int_ = v1.real_ <= v2.int_;
            else
                result.int_ = v1.real_ <= v2.real_;
            break;
        case OPTYPE_GE:
            if (v1.type == ATTYPE_STR || v2.type == ATTYPE_STR)
                printf("Type Error: String type incompatible with subtraction.\n");
            result.type = ATTYPE_NUMINT;
            if (v1.type == ATTYPE_NUMINT && v2.type == ATTYPE_NUMINT)
                result.int_ = v1.int_ >= v2.int_;
            else if (v1.type == ATTYPE_NUMINT && v2.type == ATTYPE_NUMREAL)
                result.int_ = v1.int_ >= v2.real_;
            else if (v1.type == ATTYPE_NUMREAL && v2.type == ATTYPE_NUMINT)
                result.int_ = v1.real_ >= v2.int_;
            else
                result.int_ = v1.real_ >= v2.real_;
            break;
        case OPTYPE_EQ:
            if (v1.type == ATTYPE_STR || v2.type == ATTYPE_STR)
                printf("Type Error: String type incompatible with subtraction.\n");
            result.type = ATTYPE_NUMINT;
            if (v1.type == ATTYPE_NUMINT && v2.type == ATTYPE_NUMINT)
                result.int_ = v1.int_ == v2.int_;
            else if (v1.type == ATTYPE_NUMINT && v2.type == ATTYPE_NUMREAL)
                result.int_ = v1.int_ == v2.real_;
            else if (v1.type == ATTYPE_NUMREAL && v2.type == ATTYPE_NUMINT)
                result.int_ = v1.real_ == v2.int_;
            else
                result.int_ = v1.real_ == v2.real_;
            break;
        case OPTYPE_NE:
            if (v1.type == ATTYPE_STR || v2.type == ATTYPE_STR)
                printf("Type Error: String type incompatible with subtraction.\n");
            result.type = ATTYPE_NUMINT;
            if (v1.type == ATTYPE_NUMINT && v2.type == ATTYPE_NUMINT)
                result.int_ = v1.int_ == v2.int_;
            else if (v1.type == ATTYPE_NUMINT && v2.type == ATTYPE_NUMREAL)
                result.int_ = v1.int_ == v2.real_;
            else if (v1.type == ATTYPE_NUMREAL && v2.type == ATTYPE_NUMINT)
                result.int_ = v1.real_ == v2.int_;
            else
                result.int_ = v1.real_ == v2.real_;
            break;
        case OPTYPE_NOP:
            printf("nop'ing:\n");
            print_semtype(v1);
            print_semtype(v2);

            result = v1;
            break;
        default:
            perror("Illegal State");
            assert(false);
            break;
    }
    printf("result:\n");
    print_semtype(result);
    puts("\n");
    return result;
}

semantics_s *sem_start (semantics_s *inherit, parse_s *parse, production_s *prod, mach_s *machs, pda_s *pda, pnode_s *pnode)
{
    token_s *iter;
    
    assert(pnode && prod);
    if(!(iter = prod->annot))
        return NULL;
    if(!inherit) {
        inherit = semantics_s_(parse, machs, pda, pnode);
    }
    else
        inherit->prod = prod;
    pnode->s = inherit;
    inherit->prod = prod;
    sem_statements(&iter, inherit, pda, true);
    inherit->pass = true;
    assert(inherit);
    return inherit;
}

sem_statements_s sem_statements (token_s **curr, semantics_s *s, pda_s *pda, bool evaluate)
{
    switch((*curr)->type.val) {
        case SEMTYPE_IF:
        case SEMTYPE_NONTERM:
            sem_statement(curr, s, pda, evaluate);
            sem_statements(curr, s, pda, evaluate);
        case SEMTYPE_FI:
        case SEMTYPE_ELSE:
        case LEXTYPE_EOF:
            break;
        default:
            fprintf(stderr, "Syntax Error at line %d: Expected if nonterm fi else or $ but got %s\n", (*curr)->lineno, (*curr)->lexeme);
            assert(false);
            break;
    }

}

sem_statement_s sem_statement (token_s **curr, semantics_s *s, pda_s *pda, bool evaluate)
{
    pnode_s *p;
    char *att, *nterm;
    unsigned index;
    sem_expression_s expression;
    sem_idsuffix_s idsuffix;
    
    switch((*curr)->type.val) {
        case SEMTYPE_NONTERM:
            nterm = (*curr)->lexeme;
            *curr = (*curr)->next;
            idsuffix = sem_idsuffix(curr, s, pda);
            index = idsuffix.factor_.index;
            att = idsuffix.dot.id;
            printf("matching assignop type\n");
            sem_match(curr, SEMTYPE_ASSIGNOP);
            expression = sem_expression(curr, s, pda);
            if (evaluate) {
                if(!strcmp(pda->nterm->lexeme, nterm)) {
                    if (s->pass)
                        setatt(s, att, alloc_semt(expression.value));
                }
                else {
                    /* Time to set inherited attributes */
                    p = getpnode_nterm(s, nterm, idsuffix.factor_.index);
                    if(!p) {
                        fprintf(stderr, "Error: Nonterminal %s cannot be matched in this scope.\n", nterm);
                        assert(false);
                    }
                    setatt(p->s, att, alloc_semt(expression.value));
                }
            }
            break;
        case SEMTYPE_IF:
            *curr = (*curr)->next;
            expression = sem_expression(curr, s, pda);
            sem_match(curr, SEMTYPE_THEN);
            sem_statements(curr, s, pda, evaluate);
            sem_else(curr, s, pda, test_semtype(expression.value) && evaluate);
            break;
        default:
            fprintf(stderr, "Syntax Error at line %d: Expected nonterm or if but got %s", (*curr)->lineno, (*curr)->lexeme);
            assert(false);
            break;
    }
}

sem_else_s sem_else (token_s **curr, semantics_s *s, pda_s *pda, bool evaluate)
{
    switch((*curr)->type.val) {
        case SEMTYPE_ELSE:
            *curr = (*curr)->next;
            sem_statements(curr, s, pda, evaluate);
            sem_match(curr, SEMTYPE_FI);
            break;
        case SEMTYPE_FI:
            *curr = (*curr)->next;
            break;
        default:
            fprintf(stderr, "Syntax Error at line %d: Expected else or fi but got %s\n", (*curr)->lineno, (*curr)->lexeme);
            assert(false);
            break;
    }
}

sem_expression_s sem_expression (token_s **curr, semantics_s *s, pda_s *pda)
{
    sem_expression_s expression;
    sem_expression__s expression_;
    sem_simple_expression_s simple_expression;
    
    simple_expression = sem_simple_expression(curr, s, pda);
    expression_ = sem_expression_(curr, s, pda);
    expression.value = sem_op(simple_expression.value, expression_.value, expression_.op);
    return expression;
}

sem_expression__s sem_expression_ (token_s **curr, semantics_s *s, pda_s *pda)
{
    sem_expression__s expression_;
    
    switch((*curr)->type.val) {
        case SEMTYPE_RELOP:
            expression_.op = torelop((*curr)->type.attribute);
            *curr = (*curr)->next;
            expression_.value = sem_simple_expression(curr, s, pda).value;
            break;
        case SEMTYPE_COMMA:
        case SEMTYPE_FI:
        case SEMTYPE_ELSE:
        case SEMTYPE_THEN:
        case SEMTYPE_IF:
        case SEMTYPE_NONTERM:
        case SEMTYPE_CLOSEPAREN:
        case LEXTYPE_EOF:
            expression_.op = OPTYPE_NOP;
            break;
        default:
            fprintf(stderr, "Syntax Error at line %d: Expected relop ] fi else then if nonterm or $ but got %s\n", (*curr)->lineno, (*curr)->lexeme);
            assert(false);
            break;
            
    }
    return expression_;
}

sem_simple_expression_s sem_simple_expression (token_s **curr, semantics_s *s, pda_s *pda)
{
    sem_sign_s sign;
    sem_simple_expression_s simple_expression;
    sem_term_s term;
    sem_simple_expression__s simple_expression_;
    
    switch((*curr)->type.val) {
        case SEMTYPE_ADDOP:
            sign = sem_sign(curr);
            simple_expression = sem_simple_expression(curr, s, pda);
            if (sign.value == SEMSIGN_NEG) {
                if (simple_expression.value.type == ATTYPE_NUMINT)
                    simple_expression.value.int_ = -simple_expression.value.int_;
                else if (simple_expression.value.type == ATTYPE_NUMREAL)
                    simple_expression.value.real_ = -simple_expression.value.real_;
                else
                    perror("Type Error: Cannot negate id types\n");
            }
            break;
        case SEMTYPE_NOT:
        case SEMTYPE_NUM:
        case SEMTYPE_ID:
        case SEMTYPE_NONTERM:
        case SEMTYPE_OPENPAREN:
        case SEMTYPE_CODE:
            term = sem_term(curr, s, pda);
            simple_expression_ = sem_simple_expression_(curr, s, &term.value, pda);
            simple_expression.value = term.value;
            break;
        default:
            fprintf(stderr, "Syntax Error at line %d: Expected + - not number or identifier but got %s\n", (*curr)->lineno, (*curr)->lexeme);
            assert(false);
            break;
    }
    return simple_expression;
}

sem_simple_expression__s sem_simple_expression_ (token_s **curr, semantics_s *s, sem_type_s *accum, pda_s *pda)
{
    unsigned op;
    sem_term_s term;
    sem_simple_expression__s simple_expression_, simple_expression__;
    
    switch((*curr)->type.val) {
        case SEMTYPE_ADDOP:
            op = toaddop((*curr)->type.attribute);
            *curr = (*curr)->next;
            term = sem_term(curr, s, pda);
            *accum = sem_op(*accum, term.value, op);
            simple_expression__ = sem_simple_expression_(curr, s, accum, pda);
            break;
        case SEMTYPE_COMMA:
        case SEMTYPE_RELOP:
        case SEMTYPE_FI:
        case SEMTYPE_ELSE:
        case SEMTYPE_THEN:
        case SEMTYPE_IF:
        case SEMTYPE_NONTERM:
        case SEMTYPE_CLOSEPAREN:
        case LEXTYPE_EOF:
            simple_expression_.op = OPTYPE_NOP;
            simple_expression_.value.type = ATTYPE_NULL;
            break;
        default:
            fprintf(stderr, "Syntax Error at line %d: Expected + - ] = < > <> <= >= fi else then if nonterm or $ but got %s\n", (*curr)->lineno, (*curr)->lexeme);
            assert(false);
            break;
    }
    return simple_expression_;
}

sem_term_s sem_term (token_s **curr, semantics_s *s, pda_s *pda)
{
    sem_term_s term;
    sem_factor_s factor;
    sem_term__s term_;
    
    factor = sem_factor(curr, s, pda);
    term_ = sem_term_(curr, s, &factor.value, pda);
    term.value = factor.value;
    //term.value = sem_op(factor.value, term_.value, term_.op);
    return term;
}

sem_term__s sem_term_ (token_s **curr, semantics_s *s, sem_type_s *accum, pda_s *pda)
{
    unsigned op;
    sem_factor_s factor;
    sem_term__s term_, term__;
    
    switch((*curr)->type.val) {
        case SEMTYPE_MULOP:
            op = tomulop((*curr)->type.attribute);
            *curr = (*curr)->next;
            factor = sem_factor(curr, s, pda);
            *accum = sem_op(*accum, factor.value, op);
            term__ = sem_term_(curr, s, accum, pda);
            break;
        case SEMTYPE_COMMA:
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
            fprintf(stderr, "Syntax Error at line %d: Expected * / ] + - = < > <> <= >= fi else then if nonterm or $ but got %s\n", (*curr)->lineno, (*curr)->lexeme);
            assert(false);
            break;
    }
    return term_;
}

sem_factor_s sem_factor (token_s **curr, semantics_s *s, pda_s *pda)
{
    token_s *id;
    pnode_s *pnode;
    sem_type_s value;
    sem_factor_s factor = {0};
    sem_idsuffix_s idsuffix;
    sem_expression_s expression;
    
    switch((*curr)->type.val) {
        case SEMTYPE_ID:
            id = *curr;
            factor.value.type = ATTYPE_STR;
            factor.value.str_ = id->lexeme;
            *curr = (*curr)->next;
            idsuffix = sem_idsuffix(curr, s, pda);
            //attadd (semantics_s *s, char *id, sem_type_s *data)
            if (idsuffix.dot.id) {
                if (!strcmp(idsuffix.dot.id, "val") && s->pass) {
                    printf("%s\n", id->lexeme);
                    pnode = getpnode_token(s, id->lexeme, idsuffix.factor_.index);
                    
                    factor.value = sem_type_s_(pnode->matched);
                }
                //attadd(s, //id->lexeme, pnode-);
            }
            break;
        case SEMTYPE_NONTERM:
            factor.value.type = ATTYPE_STR;
            factor.value.str_ = (*curr)->lexeme;
            factor.access.base = (*curr)->lexeme;
            *curr = (*curr)->next;
            idsuffix = sem_idsuffix(curr, s, pda);
            factor.access.offset = idsuffix.factor_.index;
            factor.access.attribute = idsuffix.dot.id;
            if (idsuffix.dot.id && s->pass) {
                if (!strcmp(idsuffix.dot.id, "in"))
                    factor.value = getatt(s->parent, idsuffix.dot.id);
                else
                    factor.value = getatt(s, idsuffix.dot.id);
            }
            break;
        case SEMTYPE_NUM:
            if (!(*curr)->type.attribute) {
                factor.value.type = ATTYPE_NUMINT;
                factor.value.int_ = safe_atol((*curr)->lexeme);
                printf("%s %ld\n", (*curr)->lexeme, factor.value.int_);
            }
            else {
                factor.value.type = ATTYPE_NUMREAL;
                factor.value.real_ = safe_atod((*curr)->lexeme);
            }
            *curr = (*curr)->next;
            break;
        case SEMTYPE_NOT:
            *curr = (*curr)->next;
            id = *curr;
            factor = sem_factor(curr, s, pda);
            switch(factor.value.type) {
                case ATTYPE_STR:
                    printf("Type Error: Cannot apply logical not to string type.");
                    break;
                case ATTYPE_NUMINT:
                    factor.value.int_ = !factor.value.int_;
                    break;
                case ATTYPE_NUMREAL:
                    factor.value.real_ = !factor.value.real_;
                    factor.value.int_ = (long)factor.value.real_;
                    factor.value.type = ATTYPE_NUMINT;
                    break;
                case ATTYPE_NULL:
                    printf("null value\n");
                    break;
                default:
                    perror("Illegal State");
                    assert(false);
                    break;
            }
            break;
        case SEMTYPE_OPENPAREN:
            *curr = (*curr)->next;
            expression = sem_expression(curr, s, pda);
            sem_match(curr, SEMTYPE_CLOSEPAREN);
            factor.value = expression.value;
            break;
        case SEMTYPE_CODE:
            factor.value.type = ATTYPE_CODE;
            factor.value.str_ = (*curr)->lexeme_;
            *curr = (*curr)->next;
            break;
        default:
            fprintf(stderr, "Syntax Error at line %d: Expected identifier nonterm number or not but got %s\n", (*curr)->lineno, (*curr)->lexeme);
            assert(false);
            break;
    }
    return factor;
}

sem_factor__s sem_factor_ (token_s **curr, semantics_s *s)
{
    sem_factor__s factor_;

    switch((*curr)->type.val) {
        case SEMTYPE_NUM:
            if ((*curr)->type.attribute != ATTYPE_NUMINT)
                perror("Type Error: Index value must be an integer");
            factor_.index = (unsigned)safe_atol((*curr)->lexeme);
            *curr = (*curr)->next;
            break;
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
            factor_.index = 1;
            break;
        default:
            fprintf(stderr, "Syntax Error at line %d: Expected [ ] * / + - = < > <> <= >= fi else then if nonterm or $ but got %s\n", (*curr)->lineno, (*curr)->lexeme);
            assert(false);
            break;
    }
    return factor_;
}

sem_idsuffix_s sem_idsuffix (token_s **curr, semantics_s *s, pda_s *pda)
{
    sem_idsuffix_s idsuffix;
    
    switch ((*curr)->type.val) {
        case SEMTYPE_COMMA:
        case SEMTYPE_CLOSEPAREN:
        case SEMTYPE_MULOP:
        case SEMTYPE_ADDOP:
        case SEMTYPE_RELOP:
        case SEMTYPE_FI:
        case SEMTYPE_ELSE:
        case SEMTYPE_THEN:
        case SEMTYPE_IF:
        case SEMTYPE_DOT:
        case SEMTYPE_NONTERM:
        case SEMTYPE_NUM:
        case LEXTYPE_EOF:
            idsuffix.factor_ = sem_factor_(curr, s);
            idsuffix.dot = sem_dot(curr, s);
            break;
        case SEMTYPE_OPENPAREN:
            sem_paramlist(curr, s, pda);
            idsuffix.factor_.index = 0;
            idsuffix.dot.id = NULL;
            break;
        default:
            fprintf(stderr, "Syntax Error at line %d: Expected , ] [ * / + - = < > <> <= >= fi else then if . nonterm or $ but got %s\n", (*curr)->lineno, (*curr)->lexeme);
            assert(false);
            break;
    }

    return idsuffix;
}

sem_dot_s sem_dot (token_s **curr, semantics_s *s)
{
    sem_dot_s dot;
    
    switch((*curr)->type.val) {
        case SEMTYPE_DOT:
            *curr = (*curr)->next;
            dot.id = (*curr)->lexeme;
            sem_match(curr, SEMTYPE_ID);
            break;
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
            fprintf(stderr, "Syntax Error at line %d: Expected . ] * / + - = < > <> <= >= fi else then if nonterm or $ but got %s\n", (*curr)->lineno, (*curr)->lexeme);
            assert(false);
            break;
    }
    return dot;
}

sem_paramlist_s sem_paramlist (token_s **curr, semantics_s *s, pda_s *pda)
{
    sem_paramlist_s paramlist;
    sem_expression_s expression;

    paramlist.pstack = NULL;
    sem_match(curr, SEMTYPE_OPENPAREN);
    expression = sem_expression(curr, s, pda);
    llpush(&paramlist.pstack, alloc_semt(expression.value));
    sem_paramlist_(curr, s, &paramlist, pda);
    sem_match(curr, SEMTYPE_CLOSEPAREN);
    return paramlist;
}

void sem_paramlist_ (token_s **curr, semantics_s *s, sem_paramlist_s *list, pda_s *pda)
{
    sem_expression_s expression;
    
    switch ((*curr)->type.val) {
        case SEMTYPE_COMMA:
            *curr = (*curr)->next;
            expression = sem_expression(curr, s, pda);
            llpush(&list->pstack, alloc_semt(expression.value));
            sem_paramlist_(curr, s, list, pda);
            break;
        case SEMTYPE_CLOSEPAREN:
            break;
        default:
            fprintf(stderr, "Syntax Error at line %d: Expected , or ( but got %s\n", (*curr)->lineno, (*curr)->lexeme);
            assert(false);
            break;
    }
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
        fprintf(stderr, "Syntax Error at line %d: Expected + or - but got %s\n", (*curr)->lineno, (*curr)->lexeme);
        assert(false);
    }
}

bool sem_match (token_s **curr, int type)
{
    if ((*curr)->type.val == type) {
        *curr = (*curr)->next;
        return true;
    }
    fprintf(stderr, "Syntax Error at line %d: Got %s\n", (*curr)->lineno, (*curr)->lexeme);
    return false;
}

sem_type_s *alloc_semt(sem_type_s value)
{
    sem_type_s *val;
    
    val = malloc(sizeof(*val));
    if (!val) {
        perror("Memory Allocation Error");
        exit(EXIT_FAILURE);
    }
    *val = value;
    return val;
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
    att->lexeme = data;
    return att;
}

void setatt (semantics_s *s, char *id, sem_type_s *data)
{
    hashinsert_(s->table, id, data);
}

sem_type_s getatt (semantics_s *s, char *id)
{
    sem_type_s dummy = {0};
    sem_type_s *data;
    
    data = hashlookup(s->table, id);
    if (!data) {
        perror("Access to unitialized attribute.");
        return dummy;
    }
    return *data;
}