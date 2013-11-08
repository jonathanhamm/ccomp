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
#include <math.h>

#define REGEX_DECORATIONS_FILE "regex_decorations"
#define MACHID_START            36

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
#define OPTYPE_OR   10
#define OPTYPE_AND  11

#define ATTYPE_MULT 0
#define ATTYPE_DIV  1
#define ATTYPE_AND  2
#define ATTYPE_ADD  0
#define ATTYPE_SUB  1
#define ATTYPE_OR   2
#define ATTYPE_EQ   0
#define ATTYPE_NE   1
#define ATTYPE_L    2
#define ATTYPE_LE   3
#define ATTYPE_GE   4
#define ATTYPE_G    5

#define FTABLE_SIZE (sizeof(ftable) / sizeof(ftable[0]))

enum semantic_types_ {
    SEMTYPE_IF = LEXID_START+1, //Guarantees that lexical and semantics token types are disjoin
    SEMTYPE_THEN,
    SEMTYPE_ELSE,
    SEMTYPE_FI,
    SEMTYPE_NOT,
    SEMTYPE_OPENPAREN,
    SEMTYPE_CLOSEPAREN,
    SEMTYPE_DOT,
    SEMTYPE_COMMA,
    SEMTYPE_SEMICOLON,
    SEMTYPE_OPENBRACKET,
    SEMTYPE_CLOSEBRACKET,
    /*^ When removing one of these, subtract 1 from MACHID_START */
    
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
    SEMTYPE_CONCAT,
    SEMTYPE_MAP,
};

typedef struct att_s att_s;

/* Return Structures */
typedef struct access_s access_s;
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
typedef struct sem_range_s sem_range_s;
typedef struct sem_paramlist_s sem_paramlist_s;
typedef struct sem_paramlist__s sem_paramlist__s;
typedef struct sem_sign_s sem_sign_s;
typedef struct ftable_s ftable_s;

typedef void *(*sem_action_f)(token_s **, semantics_s *, pda_s *, parse_s *, sem_paramlist_s, unsigned , void *);

struct access_s
{
    char *base;
    long offset;
    char *attribute;
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

struct sem_range_s
{
    bool isready;
    bool isset;
    long value;
};

struct sem_dot_s
{
    sem_range_s range;
    char *id;
};

struct sem_paramlist_s
{
    bool ready;
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
    bool hasparam;
    sem_paramlist_s params;
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

struct ftable_s
{
    const char *key;
    sem_action_f action;
};

static sem_type_s sem_type_s_(parse_s *parse, token_s *token);
static bool test_semtype(sem_type_s value);
static inline unsigned toaddop(unsigned val);
static inline unsigned tomulop(unsigned val);
static inline unsigned torelop(unsigned val);
static pnode_s *getpnode_token(pna_s *pn, char *lexeme, unsigned index);
static pnode_s *getpnode_nterm_copy(pna_s *pn, char *lexeme, unsigned index);
static pnode_s *getpnode_nterm(production_s *prod, char *lexeme, unsigned index);
static sem_type_s sem_op(sem_type_s v1, sem_type_s v2, int op);
static sem_statements_s sem_statements (parse_s *parse, token_s **curr, llist_s **il, pda_s *pda, production_s *prod, pna_s *pn, semantics_s *syn, unsigned pass, bool evaluate);
static sem_statement_s sem_statement (parse_s *parse, token_s **curr, llist_s **il, pda_s *pda, production_s *prod, pna_s *pn, semantics_s *syn, unsigned pass, bool evaluate);
static sem_else_s sem_else (parse_s *parse, token_s **curr, llist_s **il, pda_s *pda, production_s *prod, pna_s *pn, semantics_s *syn, unsigned pass, bool evaluate);
static sem_expression_s sem_expression (parse_s *parse, token_s **curr, llist_s **il,  pda_s *pda, production_s *prod, pna_s *pn, semantics_s *syn, unsigned pass);
static sem_expression__s sem_expression_ (parse_s *parse, token_s **curr, llist_s **il, pda_s *pda, production_s *prod, pna_s *pn, semantics_s *syn, unsigned pass);
static sem_simple_expression_s sem_simple_expression (parse_s *parse, token_s **curr, llist_s **il, pda_s *pda, production_s *prod, pna_s *pn, semantics_s *syn, unsigned pass);
static sem_simple_expression__s sem_simple_expression_ (parse_s *parse, token_s **curr, llist_s **il, sem_type_s *accum, pda_s *pda, production_s *prod, pna_s *pn, semantics_s *syn, unsigned pass);
static sem_term_s sem_term (parse_s *parse, token_s **curr, llist_s **il, pda_s *pda, production_s *prod, pna_s *pn, semantics_s *syn, unsigned pass);
static sem_term__s sem_term_ (parse_s *parse, token_s **curr, llist_s **il, sem_type_s *accum, pda_s *pda, production_s *prod, pna_s *pn, semantics_s *syn, unsigned pass);
static sem_factor_s sem_factor (parse_s *parse, token_s **curr, llist_s **il, pda_s *pda, production_s *prod, pna_s *pn, semantics_s *syn, unsigned pass);
static sem_factor__s sem_factor_ (parse_s *parse, token_s **curr, llist_s **il, pna_s *pn, semantics_s *syn, unsigned pass);
static sem_idsuffix_s sem_idsuffix (parse_s *parse, token_s **curr, llist_s **il, pda_s *pda, production_s *prod, pna_s *pn, semantics_s *syn, unsigned pass);
static sem_dot_s sem_dot (parse_s *parse, token_s **curr, llist_s **il, pna_s *pn, semantics_s *syn, unsigned pass);
static sem_range_s sem_range (parse_s *parse, token_s **curr, llist_s **il, pna_s *pn, semantics_s *syn, unsigned pass);
static sem_paramlist_s sem_paramlist (parse_s *parse, token_s **curr, llist_s **il, pda_s *pda, production_s *prod, pna_s *pn, semantics_s *syn, unsigned pass);
static void sem_paramlist_ (parse_s *parse, token_s **curr, llist_s **il, sem_paramlist_s *list, pda_s *pda, production_s *prod, pna_s *pn, semantics_s *syn, unsigned pass);
static sem_sign_s sem_sign (token_s **curr);
static bool sem_match (token_s **curr, int type);

static sem_type_s *alloc_semt(sem_type_s value);
static att_s *att_s_ (void *data, unsigned tid);
static void setatt(semantics_s *s, char *id, sem_type_s *data);
static sem_type_s getatt(semantics_s *s, char *id);
static void *sem_array(token_s **curr, semantics_s *s, pda_s *pda, parse_s *parse, sem_paramlist_s params, unsigned pass, sem_type_s *fill);
static void *sem_emit(token_s **curr, semantics_s *s, pda_s *pda, parse_s *parse, sem_paramlist_s params, unsigned pass, void *fill);
static void *sem_error(token_s **curr, semantics_s *s, pda_s *pda, parse_s *parse, sem_paramlist_s params, unsigned pass, void *fill);
static void *sem_gettype(token_s **curr, semantics_s *s, pda_s *pda, parse_s *parse, sem_paramlist_s params, unsigned pass, void *fill);
static void *sem_halt(token_s **curr, semantics_s *s, pda_s *pda, parse_s *parse, sem_paramlist_s params, unsigned pass, void *fill);
static void *sem_lookup(token_s **curr, semantics_s *s, pda_s *pda, parse_s *parse, sem_paramlist_s params, unsigned pass, void *fill);
static void *sem_print(token_s **curr, semantics_s *s, pda_s *pda, parse_s *parse, sem_paramlist_s params, unsigned pass, void *fill);
static void *sem_addtype(token_s **curr, semantics_s *s, pda_s *pda, parse_s *parse, sem_paramlist_s params, unsigned pass, sem_type_s *type);
static int ftable_strcmp(char *key, ftable_s *b);
static sem_action_f get_semaction(char *str);
static char *sem_tostring(sem_type_s type);
static char *semstr_concat(char *base, sem_type_s val);
static void set_type(semantics_s *s, char *id, sem_type_s type);
static sem_type_s get_type(semantics_s *s, char *id);

void print_semtype(sem_type_s value);

/* Must be alphabetized */
static ftable_s ftable[] = {
    {"addtype", (sem_action_f)sem_addtype},
    {"array", (sem_action_f)sem_array},
    {"emit", sem_emit},
    {"error", sem_error},
    {"gettype", sem_gettype},
    {"halt", sem_halt},
    {"lookup", sem_lookup},
    {"print", sem_print}
};

sem_type_s sem_type_s_(parse_s *parse, token_s *token)
{
    sem_type_s s;
    tlookup_s look;
    
    s.lexeme = token->lexeme;
    look = idtable_lookup(parse->lex->kwtable, token->lexeme);
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
    //printf("printing value: ");
    switch (value.type) {
        case ATTYPE_NUMINT:
            printf("%ld", value.int_);
            break;
        case ATTYPE_NUMREAL:
            printf("%f", value.real_);
            break;
        case ATTYPE_ID:
        case ATTYPE_CODE:
        case ATTYPE_STR:
            printf("%s", value.str_);
            break;
        case ATTYPE_RANGE:
            printf("%ld..%ld", value.low, value.high);
            break;
        case ATTYPE_ARRAY:
            printf("array[%ld..%ld] of type %s", value.low, value.high, value.lexeme);
            break;
        case ATTYPE_NULL:
            printf("null");
            break;
        case ATTYPE_NOT_EVALUATED:
            printf("not evaluated");
            break;
        default:
            /*illegal*/
            break;
    }
}

bool test_semtype(sem_type_s value)
{
    if(value.type == ATTYPE_NOT_EVALUATED || value.type == ATTYPE_NULL)
        return false;
    return value.int_ || value.real_ || value.str_;
}

semantics_s *semantics_s_(parse_s *parse, mach_s *machs)
{
    semantics_s *s;
    
    s = malloc(sizeof(*s));
    if (!s) {
        perror("Memory Allocation Error");
        exit(EXIT_FAILURE);
    }
    s->parse = parse;
    s->machs = machs;
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
    static unsigned last;
    
    assert(*lineno < 500);
    for (i = 1; buf[i] != '}'; i++);
    buf[i] = EOF;
        
    addtok(tlist, "generated {", *lineno, LEXTYPE_ANNOTATE, LEXATTR_DEFAULT, NULL);
    ltok = lexf(data, &buf[1], anlineno, true);
    anlineno = ltok.lines;
    *lineno = ltok.lines;
    (*tlist)->next = ltok.tokens;
    ltok.tokens->prev = *tlist;
    while ((*tlist)->next)
        *tlist = (*tlist)->next;
    last = *lineno;
    puts("\n\n-------------------------------------------------------------------------------");
    for(iter = ltok.tokens; iter; iter = iter->next)
        printf("%s %d\n", iter->lexeme, iter->type.val);
    puts("-------------------------------------------------------------------------------\n\n");

    return i;
}

unsigned toaddop(unsigned val)
{
    if (val == ATTYPE_ADD)
        return OPTYPE_ADD;
    if (val == ATTYPE_SUB)
        return OPTYPE_SUB;
    if (val == ATTYPE_OR)
        return OPTYPE_OR;
    perror("Op attribute mismatch");
    assert(false);
}

unsigned tomulop(unsigned val)
{
    if (val == ATTYPE_MULT)
        return OPTYPE_MULT;
    if (val == ATTYPE_DIV)
        return OPTYPE_DIV;
    if (val == ATTYPE_AND)
        return OPTYPE_AND;
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

pnode_s *getpnode_token(pna_s *pn, char *lexeme, unsigned index)
{
    unsigned i, j;
    
    for (i = 0, j = 1; i < pn->size; i++) {
        if (!strcmp(pn->array[i].token->lexeme, lexeme)) {
            if (j == index)
                return &pn->array[i];
            j++;
        }
    }
    return NULL;
}

pnode_s *getpnode_nterm_copy(pna_s *pn, char *lexeme, unsigned index)
{
    unsigned i, j;
    
    for (i = 0, j = 1; i < pn->size; i++) {
        if (!strcmp(pn->array[i].token->lexeme, lexeme)) {
            printf("passed: %u %u\n", i, index);
            if (j == index)
                return &pn->array[i];
            i++;
        }
    }
    return NULL;
}

pnode_s *getpnode_nterm(production_s *prod, char *lexeme, unsigned index)
{
    unsigned j;
    pnode_s *p;
    
    for(p = prod->start, j = 1; p; p = p->next) {
        if(!strcmp(p->token->lexeme, lexeme)) {
            if(j == index)
                return p;
            j++;
        }
    }
    return NULL;
}


/* performs basic arithmetic operations with implicit type coercion */
sem_type_s sem_op(sem_type_s v1, sem_type_s v2, int op)
{
    sem_type_s result;
    if(op != OPTYPE_NOP) {
        if(v1.type == ATTYPE_NULL) {
            if(op == OPTYPE_EQ) {
                result.type = ATTYPE_NUMINT;
                result.int_ = ((v2.type == ATTYPE_NOT_EVALUATED) || (v2.type == ATTYPE_NULL));
            }
            else if(op == OPTYPE_NE) {
                result.type = ATTYPE_NUMINT;
                result.int_ = !((v2.type == ATTYPE_NOT_EVALUATED) || (v2.type == ATTYPE_NULL));
            }
            return result;
        }
        if(v2.type == ATTYPE_NULL) {
            if(op == OPTYPE_EQ) {
                result.type = ATTYPE_NUMINT;
                result.int_ = ((v1.type == ATTYPE_NOT_EVALUATED) || (v1.type == ATTYPE_NULL));
            }
            else if(op == OPTYPE_NE) {
                result.type = ATTYPE_NUMINT;
                result.int_ = !((v1.type == ATTYPE_NOT_EVALUATED) || (v1.type == ATTYPE_NULL));
            }
            return result;
        }
        if (v1.type == ATTYPE_NOT_EVALUATED || v2.type == ATTYPE_NOT_EVALUATED) {
            result.type = ATTYPE_NOT_EVALUATED;
            result.str_ = "null";
            puts("Not Evaluated");
            return result;
        }
        else if(v1.type == ATTYPE_NULL || v2.type == ATTYPE_NULL) {
            result.type = ATTYPE_NULL;
            result.str_ = "null";
            puts("Not Evaluated");
            return result;
        }
    }
    switch(op) {
        case OPTYPE_MULT:
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
            result.type = ATTYPE_NUMINT;
            result.int_ = 0;
            if (v1.type == ATTYPE_STR && v2.type == ATTYPE_STR) {
                result.int_ = !strcmp(v1.str_, v2.str_);
            }
            else if (v1.type == ATTYPE_NUMINT && v2.type == ATTYPE_NUMINT)
                result.int_ = v1.int_ == v2.int_;
            else if (v1.type == ATTYPE_NUMINT && v2.type == ATTYPE_NUMREAL)
                result.int_ = v1.int_ == v2.real_;
            else if (v1.type == ATTYPE_NUMREAL && v2.type == ATTYPE_NUMINT)
                result.int_ = v1.real_ == v2.int_;
            else
                result.int_ = v1.real_ == v2.real_;
            break;
        case OPTYPE_NE:
            result.type = ATTYPE_NUMINT;
            result.int_ = 0;
            if (v1.type == ATTYPE_STR && v2.type == ATTYPE_STR)
                result.int_ = !!strcmp(v1.str_, v2.str_);
            else if (v1.type == ATTYPE_NUMINT && v2.type == ATTYPE_NUMINT)
                result.int_ = v1.int_ != v2.int_;
            else if (v1.type == ATTYPE_NUMINT && v2.type == ATTYPE_NUMREAL)
                result.int_ = v1.int_ != v2.real_;
            else if (v1.type == ATTYPE_NUMREAL && v2.type == ATTYPE_NUMINT)
                result.int_ = v1.real_ != v2.int_;
            else
                result.int_ = v1.real_ != v2.real_;
            break;
        case OPTYPE_OR:
            if (v1.type == ATTYPE_STR || v2.type == ATTYPE_STR)
                printf("Type Error: String type incompatible with subtraction.\n");
            result.type = ATTYPE_NUMINT;
            if (v1.type == ATTYPE_NUMINT && v2.type == ATTYPE_NUMINT)
                result.int_ = v1.int_ || v2.int_;
            else if (v1.type == ATTYPE_NUMINT && v2.type == ATTYPE_NUMREAL)
                result.int_ = v1.int_ || v2.real_;
            else if (v1.type == ATTYPE_NUMREAL && v2.type == ATTYPE_NUMINT)
                result.int_ = v1.real_ || v2.int_;
            else
                result.int_ = v1.real_ || v2.real_;
            break;
        case OPTYPE_AND:
            if (v1.type == ATTYPE_STR || v2.type == ATTYPE_STR)
                printf("Type Error: String type incompatible with subtraction.\n");
            result.type = ATTYPE_NUMINT;
            if (v1.type == ATTYPE_NUMINT && v2.type == ATTYPE_NUMINT)
                result.int_ = v1.int_ && v2.int_;
            else if (v1.type == ATTYPE_NUMINT && v2.type == ATTYPE_NUMREAL)
                result.int_ = v1.int_ && v2.real_;
            else if (v1.type == ATTYPE_NUMREAL && v2.type == ATTYPE_NUMINT)
                result.int_ = v1.real_ && v2.int_;
            else
                result.int_ = v1.real_ && v2.real_;
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

llist_s *sem_start (semantics_s *in, parse_s *parse, mach_s *machs, pda_s *pda, production_s *prod, pna_s *pn, semantics_s *syn, unsigned pass)
{
    token_s *iter = prod->annot;
    llist_s *ilist = NULL;
    
    if(!iter)
        return NULL;
    
    sem_statements(parse, &iter, &ilist, pda, prod, pn, syn, pass, true);
    return ilist;
}

sem_statements_s sem_statements (parse_s *parse, token_s **curr, llist_s **il, pda_s *pda, production_s *prod, pna_s *pn, semantics_s *syn, unsigned pass, bool evaluate)
{
    switch((*curr)->type.val) {
        case SEMTYPE_IF:
        case SEMTYPE_NONTERM:
        case SEMTYPE_ID:
            sem_statement(parse, curr, il, pda, prod, pn, syn, pass, evaluate);
            sem_statements(parse, curr, il, pda, prod, pn, syn, pass, evaluate);
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

sem_statement_s sem_statement (parse_s *parse, token_s **curr, llist_s **il, pda_s *pda, production_s *prod, pna_s *pn, semantics_s *syn, unsigned pass, bool evaluate)
{
    pnode_s *p;
    char *id, *nterm;
    unsigned index;
    sem_expression_s expression;
    sem_idsuffix_s idsuffix;
    sem_paramlist_s params;
    semantics_s *in;
    bool test;
    
    switch((*curr)->type.val) {
        case SEMTYPE_NONTERM:
            nterm = (*curr)->lexeme;
            *curr = (*curr)->next;
            idsuffix = sem_idsuffix(parse, curr, il, pda, prod, pn, syn, pass);
            index = idsuffix.factor_.index;
            id = idsuffix.dot.id;
            sem_match(curr, SEMTYPE_ASSIGNOP);
            expression = sem_expression(parse, curr, il, pda, prod, pn, syn, pass);

            if (evaluate && expression.value.type != ATTYPE_NOT_EVALUATED) {

                if(!strcmp(pda->nterm->lexeme, nterm)) {
                    if(syn) {
                        setatt(syn, id, alloc_semt(expression.value));

                    }
                }
                else {
                    /* Setting inherited attributes */
                    p = getpnode_nterm(prod, nterm, index);
                    if(p && expression.value.type != ATTYPE_NOT_EVALUATED) {
                        in = get_il(*il, p);
                        if(!in) {
                            in = semantics_s_(NULL, NULL);
                            in->n = p;
                            llpush(il, in);
                        }
                        setatt(in, id, alloc_semt(expression.value));
                    }
                }
            }
            break;
        case SEMTYPE_IF:
            *curr = (*curr)->next;
            expression = sem_expression(parse, curr, il, pda, prod, pn, syn, pass);
            sem_match(curr, SEMTYPE_THEN);
            test = test_semtype(expression.value);
            sem_statements(parse, curr, il, pda, prod, pn, syn, pass, test && evaluate);
            sem_else(parse, curr, il, pda, prod, pn, syn, pass, !test && evaluate);
            break;
        case SEMTYPE_ID:
            id = (*curr)->lexeme;
            *curr = (*curr)->next;
            sem_match(curr, SEMTYPE_OPENPAREN);
            params = sem_paramlist(parse, curr, il, pda, prod, pn, syn, pass);
            sem_match(curr, SEMTYPE_CLOSEPAREN);
            if (evaluate && params.ready)
                get_semaction(id)(curr, NULL, pda, parse, params, pass, &expression);
            break;
        default:
            fprintf(stderr, "Syntax Error at line %d: Expected nonterm or if but got %s", (*curr)->lineno, (*curr)->lexeme);
            assert(false);
            break;
    }
}

sem_else_s sem_else (parse_s *parse, token_s **curr, llist_s **il, pda_s *pda, production_s *prod, pna_s *pn, semantics_s *syn, unsigned pass, bool evaluate)
{
    switch((*curr)->type.val) {
        case SEMTYPE_ELSE:
            *curr = (*curr)->next;
            sem_statements(parse, curr, il, pda, prod, pn, syn, pass, evaluate);
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

sem_expression_s sem_expression (parse_s *parse, token_s **curr, llist_s **il, pda_s *pda, production_s *prod, pna_s *pn, semantics_s *syn, unsigned pass)
{
    sem_expression_s expression;
    sem_expression__s expression_;
    sem_simple_expression_s simple_expression;
    
    simple_expression = sem_simple_expression(parse, curr, il, pda, prod, pn, syn, pass);
    expression_ = sem_expression_(parse, curr, il, pda, prod, pn, syn, pass);
    if (expression_.op != OPTYPE_NOP) {
       /* printf("Comparing:\n");
        print_semtype(simple_expression.value);
        puts("   ");
        print_semtype(expression_.value);
        //if (expression_.value.type == ATTYPE_ID)
           // for(;;)printf("%u\n", expression_.value.type);
        puts("\n\n");*/
    }
    expression.value = sem_op(simple_expression.value, expression_.value, expression_.op);
    return expression;
}

sem_expression__s sem_expression_ (parse_s *parse, token_s **curr, llist_s **il, pda_s *pda, production_s *prod, pna_s *pn, semantics_s *syn, unsigned pass)
{
    sem_expression__s expression_;
        
    switch((*curr)->type.val) {
        case SEMTYPE_RELOP:
            expression_.op = torelop((*curr)->type.attribute);
            *curr = (*curr)->next;
            expression_.value = sem_simple_expression(parse, curr, il, pda, prod, pn, syn, pass).value;
            break;
        case SEMTYPE_COMMA:
        case SEMTYPE_FI:
        case SEMTYPE_ELSE:
        case SEMTYPE_THEN:
        case SEMTYPE_IF:
        case SEMTYPE_NONTERM:
        case SEMTYPE_CLOSEPAREN:
        case SEMTYPE_CLOSEBRACKET:
        case SEMTYPE_ID:
        case LEXTYPE_EOF:
            expression_.op = OPTYPE_NOP;
            expression_.value.type = ATTYPE_VOID;
            break;
        default:
            fprintf(stderr, "Syntax Error at line %d: Expected relop ] fi else then if nonterm or $ but got %s\n", (*curr)->lineno, (*curr)->lexeme);
            assert(false);
            break;
            
    }
    return expression_;
}

sem_simple_expression_s sem_simple_expression (parse_s *parse, token_s **curr, llist_s **il, pda_s *pda, production_s *prod, pna_s *pn, semantics_s *syn, unsigned pass)
{
    sem_sign_s sign;
    sem_simple_expression_s simple_expression;
    sem_term_s term;
    sem_simple_expression__s simple_expression_;
    
    switch((*curr)->type.val) {
        case SEMTYPE_ADDOP:

            sign = sem_sign(curr);
            simple_expression = sem_simple_expression(parse, curr, il, pda, prod, pn, syn, pass);
            if (sign.value == SEMSIGN_NEG) {
                if (simple_expression.value.type == ATTYPE_NUMINT)
                    simple_expression.value.int_ = -simple_expression.value.int_;
                else if(simple_expression.value.type == ATTYPE_NUMREAL)
                    simple_expression.value.real_ = -simple_expression.value.real_;
                else if(simple_expression.value.type != ATTYPE_NOT_EVALUATED)
                    perror("Type Error: Cannot negate id types\n");
            }
            break;
        case SEMTYPE_NOT:
        case SEMTYPE_NUM:
        case SEMTYPE_ID:
        case SEMTYPE_NONTERM:
        case SEMTYPE_OPENPAREN:
        case SEMTYPE_CODE:
            term = sem_term(parse, curr, il, pda, prod, pn, syn, pass);
            simple_expression_ = sem_simple_expression_(parse, curr, il, &term.value, pda, prod, pn, syn, pass);
            simple_expression.value = term.value;
            break;
        default:
            fprintf(stderr, "Syntax Error at line %d: Expected + - not number or identifier but got %s\n", (*curr)->lineno, (*curr)->lexeme);
            assert(false);
            break;
    }
    return simple_expression;
}

sem_simple_expression__s sem_simple_expression_ (parse_s *parse, token_s **curr, llist_s **il, sem_type_s *accum, pda_s *pda, production_s *prod, pna_s *pn, semantics_s *syn, unsigned pass)
{
    unsigned op;
    sem_term_s term;
    sem_simple_expression__s simple_expression_, simple_expression__;
    
    switch((*curr)->type.val) {
        case SEMTYPE_ADDOP:
            op = toaddop((*curr)->type.attribute);
            *curr = (*curr)->next;
            term = sem_term(parse, curr, il, pda, prod, pn, syn, pass);
            *accum = sem_op(*accum, term.value, op);
            simple_expression__ = sem_simple_expression_(parse, curr, il, accum, pda, prod, pn, syn, pass);
            break;
        case SEMTYPE_COMMA:
        case SEMTYPE_RELOP:
        case SEMTYPE_FI:
        case SEMTYPE_ELSE:
        case SEMTYPE_THEN:
        case SEMTYPE_IF:
        case SEMTYPE_NONTERM:
        case SEMTYPE_CLOSEPAREN:
        case SEMTYPE_CLOSEBRACKET:
        case SEMTYPE_ID:
        case LEXTYPE_EOF:
            simple_expression_.op = OPTYPE_NOP;
            simple_expression_.value.type = ATTYPE_VOID;
            break;
        default:
            fprintf(stderr, "Syntax Error at line %d: Expected + - ] = < > <> <= >= fi else then if nonterm or $ but got %s\n", (*curr)->lineno, (*curr)->lexeme);
            assert(false);
            break;
    }
    return simple_expression_;
}

sem_term_s sem_term (parse_s *parse, token_s **curr, llist_s **il, pda_s *pda, production_s *prod, pna_s *pn, semantics_s *syn, unsigned pass)
{
    sem_term_s term;
    sem_factor_s factor;
    sem_term__s term_;
    
    factor = sem_factor(parse, curr, il, pda, prod, pn, syn, pass);
    term_ = sem_term_(parse, curr, il, &factor.value, pda, prod, pn, syn, pass);
    term.value = factor.value;
    //term.value = sem_op(factor.value, term_.value, term_.op);
    return term;
}

sem_term__s sem_term_ (parse_s *parse, token_s **curr, llist_s **il, sem_type_s *accum, pda_s *pda, production_s *prod, pna_s *pn, semantics_s *syn, unsigned pass)
{
    unsigned op;
    sem_factor_s factor;
    sem_term__s term_, term__;
    
    switch((*curr)->type.val) {
        case SEMTYPE_MULOP:
            op = tomulop((*curr)->type.attribute);
            *curr = (*curr)->next;
            factor = sem_factor(parse, curr, il, pda, prod, pn, syn, pass);
            *accum = sem_op(*accum, factor.value, op);
            term_ = sem_term_(parse, curr, il, accum, pda, prod, pn, syn, pass);
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
        case SEMTYPE_CLOSEBRACKET:
        case SEMTYPE_ID:
        case LEXTYPE_EOF:
            term_.op = OPTYPE_NOP;
            term_.value.type = ATTYPE_VOID;
            term_.value.str_ = "null";
            break;
        default:
            fprintf(stderr, "Syntax Error at line %d: Expected * / ] + - = < > <> <= >= fi else then if nonterm or $ but got %s\n", (*curr)->lineno, (*curr)->lexeme);
            assert(false);
            break;
    }
    return term_;
}

sem_factor_s sem_factor (parse_s *parse, token_s **curr, llist_s **il, pda_s *pda, production_s *prod, pna_s *pn, semantics_s *syn, unsigned pass)
{
    token_s *id;
    pnode_s *pnode;
    semantics_s *in;
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
            idsuffix = sem_idsuffix(parse, curr, il, pda, prod, pn, syn, pass);
            //attadd (semantics_s *s, char *id, sem_type_s *data)
            if (idsuffix.dot.id) {
                if (!strcmp(idsuffix.dot.id, "val")) {
                    pnode = getpnode_token(pn, id->lexeme, idsuffix.factor_.index);
                    if(pnode && pnode->pass)
                        factor.value = sem_type_s_(parse, pnode->matched);
                    
                    if (idsuffix.dot.range.isset && idsuffix.dot.range.isready) {
                        factor.value.type = ATTYPE_RANGE;
                        pnode = getpnode_token(pn, id->lexeme, idsuffix.factor_.index);
                        if(pnode && pnode->pass) {
                            factor.value.low = safe_atol(pnode->matched->lexeme);
                            factor.value.high = idsuffix.dot.range.value;
                        }
                    }

                }
                else if (!strcmp(idsuffix.dot.id, "entry")) {
                    factor.value.type = ATTYPE_ID;
                    pnode = getpnode_token(pn, id->lexeme, idsuffix.factor_.index);
                    if(pnode && pnode->matched)
                        factor.value.str_ = pnode->matched->lexeme;
                    else {
                        factor.value.type = ATTYPE_NOT_EVALUATED;
                        factor.value.str_ = "null";
                    }
                }

                //attadd(s, //id->lexeme, pnode-);
            }
            else if (idsuffix.hasparam) {
                if(idsuffix.params.ready) {
                    factor.value = *(sem_type_s *)get_semaction(id->lexeme)(curr, NULL, pda, parse, idsuffix.params, pass, &factor.value);
                }
            }
            else {
                if(!strcmp(id->lexeme, "null")) {
                    factor.value.type = ATTYPE_NULL;
                    factor.value.str_ = "null";
                }
            }
            break;
        case SEMTYPE_NONTERM:
            factor.value.type = ATTYPE_STR;
            factor.value.str_ = (*curr)->lexeme;
            factor.access.base = (*curr)->lexeme;
            *curr = (*curr)->next;
            idsuffix = sem_idsuffix(parse, curr, il, pda, prod, pn, syn, pass);
            factor.access.offset = idsuffix.factor_.index;
            factor.access.attribute = idsuffix.dot.id;
            if (idsuffix.dot.id) {
                if (!strcmp(factor.value.str_, pda->nterm->lexeme)) {
                  /*  factor.value.type = ATTYPE_NOT_EVALUATED;
                    if(syn)
                        factor.value = getatt(pn->curr->syn, idsuffix.dot.id);
                    if(factor.value.type == ATTYPE_NOT_EVALUATED && pn->curr)
                        factor.value = getatt(pn->curr->in, idsuffix.dot.id);*/
                    if(pn->curr) {
                        printf("Attempting to get %s from %s\n", idsuffix.dot.id, pn->curr->token->lexeme);
                        factor.value = getatt(pn->curr->in, idsuffix.dot.id);
                    }
                    if(factor.value.type == ATTYPE_NOT_EVALUATED)
                        factor.value = getatt(pn->curr->syn, idsuffix.dot.id);
                }
                else {
                    factor.value.type = ATTYPE_NOT_EVALUATED;
                    pnode = getpnode_token(pn, factor.value.str_, idsuffix.factor_.index);
                    if(pnode) {
                        factor.value = getatt(pnode->syn, idsuffix.dot.id);
                            
                    }
                    if(factor.value.type == ATTYPE_NOT_EVALUATED) {
                        in = get_il(*il, pnode);
                        factor.value = getatt(in, idsuffix.dot.id);
                    }
                }
            }
            break;
        case SEMTYPE_NUM:
            if (!(*curr)->type.attribute) {
                factor.value.type = ATTYPE_NUMINT;
                factor.value.lexeme = (*curr)->lexeme;
                factor.value.int_ = safe_atol((*curr)->lexeme);
            }
            else {
                factor.value.type = ATTYPE_NUMREAL;
                factor.value.lexeme = (*curr)->lexeme;
                factor.value.real_ = safe_atod((*curr)->lexeme);
            }
            *curr = (*curr)->next;
            break;
        case SEMTYPE_NOT:
            *curr = (*curr)->next;
            id = *curr;
            factor = sem_factor(parse, curr, il, pda, prod, pn, syn, pass);
            switch(factor.value.type) {
                case ATTYPE_STR:
                    fprintf(stderr, "Type Error: Cannot apply logical not to string type.");
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
            expression = sem_expression(parse, curr, il, pda, prod, pn, syn, pass);
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

sem_factor__s sem_factor_ (parse_s *parse, token_s **curr, llist_s **il, pna_s *pn, semantics_s *syn, unsigned pass)
{
    sem_factor__s factor_;

    switch((*curr)->type.val) {
        case SEMTYPE_NUM:
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
        case SEMTYPE_CLOSEBRACKET:
        case SEMTYPE_COMMA:
        case SEMTYPE_ASSIGNOP:
        case SEMTYPE_ID:
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

sem_idsuffix_s sem_idsuffix (parse_s *parse, token_s **curr, llist_s **il, pda_s *pda, production_s *prod, pna_s *pn, semantics_s *syn, unsigned pass)
{
    sem_expression_s expression;
    sem_idsuffix_s idsuffix;
    
    switch ((*curr)->type.val) {
        case SEMTYPE_COMMA:
        case SEMTYPE_CLOSEPAREN:
        case SEMTYPE_CLOSEBRACKET:
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
        case SEMTYPE_ID:
        case LEXTYPE_EOF:
            idsuffix.factor_ = sem_factor_(parse, curr, il, pn, syn, pass);
            idsuffix.dot = sem_dot(parse, curr, il, pn, syn, pass);
            idsuffix.hasparam = false;
            break;
        case SEMTYPE_OPENPAREN:
            *curr = (*curr)->next;
            idsuffix.hasparam = true;
            idsuffix.params = sem_paramlist(parse, curr, il, pda, prod, pn, syn, pass);
            idsuffix.factor_.index = 1;
            idsuffix.dot.id = NULL;
            sem_match(curr, SEMTYPE_CLOSEPAREN);
            break;
        case SEMTYPE_OPENBRACKET:
            *curr = (*curr)->next;
            expression = sem_expression(parse, curr, il, pda, prod, pn, syn, pass);
            sem_match(curr, SEMTYPE_CLOSEBRACKET);
            idsuffix.factor_.index = 1;
            idsuffix.dot = sem_dot(parse, curr, il, pn, syn, pass);
            idsuffix.hasparam = false;
            break;
        default:
            fprintf(stderr, "Syntax Error at line %d: Expected , ] [ * / + - = < > <> <= >= fi else then if . nonterm or $ but got %s\n", (*curr)->lineno, (*curr)->lexeme);
            assert(false);
            break;
    }

    return idsuffix;
}

sem_dot_s sem_dot (parse_s *parse, token_s **curr, llist_s **il, pna_s *pn, semantics_s *syn, unsigned pass)
{
    sem_dot_s dot;
    
    switch((*curr)->type.val) {
        case SEMTYPE_DOT:
            *curr = (*curr)->next;
            dot.id = (*curr)->lexeme;
            sem_match(curr, SEMTYPE_ID);
            dot.range = sem_range(parse, curr, il, pn, syn, pass);
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
        case SEMTYPE_CLOSEBRACKET:
        case SEMTYPE_COMMA:
        case SEMTYPE_ID:
        case SEMTYPE_ASSIGNOP:
        case LEXTYPE_EOF:
            dot.id = NULL;
            dot.range.isset = false;
            break;
        default:
            fprintf(stderr, "Syntax Error at line %d: Expected . ] * / + - = < > <> <= >= fi else then if nonterm or $ but got %s\n", (*curr)->lineno, (*curr)->lexeme);
            assert(false);
            break;
    }
    return dot;
}

sem_range_s sem_range (parse_s *parse, token_s **curr, llist_s **il, pna_s *pn, semantics_s *syn, unsigned pass)
{
    unsigned index;
    token_s *id1, *id2;
    sem_range_s range;
    pnode_s *p;
    
    switch ((*curr)->type.val) {
        case SEMTYPE_DOT:
            *curr = (*curr)->next;
            sem_match(curr, SEMTYPE_DOT);
            id1 = *curr;
            sem_match(curr, SEMTYPE_ID);
            sem_match(curr, SEMTYPE_OPENBRACKET);
            index = (unsigned)safe_atol((*curr)->lexeme);
            sem_match(curr, SEMTYPE_NUM);
            sem_match(curr, SEMTYPE_CLOSEBRACKET);
            sem_match(curr, SEMTYPE_DOT);
            id2 = *curr;
            sem_match(curr, SEMTYPE_ID);
            p = getpnode_token(pn, id1->lexeme, index);
            range.isset = true;
            if(p && p->pass) {
                range.value = safe_atol(p->matched->lexeme);
                range.isready = true;
            }
            else
                range.isready = false;
            break;
        case SEMTYPE_COMMA:
        case SEMTYPE_CLOSEBRACKET:
        case SEMTYPE_MULOP:
        case SEMTYPE_ADDOP:
        case SEMTYPE_RELOP:
        case SEMTYPE_FI:
        case SEMTYPE_CLOSEPAREN:
        case SEMTYPE_ID:
        case SEMTYPE_ELSE:
        case SEMTYPE_THEN:
        case SEMTYPE_IF:
        case SEMTYPE_ASSIGNOP:
        case SEMTYPE_NONTERM:
        case LEXTYPE_EOF:
            range.isready = true;
            range.isset = false;
            range.value = 1;
            break;
        default:
            fprintf(stderr, "Syntax Error at line %d: Expected . , ] * / and or + - = < > <> <= >= ) fi else then if nonterm or $ but got %s\n", (*curr)->lineno, (*curr)->lexeme);
            assert(false);
            break;
    }
    return range;
}

sem_paramlist_s sem_paramlist(parse_s *parse, token_s **curr, llist_s **il, pda_s *pda, production_s *prod, pna_s *pn, semantics_s *syn, unsigned pass)
{
    sem_paramlist_s paramlist;
    sem_expression_s expression;

    paramlist.ready = true;
    paramlist.pstack = NULL;

    switch((*curr)->type.val) {
        case SEMTYPE_ADDOP:
        case SEMTYPE_CODE:
        case SEMTYPE_NOT:
        case SEMTYPE_NUM:
        case SEMTYPE_OPENPAREN:
        case SEMTYPE_ID:
        case SEMTYPE_NONTERM:
            expression = sem_expression(parse, curr, il, pda, prod, pn, syn, pass);
            if(expression.value.type == ATTYPE_NOT_EVALUATED)
                paramlist.ready = false;
            else
                llpush(&paramlist.pstack, alloc_semt(expression.value));
            sem_paramlist_(parse, curr, il, &paramlist, pda, prod, pn, syn, pass);
            break;
        case SEMTYPE_CLOSEPAREN:
            break;
        default:
            fprintf(stderr, "Syntax Error at line %d: Expected + - code not number ( identifier or nonterm, but got %s\n", (*curr)->lineno, (*curr)->lexeme);
            assert(false);
            break;
    }
    return paramlist;
}

void sem_paramlist_ (parse_s *parse, token_s **curr, llist_s **il, sem_paramlist_s *list, pda_s *pda, production_s *prod, pna_s *pn, semantics_s *syn, unsigned pass)
{
    sem_expression_s expression;
    
    switch ((*curr)->type.val) {
        case SEMTYPE_COMMA:
            *curr = (*curr)->next;
            expression = sem_expression(parse, curr, il, pda, prod, pn, syn, pass);
            if(expression.value.type == ATTYPE_NOT_EVALUATED)
                list->ready = false;
            else if (list->ready)
                llpush(&list->pstack, alloc_semt(expression.value));
            sem_paramlist_(parse, curr, il, list, pda, prod, pn, syn, pass);
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
    assert(false);
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
  //  printf("\n------------------INSERTING %s INTO %s %p %p  ", id, s->pda->nterm->lexeme, s->pda, data);
    
    
    if(data->type != ATTYPE_NOT_EVALUATED) {
       // print_semtype(*data);
        hashinsert_(s->table, id, data);
    }
}

sem_type_s getatt (semantics_s *s, char *id)
{
    sem_type_s dummy;
    sem_type_s *data;
    
    dummy.type = ATTYPE_NOT_EVALUATED;
    
   // printf("\n------------------RETRIEVING %s FROM %s %p\n", id, s->pda->nterm->lexeme, s->pda);
    if(!s)
        return dummy;
    
    data = hashlookup(s->table, id);
    if (!data) {
        puts("Access to unitialized attribute.");
        return dummy;
    }
    return *data;
}

void *sem_array(token_s **curr, semantics_s *s, pda_s *pda, parse_s *p, sem_paramlist_s params, unsigned pass, sem_type_s *fill)
{
    llist_s *node;
    sem_type_s *val1, *val2;
    
    node = llpop(&params.pstack);
    val1 = node->ptr;
    free(node);
    node = llpop(&params.pstack);
    val2 = node->ptr;
    free(node);

    val1->lexeme = malloc(strlen(val1->str_) + 1);
    if(!val1->lexeme) {
        perror("Memory Allocation Error");
        exit(EXIT_FAILURE);
    }
    strcpy(val1->lexeme, val1->str_);
    val1->low = val2->low;
    val1->high = val2->high;

    val1->type = ATTYPE_ARRAY;
    
    return val1;
}

void *sem_emit(token_s **curr, semantics_s *s, pda_s *pda, parse_s *p, sem_paramlist_s params, unsigned pass, void *fill)
{
    printf("Emit Called\n");
}

void *sem_error(token_s **curr, semantics_s *s, pda_s *pda, parse_s *p, sem_paramlist_s params, unsigned pass, void *fill)
{
    printf("Type Error\n");
}

void *sem_gettype(token_s **curr, semantics_s *s, pda_s *pda, parse_s *parse, sem_paramlist_s params, unsigned pass, void *fill)
{
    llist_s *node;
    sem_type_s *val;
    
    if(params.ready) {
        node = llpop(&params.pstack);
        val = node->ptr;
        free(node);
        //parse->lex->machs
        
        switch(val->type) {
        
        }
        
    }
}

void *sem_halt(token_s **curr, semantics_s *s, pda_s *pda, parse_s *p, sem_paramlist_s params, unsigned pass, void *fill)
{
    printf("Halt Called in %s\n", pda->nterm->lexeme);
    fflush(stderr);
    fflush(stdin);
    assert(false);
}

void *sem_lookup(token_s **curr, semantics_s *s, pda_s *pda, parse_s *parse, sem_paramlist_s params, unsigned pass, void *fill)
{
    llist_s *node;
    sem_type_s *val, type;
    
    if(params.ready) {
        node = llpop(&params.pstack);
        val = node->ptr;
        free(node);
        
        printf("getting type for: %s\n", val->str_);
        type = gettype(parse->lex, val->str_);
        return alloc_semt(type);
    }
    return NULL;
}

void *sem_print(token_s **curr, semantics_s *s, pda_s *pda, parse_s *p, sem_paramlist_s params, unsigned pass, void *fill)
{
    llist_s *node;
    sem_type_s *val;
    
    llreverse(&params.pstack);
    while (params.pstack) {
        node = llpop(&params.pstack);
        val = node->ptr;
        free(node);
        print_semtype(*val);
        putchar('\n');
        free(val);
    }
    return NULL;
}
static int bobb;

void *sem_addtype(token_s **curr, semantics_s *s, pda_s *pda, parse_s *p, sem_paramlist_s params, unsigned pass, sem_type_s *type)
{
    llist_s *node;
    sem_type_s *t, *id;
    
    if(pass > 2)
        return NULL;
    
    node = llpop(&params.pstack);
    t = node->ptr;
    free(node);
    
    node = llpop(&params.pstack);
    id = node->ptr;
    free(node);
    
    printf("Adding Type %s of type ", id->str_);
    print_semtype(*t);
    putchar('\n');
    settype(p->lex, id->str_, *t);
    if(bobb > 10)
        assert(false);

    ++bobb;
    return NULL;
}

int ftable_strcmp(char *key, ftable_s *b)
{
    return strcasecmp(key, b->key);
}

sem_action_f get_semaction(char *str)
{
    ftable_s *rec;
    
    printf("%lu, %lu\n", FTABLE_SIZE, sizeof(ftable[0]));
    rec = bsearch(str, ftable, FTABLE_SIZE, sizeof(*ftable), (int (*)(const void *, const void *))ftable_strcmp);
    if (!rec) {
        fprintf(stderr, "Error: Undefined function: %s\n", str);
        assert(false);
    }
    return rec->action;
}

char *sem_tostring(sem_type_s type)
{
    char *stralloc;
    
    switch(type.type) {
        case ATTYPE_STR:
        case ATTYPE_CODE:
            return type.str_;
        case ATTYPE_NUMINT:
            stralloc = malloc(FS_INTWIDTH_DEC(type.int_)+1);
            if (stralloc) {
                perror("Memory Allocation Error");
                exit(EXIT_FAILURE);
            }
            sprintf(stralloc, "%ld", type.int_);
            break;
        case ATTYPE_NUMREAL:
            
        default:
            break;
    }
}

char *semstr_concat(char *base, sem_type_s val)
{
    char *str = sem_tostring(val);
        
    if (!base) {
        base = malloc(sizeof(*base) + strlen(str) + 1);
        if (!base) {
            perror("Memory Allocation Error");
            exit(EXIT_FAILURE);
        }
    }
    else
        base = realloc(base, strlen(base) + strlen(str) + 1);
    strcat(base, str);
    return base;
}

void set_type(semantics_s *s, char *id, sem_type_s type)
{
    tdat_s tdat;
    idtable_s *table = s->parse->lex->idtable;
    
    tdat = idtable_lookup(table, id).tdat;
    tdat.type = type;
    idtable_set(table, id, tdat);
}

sem_type_s get_type(semantics_s *s, char *id)
{
    tdat_s tdat;
    idtable_s *table = s->parse->lex->idtable;
    
    tdat = idtable_lookup(table, id).tdat;
    return tdat.type;
}

void free_sem(semantics_s *s)
{
    if(s) {
        free_hash(s->table);
        free(s);
    }
}

semantics_s *get_il(llist_s *l, pnode_s *p)
{
    while(l) {
        if(((semantics_s *)l->ptr)->n == p)
            return l->ptr;
        l = l->next;
    }
    return NULL;
}