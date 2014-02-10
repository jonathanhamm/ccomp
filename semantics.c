/*
 semantics.c
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
#define MACHID_START            37

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

#define TYPE_ERROR_PREFIX "      --Semantics Error at line %u: "
#define TYPE_ERROR_SUFFIX " at token %s"

#define FTABLE_SIZE (sizeof(ftable) / sizeof(ftable[0]))

enum semantic_types_ {
    SEMTYPE_IF = LEXID_START+1, //Guarantees that lexical and semantics token types are disjoint
    SEMTYPE_THEN,
    SEMTYPE_ELSE,
    SEMTYPE_END,
    SEMTYPE_NOT,
    SEMTYPE_OPENPAREN,
    SEMTYPE_CLOSEPAREN,
    SEMTYPE_DOT,
    SEMTYPE_COMMA,
    SEMTYPE_SEMICOLON,
    SEMTYPE_OPENBRACKET,
    SEMTYPE_CLOSEBRACKET,
    SEMTYPE_ELIF,
    /* When removing one of these, subtract 1 from MACHID_START */
    
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
typedef struct sem_elif_s sem_elif_s;
typedef struct sem_elif__s sem_elif__s;
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
typedef struct test_s test_s;

typedef void *(*sem_action_f)(token_s **, semantics_s *, pda_s *, pna_s *, parse_s *, sem_paramlist_s, unsigned , void *, bool, bool);

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

struct sem_elif_s
{
    
};

struct sem_elif__s
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
    bool isset;
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
    bool hasmap;
    sem_type_s map;
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

struct test_s
{
    bool evaluated;
    bool result;
};

FILE *emitdest;
llist_s *grammar_stack;
static unsigned tempcount;
static unsigned lablecount;

static uint16_t semgrammar_hashf(void *key);
static bool semgrammar_isequalf(void *key1, void *key2);
static sem_type_s sem_type_s_(parse_s *parse, token_s *token);
static test_s test_semtype(sem_type_s value);
static inline unsigned toaddop(unsigned val);
static inline unsigned tomulop(unsigned val);
static inline unsigned torelop(unsigned val);
static pnode_s *getpnode_token(pna_s *pn, char *lexeme, unsigned index);
static pnode_s *getpnode_nterm_copy(pna_s *pn, char *lexeme, unsigned index);
static pnode_s *getpnode_nterm(production_s *prod, char *lexeme, unsigned index);
static sem_type_s sem_op(token_s **curr, parse_s *parse, token_s *tok, sem_type_s v1, sem_type_s v2, int op);
static sem_statements_s sem_statements (parse_s *parse, token_s **curr, llist_s **il, pda_s *pda, production_s *prod, pna_s *pn, semantics_s *syn, unsigned pass, test_s evaluate, bool elprev, bool isfinal);
static sem_statement_s sem_statement (parse_s *parse, token_s **curr, llist_s **il, pda_s *pda, production_s *prod, pna_s *pn, semantics_s *syn, unsigned pass, test_s evaluate, bool elprev, bool isfinal);
static sem_else_s sem_else (parse_s *parse, token_s **curr, llist_s **il, pda_s *pda, production_s *prod, pna_s *pn, semantics_s *syn, unsigned pass, test_s evaluate, bool elprev, bool isfinal);
static sem_elif_s sem_elif(parse_s *parse, token_s **curr, llist_s **il, pda_s *pda, production_s *prod, pna_s *pn, semantics_s *syn, unsigned pass, test_s evaluate, bool elprev, bool isfinal);
static sem_expression_s sem_expression (parse_s *parse, token_s **curr, llist_s **il,  pda_s *pda, production_s *prod, pna_s *pn, semantics_s *syn, unsigned pass, bool eval, bool isfinal);
static sem_expression__s sem_expression_ (parse_s *parse, token_s **curr, llist_s **il, pda_s *pda, production_s *prod, pna_s *pn, semantics_s *syn, unsigned pass, bool eval, bool isfinal);
static sem_simple_expression_s sem_simple_expression (parse_s *parse, token_s **curr, llist_s **il, pda_s *pda, production_s *prod, pna_s *pn, semantics_s *syn, unsigned pass, bool eval, bool isfinal);
static sem_simple_expression__s sem_simple_expression_ (parse_s *parse, token_s **curr, llist_s **il, sem_type_s *accum, pda_s *pda, production_s *prod, pna_s *pn, semantics_s *syn, unsigned pass, bool eval, bool isfinal);
static sem_term_s sem_term (parse_s *parse, token_s **curr, llist_s **il, pda_s *pda, production_s *prod, pna_s *pn, semantics_s *syn, unsigned pass, bool eval, bool isfinal);
static sem_term__s sem_term_ (parse_s *parse, token_s **curr, llist_s **il, sem_type_s *accum, pda_s *pda, production_s *prod, pna_s *pn, semantics_s *syn, unsigned pass, bool eval, bool isfinal);
static sem_factor_s sem_factor (parse_s *parse, token_s **curr, llist_s **il, pda_s *pda, production_s *prod, pna_s *pn, semantics_s *syn, unsigned pass, bool eval, bool isfinal);
static sem_factor__s sem_factor_ (parse_s *parse, token_s **curr, llist_s **il, pna_s *pn, semantics_s *syn, unsigned pass, bool eval, bool isfinal);
static sem_idsuffix_s sem_idsuffix (parse_s *parse, token_s **curr, llist_s **il, pda_s *pda, production_s *prod, pna_s *pn, semantics_s *syn, unsigned pass, bool eval, bool isfinal);
static sem_dot_s sem_dot (parse_s *parse, token_s **curr, llist_s **il, pna_s *pn, semantics_s *syn, unsigned pass, bool eval, bool isfinal);
static sem_range_s sem_range (parse_s *parse, token_s **curr, llist_s **il, pna_s *pn, semantics_s *syn, unsigned pass, bool eval, bool isfinal);
static sem_paramlist_s sem_paramlist (parse_s *parse, token_s **curr, llist_s **il, pda_s *pda, production_s *prod, pna_s *pn, semantics_s *syn, unsigned pass, bool eval, bool isfinal);
static void sem_paramlist_ (parse_s *parse, token_s **curr, llist_s **il, sem_paramlist_s *list, pda_s *pda, production_s *prod, pna_s *pn, semantics_s *syn, unsigned pass, bool eval, bool isfinal);
static sem_sign_s sem_sign (token_s **curr);
static bool sem_match (token_s **curr, int type);

static sem_type_s *alloc_semt(sem_type_s value);
static att_s *att_s_ (void *data, unsigned tid);
static void setatt(semantics_s *s, char *id, sem_type_s *data);
static sem_type_s getatt(semantics_s *s, char *id);
static void *sem_array(token_s **curr, semantics_s *s, pda_s *pda, pna_s *pna, parse_s *parse, sem_paramlist_s params, unsigned pass, sem_type_s *fill, bool eval, bool isfinal);
static void *sem_emit(token_s **curr, semantics_s *s, pda_s *pda, pna_s *pna, parse_s *parse, sem_paramlist_s params, unsigned pass, void *fill, bool eval, bool isfinal);
static void *sem_error(token_s **curr, semantics_s *s, pda_s *pda, pna_s *pna, parse_s *parse, sem_paramlist_s params, unsigned pass, void *fill, bool eval, bool isfinal);
static void *sem_getarray(token_s **curr, semantics_s *s, pda_s *pda, pna_s *pna, parse_s *parse, sem_paramlist_s params, unsigned pass, void *fill, bool eval, bool isfinal);
static void *sem_gettype(token_s **curr, semantics_s *s, pda_s *pda, pna_s *pna, parse_s *parse, sem_paramlist_s params, unsigned pass, void *fill, bool eval, bool isfinal);
static void *sem_halt(token_s **curr, semantics_s *s, pda_s *pda, pna_s *pna, parse_s *parse, sem_paramlist_s params, unsigned pass, void *fill, bool eval, bool isfinal);
static void *sem_lookup(token_s **curr, semantics_s *s, pda_s *pda, pna_s *pna, parse_s *parse, sem_paramlist_s params, unsigned pass, void *fill, bool eval, bool isfinal);
static void *sem_print(token_s **curr, semantics_s *s, pda_s *pda, pna_s *pna, parse_s *parse, sem_paramlist_s params, unsigned pass, void *fill, bool eval, bool isfinal);
static void *sem_addtype(token_s **curr, semantics_s *s, pda_s *pda, pna_s *pna, parse_s *parse, sem_paramlist_s params, unsigned pass, sem_type_s *type, bool eval, bool isfinal);
static void *sem_addarg(token_s **curr, semantics_s *s, pda_s *pda, pna_s *pna, parse_s *parse, sem_paramlist_s params, unsigned pass, sem_type_s *type, bool eval, bool isfinal);
static void *sem_listappend(token_s **curr, semantics_s *s, pda_s *pda, pna_s *pna, parse_s *parse, sem_paramlist_s params, unsigned pass, sem_type_s *type, bool eval, bool isfinal);
static void *sem_makelista(token_s **curr, semantics_s *s, pda_s *pda, pna_s *pna, parse_s *parse, sem_paramlist_s params, unsigned pass, sem_type_s *type, bool eval, bool isfinal);
static void *sem_makelistf(token_s **curr, semantics_s *s, pda_s *pda, pna_s *pna, parse_s *parse, sem_paramlist_s params, unsigned pass, sem_type_s *type, bool eval, bool isfinal);
static void *sem_pushscope(token_s **curr, semantics_s *s, pda_s *pda, pna_s *pna, parse_s *parse, sem_paramlist_s params, unsigned pass, sem_type_s *type, bool eval, bool isfinal);
static void *sem_popscope(token_s **curr, semantics_s *s, pda_s *pda, pna_s *pna, parse_s *parse, sem_paramlist_s params, unsigned pass, sem_type_s *type, bool eval, bool isfinal);
static void *sem_resettemps(token_s **curr, semantics_s *s, pda_s *pda, pna_s *pna, parse_s *parse, sem_paramlist_s params, unsigned pass, sem_type_s *type, bool eval, bool isfinal);
static void *sem_resolveproc(token_s **curr, semantics_s *s, pda_s *pda, pna_s *pna, parse_s *parse, sem_paramlist_s params, unsigned pass, sem_type_s *type, bool eval, bool isfinal);
static void *sem_getwidth(token_s **curr, semantics_s *s, pda_s *pda, pna_s *pna, parse_s *parse, sem_paramlist_s params, unsigned pass, sem_type_s *type, bool eval, bool isfinal);
static void *sem_low(token_s **curr, semantics_s *s, pda_s *pda, pna_s *pna, parse_s *parse, sem_paramlist_s params, unsigned pass, sem_type_s *type, bool eval, bool isfinal);

static int arglist_cmp(token_s **curr, parse_s *parse, token_s *tok, sem_type_s formal, sem_type_s actual);


static int ftable_strcmp(char *key, ftable_s *b);
static sem_action_f get_semaction(char *str);
static char *sem_tostring(sem_type_s type);
static char *semstr_concat(char *base, sem_type_s val);
static void set_type(semantics_s *s, char *id, sem_type_s type);
static sem_type_s get_type(semantics_s *s, char *id);
static char *make_semerror(unsigned lineno, char *lexeme, char *message);
static void add_semerror(parse_s *p, token_s *t, char *message);

static sem_type_s sem_newtemp(token_s **curr);
static sem_type_s sem_newlabel(token_s **curr);
static char *scoped_label(void);

static void write_code_(scope_s *s);

extern void print_semtype(sem_type_s value);

/* Must be alphabetized */
static ftable_s ftable[] = {
    {"addarg", (sem_action_f)sem_addarg},
    {"addtype", (sem_action_f)sem_addtype},
    {"array", (sem_action_f)sem_array},
    {"emit", sem_emit},
    {"error", sem_error},
    {"getarray", sem_getarray},
    {"gettype", sem_gettype},
    {"halt", sem_halt},
    {"listappend", (sem_action_f)sem_listappend},
    {"lookup", sem_lookup},
    {"low", (sem_action_f)sem_low},
    {"makelista", (sem_action_f)sem_makelista},
    {"makelistf", (sem_action_f)sem_makelistf},
    {"popscope", (sem_action_f)sem_popscope},
    {"print", sem_print},
    {"pushscope", (sem_action_f)sem_pushscope},
    {"resettemps", (sem_action_f)sem_resettemps},
    {"resolveproc", (sem_action_f)sem_resolveproc},
    {"width", (sem_action_f)sem_getwidth},
};

inline void grstack_push(void)
{
    hash_s *h = hash_(semgrammar_hashf, semgrammar_isequalf);
    llpush(&grammar_stack, h);
}

inline void grstack_pop(void)
{
    llist_s *l = llpop(&grammar_stack);
    free_hash(l->ptr);
    free(l);
}

uint16_t semgrammar_hashf(void *key)
{
    return (uint16_t)((intptr_t)key % HTABLE_SIZE);
}

bool semgrammar_isequalf(void *key1, void *key2)
{
    return key1 == key2;
}

sem_type_s sem_type_s_(parse_s *parse, token_s *token)
{
    sem_type_s s;
    tlookup_s res;
    regex_match_s match;
    
    
    s.str_ = NULL;
    s.low = 0 ;
    s.high = 0;
    
    if(!token->stype) {
        res = idtable_lookup(parse->lex->idtable, token->lexeme);
        if(res.is_found) {
            s = res.tdat.type;
            s.str_ = token->lexeme;
        }
        else {
            s.type = ATTYPE_CODE;
            if((match = lex_matches(parse->lex, "addop", token->lexeme)).matched) {
                puts("addop");
                switch(match.attribute){
                    case LEXATTR_PLUS:
                        s.str_ = "+";
                        break;
                    case LEXATTR_MINUS:
                        s.str_ = "-";
                        break;
                    case LEXATTR_OR:
                        s.str_ = "OR";
                        break;
                    default:
                        assert(false);
                        break;
                }
            }
            else if((match = lex_matches(parse->lex, "mulop", token->lexeme)).matched) {
                puts("mulop");
                switch(match.attribute){
                    case LEXATTR_MULT:
                        s.str_ = "*";
                        break;
                    case LEXATTR_DIV1:
                    case LEXATTR_DIV2:
                        s.str_ = "/";
                        break;
                    case LEXATTR_MOD:
                        s.str_ = "MOD";
                        break;
                    case LEXATTR_AND:
                        s.str_ = "AND";
                        break;
                    default:
                        assert(false);
                        break;
                }
            }
            else if((match = lex_matches(parse->lex, "relop", token->lexeme)).matched) {
                puts("relop");
                switch(match.attribute) {
                    case LEXATTR_EQ:
                        s.str_ = "=";
                        break;
                    case LEXATTR_NEQ:
                        s.str_ = "<>";
                        break;
                    case LEXATTR_LE:
                        s.str_ = "<";
                        break;
                    case LEXATTR_LEQ:
                        s.str_ = "<=";
                        break;
                    case LEXATTR_GEQ:
                        s.str_ = ">=";
                        break;
                    case LEXATTR_GE:
                        s.str_ = ">";
                        break;
                    default:
                        assert(false);
                        break;
                }
            }
            else if (!strcmp(token->lexeme, "integer")) {
                puts("integer 1");
                s.type = ATTYPE_ID;
            
                s.str_ = "integer";
            }
            else if (!strcmp(token->lexeme, "real")) {
                puts("real 1");
                s.type = ATTYPE_ID;
                s.str_ = "real";
            }
            else {
                puts("id 1");
                s.type = ATTYPE_ID;
                s.str_ = token->lexeme;
            }
        }
    }
    else if(!strcmp(token->stype, "integer")) {
        puts("integer");
        s.type = ATTYPE_ID;
        s.str_ = "integer";
    }
    else if(!strcmp(token->stype, "real")) {
        puts("real");
        s.type = ATTYPE_ID;
        s.str_ = "real";
    }
    else {
        puts("id");
        s.type = ATTYPE_ID;
        s.str_ = token->lexeme;
    }
    //s.lexeme = token->lexeme;
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
        case ATTYPE_TEMP:
        case ATTYPE_LABEL:
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
        case ATTYPE_VOID:
            printf("void");
            break;
        case ATTYPE_NOT_EVALUATED:
            printf("not evaluated");
            break;
        case ATTYPE_ARGLIST_FORMAL:
        case ATTYPE_ARGLIST_ACTUAL:
            printf("arg list");
            break;
        default:
            puts("ILLEGAL STATE");
            assert(false);
            /*illegal*/
            break;
    }
   // puts("\n~~~~~~~~~~~~~~~\n");
    fflush(stdout);
}

test_s test_semtype(sem_type_s value)
{
    if(value.type == ATTYPE_NOT_EVALUATED || value.type == ATTYPE_NULL)
        return (test_s){.evaluated = false, .result = false};
    return (test_s){.evaluated = true, .result = value.int_ || value.real_ || value.str_};
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
        printf("sem token: %s %d %u\n", iter->lexeme, iter->type.val, SEMTYPE_CODE);
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
sem_type_s sem_op(token_s **curr, parse_s *parse, token_s *tok, sem_type_s v1, sem_type_s v2, int op)
{
    sem_type_s result;
    result.str_ = NULL;
    if(op != OPTYPE_NOP) {
        if(v1.type == ATTYPE_NULL) {
            puts("COMPARING A NULL");
            if(op == OPTYPE_EQ) {
                result.type = ATTYPE_NUMINT;
                result.int_ = ((v2.type == ATTYPE_NOT_EVALUATED) || (v2.type == ATTYPE_NULL));
            }
            else if(op == OPTYPE_NE) {
                result.type = ATTYPE_NUMINT;
                result.int_ = !((v2.type == ATTYPE_NOT_EVALUATED) || (v2.type == ATTYPE_NULL));
            }
            result.tok = tok_lastmatched;
            return result;
        }
        if(v2.type == ATTYPE_NULL) {
            puts("COMPARING A NULL");
            if(op == OPTYPE_EQ) {
                result.type = ATTYPE_NUMINT;
                result.int_ = ((v1.type == ATTYPE_NOT_EVALUATED) || (v1.type == ATTYPE_NULL));
            }
            else if(op == OPTYPE_NE) {
                result.type = ATTYPE_NUMINT;
                result.int_ = !((v1.type == ATTYPE_NOT_EVALUATED) || (v1.type == ATTYPE_NULL));
            }
            result.tok = tok_lastmatched;
            return result;
        }
        if (v1.type == ATTYPE_NOT_EVALUATED || v2.type == ATTYPE_NOT_EVALUATED) {
            result.type = ATTYPE_NOT_EVALUATED;
            result.str_ = "null";
            puts("Not Evaluated");
            result.tok = tok_lastmatched;
            return result;
        }
        else if(v1.type == ATTYPE_NULL || v2.type == ATTYPE_NULL) {
            result.type = ATTYPE_NULL;
            result.str_ = "null";
            result.tok = tok_lastmatched;
      //      puts("Not Evaluated");
            return result;
        }
    }
    switch(op) {
        case OPTYPE_MULT:
            assert (v1.int_ != 28);
            if (v1.type == ATTYPE_ID || v2.type == ATTYPE_ID)
                fprintf(stderr, "Type Error: String type incompatible with multiplication.\n");
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
            if (v1.type == ATTYPE_ID || v2.type == ATTYPE_ID)
                fprintf(stderr, "Type Error: String type incompatible with division.\n");
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
            if (v1.type == ATTYPE_ID || v2.type == ATTYPE_ID)
                fprintf(stderr, "Type Error: Not yet implemented.\n");
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
            if (v1.type == ATTYPE_ID || v2.type == ATTYPE_ID)
                fprintf(stderr, "Type Error: String type incompatible with subtraction.\n");
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
            if (v1.type == ATTYPE_ID || v2.type == ATTYPE_ID)
                fprintf(stderr, "Type Error: String type incompatible with subtraction.\n");
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
            if (v1.type == ATTYPE_ID || v2.type == ATTYPE_ID)
                fprintf(stderr, "Type Error: String type incompatible with subtraction.\n");
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
            if (v1.type == ATTYPE_ID || v2.type == ATTYPE_ID)
                fprintf(stderr, "Type Error: String type incompatible with subtraction.\n");
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
            if (v1.type == ATTYPE_ID || v2.type == ATTYPE_ID)
                fprintf(stderr, "Type Error: String type incompatible with subtraction.\n");
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
            if ((v1.type == ATTYPE_CODE || v1.type == ATTYPE_ID || v1.type == ATTYPE_VOID) && (v2.type == ATTYPE_CODE || v2.type == ATTYPE_ID || v2.type == ATTYPE_VOID)) {
                result.int_ = !strcmp(v1.str_, v2.str_);
                if(v1.type != v2.type) {
                    if(v1.type == ATTYPE_CODE)
                        result.int_ = !quote_strcmp(v1.str_, v2.str_);
                    else if (v2.type == ATTYPE_CODE)
                        result.int_ = !quote_strcmp(v2.str_, v1.str_);
                }
            }
            else if (v1.type == ATTYPE_NUMINT && v2.type == ATTYPE_NUMINT)
                result.int_ = v1.int_ == v2.int_;
            else if (v1.type == ATTYPE_NUMINT && v2.type == ATTYPE_NUMREAL)
                result.int_ = v1.int_ == v2.real_;
            else if (v1.type == ATTYPE_NUMREAL && v2.type == ATTYPE_NUMINT)
                result.int_ = v1.real_ == v2.int_;
            else if(v1.type == ATTYPE_ARRAY || v2.type == ATTYPE_ARRAY) {
                result.int_ = (v2.type == ATTYPE_ARRAY && v1.low == v2.low && v1.high == v2.high);
            }
            else if((v1.type == ATTYPE_ARGLIST_FORMAL || v1.type == ATTYPE_ARGLIST_ACTUAL) || (v2.type == ATTYPE_ARGLIST_FORMAL || v2.type == ATTYPE_ARGLIST_ACTUAL)) {
                printf("\ntypes: %u %u v1: %u -- v2: %u\n", ATTYPE_ARGLIST_ACTUAL, ATTYPE_ARGLIST_FORMAL, v1.type, v2.type);
                if(v1.type == ATTYPE_ARGLIST_FORMAL)
                    result.int_ = arglist_cmp(curr, parse, tok, v1, v2);
                else
                    result.int_ = arglist_cmp(curr, parse, tok, v2, v1);
                result.int_ = 0;
            }
            else
                result.int_ = v1.real_ == v2.real_;
            break;
        case OPTYPE_NE:
            result.type = ATTYPE_NUMINT;
            result.int_ = 0;
            if ((v1.type == ATTYPE_CODE || v1.type == ATTYPE_ID || v1.type == ATTYPE_VOID) && (v2.type == ATTYPE_CODE || v2.type == ATTYPE_ID || v2.type == ATTYPE_VOID)) {
                result.int_ = !!strcmp(v1.str_, v2.str_);
                if(v1.type != v2.type) {
                    if(v1.type == ATTYPE_CODE)
                        result.int_ = !!quote_strcmp(v1.str_, v2.str_);
                    else if (v2.type == ATTYPE_CODE)
                        result.int_ = !!quote_strcmp(v2.str_, v1.str_);
                }
            }
            else if(v1.type == ATTYPE_NUMINT && v2.type == ATTYPE_NUMINT)
                result.int_ = v1.int_ != v2.int_;
            else if(v1.type == ATTYPE_NUMINT && v2.type == ATTYPE_NUMREAL)
                result.int_ = v1.int_ != v2.real_;
            else if(v1.type == ATTYPE_NUMREAL && v2.type == ATTYPE_NUMINT)
                result.int_ = v1.real_ != v2.int_;
            else if(v1.type == ATTYPE_ARRAY || v2.type == ATTYPE_ARRAY)
                result.int_ = !(v2.type == ATTYPE_ARRAY && v1.low == v2.low && v1.high == v2.high);
            else
                result.int_ = v1.real_ != v2.real_;
            break;
        case OPTYPE_OR:
            if (v1.type == ATTYPE_ID || v2.type == ATTYPE_ID)
                fprintf(stderr, "Type Error: String type incompatible with subtraction.\n");
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
            if (v1.type == ATTYPE_ID || v2.type == ATTYPE_ID)
                fprintf(stderr, "Type Error: String type incompatible with subtraction.\n");
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
    result.tok = tok_lastmatched;
    putchar('\n');
    return result;
}

llist_s *sem_start(semantics_s *in, parse_s *parse, mach_s *machs, pda_s *pda, production_s *prod, pna_s *pn, semantics_s *syn, unsigned pass, bool islast)
{
    token_s *iter = prod->annot;
    llist_s *ilist = NULL;
    
    if(!iter)
        return NULL;
    
    sem_statements(parse, &iter, &ilist, pda, prod, pn, syn, pass, (test_s){true, true}, false, islast);
    return ilist;
}

sem_statements_s sem_statements(parse_s *parse, token_s **curr, llist_s **il, pda_s *pda, production_s *prod, pna_s *pn, semantics_s *syn, unsigned pass, test_s evaluate, bool elprev, bool isfinal)
{
    switch((*curr)->type.val) {
        case SEMTYPE_IF:
        case SEMTYPE_NONTERM:
        case SEMTYPE_ID:
            sem_statement(parse, curr, il, pda, prod, pn, syn, pass, evaluate, false, isfinal);
            sem_statements(parse, curr, il, pda, prod, pn, syn, pass, evaluate, false, isfinal);
        case SEMTYPE_END:
        case SEMTYPE_ELSE:
        case SEMTYPE_ELIF:
        case LEXTYPE_EOF:
            break;
        default:
            fprintf(stderr, "Syntax Error at line %d: Expected if nonterm fi else or $ but got %s\n", (*curr)->lineno, (*curr)->lexeme);
            assert(false);
            break;
    }
}

sem_statement_s sem_statement(parse_s *parse, token_s **curr, llist_s **il, pda_s *pda, production_s *prod, pna_s *pn, semantics_s *syn, unsigned pass, test_s evaluate, bool elprev, bool isfinal)
{
    pnode_s *p;
    char *id, *nterm;
    unsigned index;
    sem_expression_s expression;
    sem_idsuffix_s idsuffix;
    sem_paramlist_s params;
    semantics_s *in = NULL;
    test_s test;
    
    switch((*curr)->type.val) {
        case SEMTYPE_NONTERM:
            nterm = (*curr)->lexeme;
            *curr = (*curr)->next;
            idsuffix = sem_idsuffix(parse, curr, il, pda, prod, pn, syn, pass, evaluate.evaluated && evaluate.result, isfinal);
            index = idsuffix.factor_.index;
            id = idsuffix.dot.id;
            sem_match(curr, SEMTYPE_ASSIGNOP);
            expression = sem_expression(parse, curr, il, pda, prod, pn, syn, pass, evaluate.evaluated && evaluate.result, isfinal);
            
            if (evaluate.result && evaluate.evaluated && expression.value.type != ATTYPE_NOT_EVALUATED) {

                if(!strcmp(pda->nterm->lexeme, nterm) && !idsuffix.factor_.isset) {
                    if(syn) {
                        setatt(syn, id, alloc_semt(expression.value));
                    }
                    else {
                        //Nasty hack to fix the attribute system when assigning data to the current nonterminal production.
                      /*  if(pn->curr) {
                            if(!pn->curr->in) {
                                pn->curr->in = semantics_s_(parse, NULL);
                                pn->curr->in->n = malloc(sizeof(*pn->curr->in->n));
                                if(!pn->curr->in->n) {
                                    perror("Memory Allocation Error");
                                    exit(EXIT_FAILURE);
                                }
                                pn->curr->in->n->token = pda->nterm;
                            }
                            setatt(pn->curr->in, id, alloc_semt(expression.value));
                        }*/
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
            expression = sem_expression(parse, curr, il, pda, prod, pn, syn, pass, evaluate.evaluated && evaluate.result, isfinal);
            sem_match(curr, SEMTYPE_THEN);
            test = test_semtype(expression.value);
            sem_statements(parse, curr, il, pda, prod, pn, syn, pass, (test_s){.evaluated = test.evaluated && evaluate.evaluated, .result = test.result && evaluate.result}, false, isfinal);
            evaluate.evaluated = evaluate.evaluated && test.evaluated;
            sem_else(parse, curr, il, pda, prod, pn, syn, pass, evaluate, test.result, isfinal);
            break;
        case SEMTYPE_ID:
            id = (*curr)->lexeme;
            *curr = (*curr)->next;
            sem_match(curr, SEMTYPE_OPENPAREN);
            params = sem_paramlist(parse, curr, il, pda, prod, pn, syn, pass, evaluate.evaluated && evaluate.result, isfinal);
            sem_match(curr, SEMTYPE_CLOSEPAREN);
            if (evaluate.result && evaluate.evaluated && params.ready) {
                get_semaction(id)(curr, NULL, pda, pn, parse, params, pass, &expression, evaluate.evaluated && evaluate.result, isfinal);
            }
            break;
        default:
            fprintf(stderr, "Syntax Error at line %d: Expected nonterm or if but got %s", (*curr)->lineno, (*curr)->lexeme);
            assert(false);
            break;
    }
}

sem_else_s sem_else(parse_s *parse, token_s **curr, llist_s **il, pda_s *pda, production_s *prod, pna_s *pn, semantics_s *syn, unsigned pass, test_s evaluate, bool elprev, bool isfinal)
{
    switch((*curr)->type.val) {
        case SEMTYPE_ELSE:
            *curr = (*curr)->next;
            sem_statements(parse, curr, il, pda, prod, pn, syn, pass, (test_s){.evaluated = evaluate.evaluated && evaluate.result, .result = !elprev}, false, isfinal);
            sem_match(curr, SEMTYPE_END);
            break;
        case SEMTYPE_END:
            *curr = (*curr)->next;
            break;
        case SEMTYPE_ELIF:
            sem_elif(parse, curr, il, pda, prod, pn, syn, pass, evaluate, elprev, isfinal);
            break;
        default:
            fprintf(stderr, "Syntax Error at line %d: Expected else or fi but got %s\n", (*curr)->lineno, (*curr)->lexeme);
            assert(false);
            break;
    }
}

sem_elif_s sem_elif(parse_s *parse, token_s **curr, llist_s **il, pda_s *pda, production_s *prod, pna_s *pn, semantics_s *syn, unsigned pass, test_s evaluate, bool elprev, bool isfinal)
{
    test_s test;
    sem_expression_s expression;
    
    sem_match(curr, SEMTYPE_ELIF);
    expression = sem_expression(parse, curr, il, pda, prod, pn, syn, pass, evaluate.evaluated && evaluate.result, isfinal);
    sem_match(curr, SEMTYPE_THEN);
    test = test_semtype(expression.value);
    sem_statements(parse, curr, il, pda, prod, pn, syn, pass, (test_s){.evaluated = test.evaluated && evaluate.evaluated, .result = evaluate.result && test.result && !elprev}, false, isfinal);
    sem_else(parse, curr, il, pda, prod, pn, syn, pass, (test_s){.evaluated = test.evaluated && evaluate.evaluated, .result = evaluate.result}, test.result || elprev, isfinal);
}

sem_expression_s sem_expression(parse_s *parse, token_s **curr, llist_s **il, pda_s *pda, production_s *prod, pna_s *pn, semantics_s *syn, unsigned pass, bool eval, bool isfinal)
{
    sem_expression_s expression;
    sem_expression__s expression_;
    sem_simple_expression_s simple_expression;
    
    expression.value.str_= NULL;
    expression.value.str_ = NULL;
    expression.value.tok = NULL;
    simple_expression.value.str_ = NULL;
    simple_expression.value.tok = NULL;
    simple_expression = sem_simple_expression(parse, curr, il, pda, prod, pn, syn, pass, eval, isfinal);
    expression_ = sem_expression_(parse, curr, il, pda, prod, pn, syn, pass, eval, isfinal);
    if (expression_.op != OPTYPE_NOP) {
       /* printf("Comparing:\n");
        print_semtype(simple_expression.value);
        puts("   ");
        print_semtype(expression_.value);
        //if (expression_.value.type == ATTYPE_ID)
           // for(;;)printf("%u\n", expression_.value.type);
        puts("\n\n");*/
    }
    expression.value = sem_op(curr, parse, tok_lastmatched, simple_expression.value, expression_.value, expression_.op);
    
    if(!expression.value.tok) {
        expression.value.tok = expression_.value.tok;
        if(!expression.value.tok)
            expression.value.tok = simple_expression.value.tok;
    }
    while(!expression.value.tok);
    return expression;
}

sem_expression__s sem_expression_(parse_s *parse, token_s **curr, llist_s **il, pda_s *pda, production_s *prod, pna_s *pn, semantics_s *syn, unsigned pass, bool eval, bool isfinal)
{
    sem_expression__s expression_;
    
    expression_.value.tok = NULL;
    expression_.value.str_ = NULL;
    switch((*curr)->type.val) {
        case SEMTYPE_RELOP:
            expression_.op = torelop((*curr)->type.attribute);
            *curr = (*curr)->next;
            expression_.value = sem_simple_expression(parse, curr, il, pda, prod, pn, syn, pass, eval, isfinal).value;
            break;
        case SEMTYPE_COMMA:
        case SEMTYPE_END:
        case SEMTYPE_ELSE:
        case SEMTYPE_THEN:
        case SEMTYPE_IF:
        case SEMTYPE_NONTERM:
        case SEMTYPE_CLOSEPAREN:
        case SEMTYPE_CLOSEBRACKET:
        case SEMTYPE_ID:
        case SEMTYPE_ELIF:
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

sem_simple_expression_s sem_simple_expression(parse_s *parse, token_s **curr, llist_s **il, pda_s *pda, production_s *prod, pna_s *pn, semantics_s *syn, unsigned pass, bool eval, bool isfinal)
{
    sem_sign_s sign;
    sem_simple_expression_s simple_expression;
    sem_term_s term;
    sem_simple_expression__s simple_expression_;
    
    simple_expression.value.str_ = NULL;
    simple_expression.value.tok = NULL;
    term.value.str_ = NULL;
    term.value.tok = NULL;
    simple_expression_.value.str_ = NULL;
    simple_expression_.value.tok = NULL;
    switch((*curr)->type.val) {
        case SEMTYPE_ADDOP:
            sign = sem_sign(curr);
            simple_expression = sem_simple_expression(parse, curr, il, pda, prod, pn, syn, pass, eval, isfinal);
            if (sign.value == SEMSIGN_NEG) {
                if(simple_expression.value.type == ATTYPE_NUMINT)
                    simple_expression.value.int_ = -simple_expression.value.int_;
                else if(simple_expression.value.type == ATTYPE_NUMREAL)
                    simple_expression.value.real_ = -simple_expression.value.real_;
                else if(simple_expression.value.type != ATTYPE_NOT_EVALUATED)
                    fprintf(stderr, "Type Error: Cannot negate id types\n");
            }
            break;
        case SEMTYPE_NOT:
        case SEMTYPE_NUM:
        case SEMTYPE_ID:
        case SEMTYPE_NONTERM:
        case SEMTYPE_OPENPAREN:
        case SEMTYPE_CODE:
            term = sem_term(parse, curr, il, pda, prod, pn, syn, pass, eval, isfinal);
            simple_expression_ = sem_simple_expression_(parse, curr, il, &term.value, pda, prod, pn, syn, pass, eval, isfinal);
            simple_expression.value = term.value;
            if(!simple_expression.value.tok)
                simple_expression.value.tok = simple_expression_.value.tok;
            break;
        default:
            fprintf(stderr, "Syntax Error at line %d: Expected + - not number or identifier but got %s\n", (*curr)->lineno, (*curr)->lexeme);
            assert(false);
            break;
    }
    return simple_expression;
}

sem_simple_expression__s sem_simple_expression_(parse_s *parse, token_s **curr, llist_s **il, sem_type_s *accum, pda_s *pda, production_s *prod, pna_s *pn, semantics_s *syn, unsigned pass, bool eval, bool isfinal)
{
    unsigned op;
    sem_term_s term;
    sem_simple_expression__s simple_expression_, simple_expression__;
    
    term.value.str_ = NULL;
    term.value.tok = NULL;
    simple_expression_.value.str_ = NULL;
    simple_expression_.value.tok = NULL;
    simple_expression__.value.str_ = NULL;
    simple_expression__.value.tok = NULL;
    switch((*curr)->type.val) {
        case SEMTYPE_ADDOP:
            op = toaddop((*curr)->type.attribute);
            *curr = (*curr)->next;
            term = sem_term(parse, curr, il, pda, prod, pn, syn, pass, eval, isfinal);
            *accum = sem_op(curr, parse, tok_lastmatched, *accum, term.value, op);
            simple_expression__ = sem_simple_expression_(parse, curr, il, accum, pda, prod, pn, syn, pass, eval, isfinal);
            if(!simple_expression_.value.tok) {
                simple_expression_.value.tok = simple_expression__.value.tok;
                if(!simple_expression_.value.tok)
                    simple_expression_.value.tok = term.value.tok;
            }
            break;
        case SEMTYPE_COMMA:
        case SEMTYPE_RELOP:
        case SEMTYPE_END:
        case SEMTYPE_ELSE:
        case SEMTYPE_THEN:
        case SEMTYPE_IF:
        case SEMTYPE_NONTERM:
        case SEMTYPE_CLOSEPAREN:
        case SEMTYPE_CLOSEBRACKET:
        case SEMTYPE_ID:
        case SEMTYPE_ELIF:
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

sem_term_s sem_term(parse_s *parse, token_s **curr, llist_s **il, pda_s *pda, production_s *prod, pna_s *pn, semantics_s *syn, unsigned pass, bool eval, bool isfinal)
{
    sem_term_s term;
    sem_factor_s factor;
    sem_term__s term_;
    
    term.value.str_ = NULL;
    term.value.tok = NULL;
    factor.value.str_= NULL;
    factor.value.tok = NULL;
    term_.value.str_ = NULL;
    term_.value.tok = NULL;
    factor = sem_factor(parse, curr, il, pda, prod, pn, syn, pass, eval, isfinal);
    term_ = sem_term_(parse, curr, il, &factor.value, pda, prod, pn, syn, pass, eval, isfinal);
    term.value = factor.value;
    if(!term.value.tok)
        term.value.tok = term_.value.tok;
    return term;
}

sem_term__s sem_term_(parse_s *parse, token_s **curr, llist_s **il, sem_type_s *accum, pda_s *pda, production_s *prod, pna_s *pn, semantics_s *syn, unsigned pass, bool eval, bool isfinal)
{
    unsigned op;
    sem_factor_s factor;
    sem_term__s term_, term__;
    
    factor.value.str_ = NULL;
    term_.value.str_ = NULL;
    term__.value.str_ = NULL;
    term_.value.tok = NULL;
    term__.value.tok = NULL;
    switch((*curr)->type.val) {
        case SEMTYPE_MULOP:
            op = tomulop((*curr)->type.attribute);
            *curr = (*curr)->next;
            factor = sem_factor(parse, curr, il, pda, prod, pn, syn, pass, eval, isfinal);
            *accum = sem_op(curr, parse, tok_lastmatched, *accum, factor.value, op);
            term_ = sem_term_(parse, curr, il, accum, pda, prod, pn, syn, pass, eval, isfinal);
            if(!term_.value.tok) {
                term_.value.tok = factor.value.tok;
                if(!term_.value.tok)
                    term_.value.tok = accum->tok;
            }
            break;
        case SEMTYPE_COMMA:
        case SEMTYPE_ADDOP:
        case SEMTYPE_RELOP:
        case SEMTYPE_END:
        case SEMTYPE_ELSE:
        case SEMTYPE_THEN:
        case SEMTYPE_IF:
        case SEMTYPE_NONTERM:
        case SEMTYPE_CLOSEPAREN:
        case SEMTYPE_CLOSEBRACKET:
        case SEMTYPE_ID:
        case SEMTYPE_ELIF:
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

sem_factor_s sem_factor(parse_s *parse, token_s **curr, llist_s **il, pda_s *pda, production_s *prod, pna_s *pn, semantics_s *syn, unsigned pass, bool eval, bool isfinal)
{
    long difference;
    token_s *id;
    pnode_s *pnode, *ptmp;
    semantics_s *in;
    sem_type_s value;
    sem_factor_s factor = {0};
    sem_idsuffix_s idsuffix;
    sem_expression_s expression;
    
    factor.value.tok = NULL;
    expression.value.str_ = NULL;
    value.str_ = NULL;
    switch((*curr)->type.val) {
        case SEMTYPE_ID:
            id = *curr;
            *curr = (*curr)->next;
            idsuffix = sem_idsuffix(parse, curr, il, pda, prod, pn, syn, pass, eval, isfinal);
            //attadd (semantics_s *s, char *id, sem_type_s *data)
            if (idsuffix.dot.id) {
                if (!strcmp(idsuffix.dot.id, "entry")) {
                    pnode = getpnode_token(pn, id->lexeme, idsuffix.factor_.index);
                    if(pnode && pnode->pass) {
                        factor.value.str_ = pnode->matched->lexeme;
                        factor.value.lexeme = pnode->matched->lexeme;
                        factor.value.type = ATTYPE_ID;
                        factor.value.tok = pnode->matched;
                    }
                    if (idsuffix.dot.range.isset && idsuffix.dot.range.isready) {
                        factor.value.type = ATTYPE_RANGE;
                        pnode = getpnode_token(pn, id->lexeme, idsuffix.factor_.index);
                        if(pnode && pnode->pass) {
                            factor.value.low = safe_atol(pnode->matched->lexeme);
                            factor.value.high = idsuffix.dot.range.value;
                            factor.value.tok = pnode->matched;
                            difference = factor.value.high - factor.value.low;
                            if(difference < 0)
                                add_semerror(parse, pnode->matched, "Invalid array range. Upper bound is less than lower bound.");
                        }
                    }

                }
                else if (!strcmp(idsuffix.dot.id, "val")) {
                    pnode = getpnode_token(pn, id->lexeme, idsuffix.factor_.index);
                    if (idsuffix.dot.range.isset && idsuffix.dot.range.isready) {
                        factor.value.type = ATTYPE_RANGE;
                        pnode = getpnode_token(pn, id->lexeme, idsuffix.factor_.index);
                        if(pnode && pnode->pass) {
                            factor.value.low = safe_atol(pnode->matched->lexeme);
                            factor.value.high = idsuffix.dot.range.value;
                            factor.value.tok = pnode->matched;
                            difference = factor.value.high - factor.value.low;
                            if(difference < 0)
                                add_semerror(parse, pnode->matched, "Invalid array range. Upper bound is less than lower bound.");
                        }
                    }
                    else {
                        if(pnode->matched->stype) {
                            if(!strcmp(pnode->matched->stype, "integer")) {
                                factor.value.type = ATTYPE_NUMINT;
                                factor.value.int_ = safe_atol(pnode->matched->lexeme);
                            }
                            else if(!strcmp(pnode->matched->stype, "real")) {
                                factor.value.type = ATTYPE_NUMREAL;
                                factor.value.real_ = safe_atod(pnode->matched->lexeme);
                            }
                            factor.value.tok = pnode->matched;
                        }
                    }


                }
                else if(!strcmp(idsuffix.dot.id, "type")) {
                    ptmp = getpnode_token(pn, id->lexeme, 1);
                    factor.value = sem_type_s_(parse, ptmp->matched);
                    factor.value.tok = ptmp->matched;
                }
                else {
                    factor.value.str_ = id->lexeme;
                    factor.value.lexeme = id->lexeme;
                    factor.value.type = ATTYPE_ID;
                    factor.value.tok = id;
                }
            }
            else if (idsuffix.hasparam) {
                if(idsuffix.params.ready) {
                    factor.value = *(sem_type_s *)get_semaction(id->lexeme)(curr, NULL, pda, pn, parse, idsuffix.params, pass, &factor.value, eval, isfinal);
                }
                if(!factor.value.tok) {
                    factor.value.tok = tok_lastmatched;
                }
            }
            else {
                if(!strcmp(id->lexeme, "null")) {
                    factor.value.type = ATTYPE_NULL;
                    factor.value.str_ = "null";
                }
                else if(!strcmp(id->lexeme, "void")) {
                    factor.value.type = ATTYPE_VOID;
                    factor.value.str_ = "void";
                }
                else if(!strcmp(id->lexeme, "newtemp")) {
                    factor.value = sem_newtemp(curr);
                }
                else if(!strcmp(id->lexeme, "newlabel")) {
                    factor.value = sem_newlabel(curr);
                }
                else {
                    factor.value.str_ = id->lexeme;
                    factor.value.lexeme = id->lexeme;
                    factor.value.type = ATTYPE_ID;
                }
                factor.value.tok = tok_lastmatched;
            }
            break;
        case SEMTYPE_NONTERM:
            factor.value.type = ATTYPE_ID;
            factor.value.str_ = (*curr)->lexeme;
            factor.access.base = (*curr)->lexeme;
            *curr = (*curr)->next;
            idsuffix = sem_idsuffix(parse, curr, il, pda, prod, pn, syn, pass, eval, isfinal);
            factor.access.offset = idsuffix.factor_.index;
            factor.access.attribute = idsuffix.dot.id;
            if (idsuffix.dot.id) {
                if (!strcmp(factor.value.str_, pda->nterm->lexeme) && !idsuffix.factor_.isset) {
                    if(pn->curr) {
                        factor.value = getatt(syn, idsuffix.dot.id);

                        if(factor.value.type == ATTYPE_NOT_EVALUATED) {
                            factor.value = getatt(pn->curr->in, idsuffix.dot.id);
                            if(factor.value.type == ATTYPE_NOT_EVALUATED) {
                                factor.value = getatt(pn->curr->syn, idsuffix.dot.id);

                            }
                        }
                        if(!factor.value.tok)
                            factor.value.tok = tok_lastmatched;
                    }
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
            factor.value.tok = tok_lastmatched;
            *curr = (*curr)->next;
            break;
        case SEMTYPE_NOT:
            *curr = (*curr)->next;
            id = *curr;
            factor = sem_factor(parse, curr, il, pda, prod, pn, syn, pass, eval, isfinal);
            switch(factor.value.type) {
                case ATTYPE_ID:
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
            expression = sem_expression(parse, curr, il, pda, prod, pn, syn, pass, eval, isfinal);
            sem_match(curr, SEMTYPE_CLOSEPAREN);
            factor.value = expression.value;
            break;
        case SEMTYPE_CODE:
            factor.value.type = ATTYPE_CODE;
            factor.value.str_ = (*curr)->lexeme_;
            factor.value.tok = tok_lastmatched;
            *curr = (*curr)->next;
            break;
        default:
            fprintf(stderr, "Syntax Error at line %d: Expected identifier nonterm number or not but got %s\n", (*curr)->lineno, (*curr)->lexeme);
            assert(false);
            break;
    }
    return factor;
}

sem_factor__s sem_factor_(parse_s *parse, token_s **curr, llist_s **il, pna_s *pn, semantics_s *syn, unsigned pass, bool eval, bool isfinal)
{
    sem_factor__s factor_;

    switch((*curr)->type.val) {
        case SEMTYPE_NUM:
            factor_.isset = true;
            factor_.index = (unsigned)safe_atol((*curr)->lexeme);
            *curr = (*curr)->next;
            break;
        case SEMTYPE_MULOP:
        case SEMTYPE_ADDOP:
        case SEMTYPE_RELOP:
        case SEMTYPE_END:
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
        case SEMTYPE_ELIF:
        case LEXTYPE_EOF:
            factor_.isset = false;
            factor_.index = 1;
            break;
        default:
            fprintf(stderr, "Syntax Error at line %d: Expected [ ] * / + - = < > <> <= >= fi else then if nonterm or $ but got %s\n", (*curr)->lineno, (*curr)->lexeme);
            assert(false);
            break;
    }
    return factor_;
}

sem_idsuffix_s sem_idsuffix(parse_s *parse, token_s **curr, llist_s **il, pda_s *pda, production_s *prod, pna_s *pn, semantics_s *syn, unsigned pass, bool eval, bool isfinal)
{
    sem_expression_s expression;
    sem_idsuffix_s idsuffix;
    
    expression.value.str_ = NULL;
    switch ((*curr)->type.val) {
        case SEMTYPE_COMMA:
        case SEMTYPE_CLOSEPAREN:
        case SEMTYPE_CLOSEBRACKET:
        case SEMTYPE_MULOP:
        case SEMTYPE_ADDOP:
        case SEMTYPE_RELOP:
        case SEMTYPE_END:
        case SEMTYPE_ELSE:
        case SEMTYPE_THEN:
        case SEMTYPE_IF:
        case SEMTYPE_DOT:
        case SEMTYPE_NONTERM:
        case SEMTYPE_NUM:
        case SEMTYPE_ID:
        case SEMTYPE_ELIF:
        case LEXTYPE_EOF:
            idsuffix.factor_ = sem_factor_(parse, curr, il, pn, syn, pass, eval, isfinal);
            idsuffix.dot = sem_dot(parse, curr, il, pn, syn, pass, eval, isfinal);
            idsuffix.hasparam = false;
            idsuffix.hasmap = false;
            break;
        case SEMTYPE_OPENPAREN:
            *curr = (*curr)->next;
            idsuffix.hasparam = true;
            idsuffix.hasmap = false;
            idsuffix.params = sem_paramlist(parse, curr, il, pda, prod, pn, syn, pass, eval, isfinal);
            idsuffix.factor_.index = 1;
            idsuffix.factor_.isset = false;
            idsuffix.dot.id = NULL;
            sem_match(curr, SEMTYPE_CLOSEPAREN);
            break;
        case SEMTYPE_OPENBRACKET:
            *curr = (*curr)->next;
            expression = sem_expression(parse, curr, il, pda, prod, pn, syn, pass, eval, isfinal);
            sem_match(curr, SEMTYPE_CLOSEBRACKET);
            idsuffix.factor_.isset = true;
	    idsuffix.factor_.index = 1;
            idsuffix.dot = sem_dot(parse, curr, il, pn, syn, pass, eval, isfinal);
            idsuffix.hasparam = false;
            idsuffix.hasmap = false;
            break;
        case SEMTYPE_MAP:
            *curr = (*curr)->next;
            idsuffix.hasmap = true;
            idsuffix.hasparam = false;
            idsuffix.dot.id = NULL;
            idsuffix.factor_.isset = false;
	    break;
        default:
            fprintf(stderr, "Syntax Error at line %d: Expected , ] [ * / + - = < > <> <= >= fi else then if . nonterm or $ but got %s\n", (*curr)->lineno, (*curr)->lexeme);
            assert(false);
            break;
    }
    return idsuffix;
}

sem_dot_s sem_dot(parse_s *parse, token_s **curr, llist_s **il, pna_s *pn, semantics_s *syn, unsigned pass, bool eval, bool isfinal)
{
    sem_dot_s dot;
    
    switch((*curr)->type.val) {
        case SEMTYPE_DOT:
            *curr = (*curr)->next;
            dot.id = (*curr)->lexeme;
            sem_match(curr, SEMTYPE_ID);
            dot.range = sem_range(parse, curr, il, pn, syn, pass, eval, isfinal);
            break;
        case SEMTYPE_MULOP:
        case SEMTYPE_ADDOP:
        case SEMTYPE_RELOP:
        case SEMTYPE_END:
        case SEMTYPE_ELSE:
        case SEMTYPE_THEN:
        case SEMTYPE_IF:
        case SEMTYPE_NONTERM:
        case SEMTYPE_CLOSEPAREN:
        case SEMTYPE_CLOSEBRACKET:
        case SEMTYPE_COMMA:
        case SEMTYPE_ID:
        case SEMTYPE_ASSIGNOP:
        case SEMTYPE_ELIF:
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

sem_range_s sem_range(parse_s *parse, token_s **curr, llist_s **il, pna_s *pn, semantics_s *syn, unsigned pass, bool eval, bool isfinal)
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
        case SEMTYPE_END:
        case SEMTYPE_CLOSEPAREN:
        case SEMTYPE_ID:
        case SEMTYPE_ELSE:
        case SEMTYPE_THEN:
        case SEMTYPE_IF:
        case SEMTYPE_ASSIGNOP:
        case SEMTYPE_NONTERM:
        case SEMTYPE_ELIF:
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

sem_paramlist_s sem_paramlist(parse_s *parse, token_s **curr, llist_s **il, pda_s *pda, production_s *prod, pna_s *pn, semantics_s *syn, unsigned pass, bool eval, bool isfinal)
{
    sem_paramlist_s paramlist;
    sem_expression_s expression;

    expression.value.str_ = NULL;
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
            expression = sem_expression(parse, curr, il, pda, prod, pn, syn, pass, eval, isfinal);
            if(expression.value.type == ATTYPE_NOT_EVALUATED || expression.value.type == ATTYPE_NULL)
                paramlist.ready = false;
            else
                llpush(&paramlist.pstack, alloc_semt(expression.value));
            sem_paramlist_(parse, curr, il, &paramlist, pda, prod, pn, syn, pass, eval, isfinal);
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

void sem_paramlist_(parse_s *parse, token_s **curr, llist_s **il, sem_paramlist_s *list, pda_s *pda, production_s *prod, pna_s *pn, semantics_s *syn, unsigned pass, bool eval, bool isfinal)
{
    sem_expression_s expression;
    
    switch ((*curr)->type.val) {
        case SEMTYPE_COMMA:
            *curr = (*curr)->next;
            expression = sem_expression(parse, curr, il, pda, prod, pn, syn, pass, eval, isfinal);
            if(expression.value.type == ATTYPE_NOT_EVALUATED || expression.value.type == ATTYPE_NULL)
                list->ready = false;
            else if (list->ready)
                llpush(&list->pstack, alloc_semt(expression.value));
            sem_paramlist_(parse, curr, il, list, pda, prod, pn, syn, pass, eval, isfinal);
            break;
        case SEMTYPE_CLOSEPAREN:
            break;
        default:
            fprintf(stderr, "Syntax Error at line %d: Expected , or ( but got %s\n", (*curr)->lineno, (*curr)->lexeme);
            assert(false);
            break;
    }
}

sem_sign_s sem_sign(token_s **curr)
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

bool sem_match(token_s **curr, int type)
{
    if ((*curr)->type.val == type) {
        *curr = (*curr)->next;
        return true;
    }
    fprintf(stderr, "Syntax Error at line %d: Got %s\n", (*curr)->lineno, (*curr)->lexeme);
    asm("hlt");
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

att_s *att_s_(void *data, unsigned tid)
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

void setatt(semantics_s *s, char *id, sem_type_s *data)
{
    if(data->type != ATTYPE_NOT_EVALUATED && data->type != ATTYPE_NULL) {
        hashinsert_(s->table, id, data);
    }
}

sem_type_s getatt(semantics_s *s, char *id)
{
    sem_type_s dummy;
    sem_type_s *data;
    
    dummy.type = ATTYPE_NOT_EVALUATED;
    dummy.str_ = NULL;
    if(!s)
        return dummy;
    
    data = hashlookup(s->table, id);
    if (!data) {
        puts("Access to unitialized attribute.");
        return dummy;
    }
    return *data;
}

void *sem_array(token_s **curr, semantics_s *s, pda_s *pda, pna_s *pn, parse_s *p, sem_paramlist_s params, unsigned pass, sem_type_s *fill, bool eval, bool isfinal)
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

void *sem_emit(token_s **curr, semantics_s *s, pda_s *pda, pna_s *pn, parse_s *parse, sem_paramlist_s params, unsigned pass, void *fill, bool eval, bool isfinal)
{
    int c;
    size_t lsize = 32;
    char *out = NULL;
    size_t len;
    llist_s *iter;
    sem_type_s *dummy;
    sem_type_s *val;
    bool gotfirst = false, gotlabelf = false, gotlabell = false;
    char *line = NULL;
    
    if(hashlookup(grammar_stack->ptr, *curr))
        return NULL;
    
    if(params.ready && eval) {
        llreverse(&params.pstack);
        
        while((iter = llpop(&params.pstack))) {
            val = iter->ptr;
            free(iter);
            
            
            if(!gotfirst) {
                if(val->type == ATTYPE_ID) {
                    if(!strcmp(val->str_, "labelf")) {
                        gotfirst = true;
                        gotlabelf = true;
                        continue;
                    }
                    else if(!strcmp(val->str_, "label")) {
                        gotfirst = true;
                        gotlabell = true;
                        continue;
                    }
                }
                safe_addstring(&line, "\t");
                gotfirst = true;
            }
            
            switch(val->type) {
                case ATTYPE_CODE:
                    len = strlen(val->str_);
                    out = malloc(len-1);
                    if(!out){
                        perror("Memory Allocation Error");
                        exit(EXIT_FAILURE);
                    }
                    c = val->str_[len-1];
                    val->str_[len-1] = '\0';
                    strcpy(out, &val->str_[1]);
                    val->str_[len-1] = c;
                    safe_addstring(&line, out);
                    free(out);
                    break;
                case ATTYPE_NUMINT:
                    safe_addint(&line, val->int_);
                    break;
                case ATTYPE_NUMREAL:
                    safe_adddouble(&line, val->real_);
                    break;
                case ATTYPE_ID:
                case ATTYPE_TEMP:
                case ATTYPE_LABEL:
                    if(gotlabelf) {
                        safe_addstring(&line, scope_tree->full_id);
                        gotlabelf = false;
                    }
                    else {
                        safe_addstring(&line, val->str_);
                    }
                    
                    break;
                default:
                    break;
            }
            
        }
        addline(&scope_tree->code, line);
        dummy = (sem_type_s *)1;
        hashinsert(grammar_stack->ptr, *curr, dummy);
    }
    return NULL;
}

void *sem_error(token_s **curr, semantics_s *s, pda_s *pda, pna_s *pn, parse_s *parse, sem_paramlist_s params, unsigned pass, void *fill, bool eval, bool isfinal)
{
    char *str;
    pnode_s *p;
    llist_s *node;
    sem_type_s *val, *id;
    
    if(hashlookup(grammar_stack->ptr, *curr))
        return NULL;
    
    if(params.ready && eval) {
        node = llpop(&params.pstack);
        val = node->ptr;
        free(node);
        
        node = llpop(&params.pstack);
        id = node->ptr;
        free(node);
        
        str = val->str_;
        str[0] = ' ';
        str[strlen(str)-1] = ' ';
        p = getpnode_nterm_copy(pn, id->str_, 1);
        if(p)
            add_semerror(parse, p->matched, str);
        else
            add_semerror(parse, tok_lastmatched, str);
        hashinsert(grammar_stack->ptr, *curr, val);
    }
    return NULL;
}

void *sem_getarray(token_s **curr, semantics_s *s, pda_s *pda, pna_s *pn, parse_s *parse, sem_paramlist_s params, unsigned pass, void *fill, bool eval, bool isfinal)
{
    pnode_s *p;
    llist_s *node;
    sem_type_s *val, type;
    check_id_s check;
    
    if(params.ready && eval) {
        node = llpop(&params.pstack);
        val = node->ptr;
        free(node);
        
        p = getpnode_nterm_copy(pn, val->str_, 1);
        type = gettype(parse->lex, p->matched->lexeme);
        
        if(type.type == ATTYPE_ARRAY) {
            type.type = ATTYPE_ID;
            check = check_id(p->matched->lexeme);
            if(!check.isfound && isfinal) {
                check = check_id(p->matched->lexeme);
                add_semerror(parse, p->matched, "undeclared identifier");
            }
        }
        else if(eval) {
            if(type.type == ATTYPE_NULL)
                add_semerror(parse, p->matched, "undeclared identifier");
            else
                add_semerror(parse, p->matched, "attempt to index non-array identifier");
        }
    }
    return alloc_semt(type);
}

void *sem_gettype(token_s **curr, semantics_s *s, pda_s *pda, pna_s *pn, parse_s *parse, sem_paramlist_s params, unsigned pass, void *fill, bool eval, bool isfinal)
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

void *sem_halt(token_s **curr, semantics_s *s, pda_s *pda, pna_s *pn, parse_s *p, sem_paramlist_s params, unsigned pass, void *fill, bool eval, bool isfinal)
{
    if(!eval)
        return NULL;
    
    printf("Halt Called in %s\n", pda->nterm->lexeme);
    fflush(stderr);
    fflush(stdout);
  //  assert(false);
    asm("hlt");
}

void *sem_lookup(token_s **curr, semantics_s *s, pda_s *pda, pna_s *pn, parse_s *parse, sem_paramlist_s params, unsigned pass, void *fill, bool eval, bool isfinal)
{
    pnode_s *p;
    llist_s *node;
    sem_type_s *val, type;
        
    if(params.ready) {
        node = llpop(&params.pstack);
        val = node->ptr;
        free(node);
        
        p = getpnode_nterm_copy(pn, val->str_, 1);
        type = gettype(parse->lex, p->matched->lexeme);
        if((type.type == ATTYPE_NULL || !check_id(p->matched->lexeme).isfound) && eval && isfinal) {
            add_semerror(parse, p->matched, "undeclared identifier");
        }
        print_semtype(type);
        return alloc_semt(type);
    }
    return NULL;
}

void *sem_print(token_s **curr, semantics_s *s, pda_s *pda, pna_s *pn, parse_s *p, sem_paramlist_s params, unsigned pass, void *fill, bool eval, bool isfinal)
{
    llist_s *node;
    sem_type_s *val;
    
    if(!(params.ready && eval))
        return NULL;
    
    llreverse(&params.pstack);
    while (params.pstack) {
        node = llpop(&params.pstack);
        val = node->ptr;
        free(node);
        
        printf("%u %u %u %u\n", val->type, ATTYPE_NUMINT, ATTYPE_NUMREAL, ATTYPE_ID);
        
        print_semtype(*val);
        putchar('\n');
        free(val);
    }
    printf("exiting sem print\n");
    return NULL;
}

void *sem_addtype(token_s **curr, semantics_s *s, pda_s *pda, pna_s *pn, parse_s *p, sem_paramlist_s params, unsigned pass, sem_type_s *type, bool eval, bool isfinal)
{
    llist_s *node;
    pnode_s *pnode;
    sem_type_s *t, *id;
    token_s *temp;
    bool declared;
    check_id_s check;
    
    if(hashlookup(grammar_stack->ptr, *curr))
        return NULL;
    
    if(!(params.ready && eval))
        return NULL;
    
    node = llpop(&params.pstack);
    t = node->ptr;
    free(node);
    
    node = llpop(&params.pstack);
    id = node->ptr;
    free(node);
    declared = check_redeclared(id->lexeme);
    check = check_id(id->lexeme);
    
    if(declared && (check.type->type != ATTYPE_NULL && check.type->type != ATTYPE_NOT_EVALUATED)) {
        temp = malloc(sizeof(*temp));
        if(!temp) {
            perror("Memory Allocation Error");
            exit(EXIT_FAILURE);
        }
        temp->lineno = id->tok->lineno;
        strcpy(temp->lexeme, id->lexeme);
        add_semerror(p, temp, "Redeclaration of identifier");
        hashinsert(grammar_stack->ptr, *curr, *curr);
    }
    else {
        t->tok = id->tok;
        if(!declared) {
            printf("%u %u %u %u\n", ATTYPE_NOT_EVALUATED, ATTYPE_NULL, ATTYPE_ARGLIST_FORMAL, ATTYPE_ARGLIST_ACTUAL);
            fflush(stdout);
           if(t->type != ATTYPE_ARGLIST_FORMAL)
                add_id(id->lexeme, *t, true);
        }
        settype(p->lex, id->lexeme, *t);
        if(t->type != ATTYPE_NULL && t->type != ATTYPE_NOT_EVALUATED) {
            hashinsert(grammar_stack->ptr, *curr, *curr);
        }
    }
    return NULL;
}

void *sem_addarg(token_s **curr, semantics_s *s, pda_s *pda, pna_s *pna, parse_s *p, sem_paramlist_s params, unsigned pass, sem_type_s *type, bool eval, bool isfinal)
{
    llist_s *node;
    pnode_s *pnode;
    sem_type_s *t, *id, test;
    token_s *temp;
    bool declared;
    
    if(hashlookup(grammar_stack->ptr, *curr))
        return NULL;
    
    if(!(params.ready && eval))
        return NULL;
    
    node = llpop(&params.pstack);
    t = node->ptr;
    free(node);
    
    node = llpop(&params.pstack);
    id = node->ptr;
    free(node);
    
    //  pnode = getpnode_nterm_copy(pn, id->str_, 1);
    //print_semtype(*t);
    putchar('\n');
    //for(;;)printf("%s\n", t->lexeme);
    declared = check_redeclared(id->lexeme);
    test = gettype(p->lex, id->lexeme);
    if(declared && (test.type != ATTYPE_NOT_EVALUATED && test.type != ATTYPE_NULL)) {
        temp = malloc(sizeof(*temp));
        if(!temp) {
            perror("Memory Allocation Error");
            exit(EXIT_FAILURE);
        }
        temp->lineno = id->tok->lineno;
        strcpy(temp->lexeme, id->lexeme);
        add_semerror(p, temp, "Redeclaration of identifier");
    }
    else {
        t->tok = id->tok;
        add_id(id->lexeme, *t, false);
        settype(p->lex, id->lexeme, *t);
    }
    hashinsert(grammar_stack->ptr, *curr, *curr);
    return NULL;
}

void *sem_listappend(token_s **curr, semantics_s *s, pda_s *pda, pna_s *pna, parse_s *parse, sem_paramlist_s params, unsigned pass, sem_type_s *type, bool eval, bool isfinal)
{
    llist_s *listparam, *argparam;
    sem_type_s *arglist, *arg;
    
    if(hashlookup(grammar_stack->ptr, *curr))
        return NULL;
    
    if(!(params.ready && eval))
        return NULL;
    
    argparam = llpop(&params.pstack);
    arg = argparam->ptr;
    free(argparam);
    
    listparam = llpop(&params.pstack);
    arglist = listparam->ptr;
    free(listparam);
    
    printf("enqueing: %p to %p in ", arg, arglist->q);
    if(arglist->type == ATTYPE_ARGLIST_ACTUAL)
        puts("actual paramater.");
    else
        puts("formal parameter.");
    
    
    enqueue(arglist->q, arg);
    
    llist_s *iter = arglist->q->head;
    for(; iter; iter = iter->next) {
        print_semtype(*(sem_type_s *)iter->ptr);
        printf(", ");
    }
    puts("");

    hashinsert(grammar_stack->ptr, *curr, *curr);
    
    return NULL;
}

void *sem_makelista(token_s **curr, semantics_s *s, pda_s *pda, pna_s *pna, parse_s *parse, sem_paramlist_s params, unsigned pass, sem_type_s *type, bool eval, bool isfinal)
{
    llist_s *node;
    sem_type_s *t, *final, list;
    
    if((final = hashlookup(grammar_stack->ptr, *curr)))
        return final;
    
    if(!(params.ready && eval))
        return NULL;
    
    node = llpop(&params.pstack);
    
    t = node->ptr;
    free(node);
    
    list.type = ATTYPE_ARGLIST_ACTUAL;
    list.q = queue_s_();
    list.lexeme = "--ARGLIST--";
    enqueue(list.q, t);
        
    final = alloc_semt(list);
    hashinsert(grammar_stack->ptr, *curr, final);
    return final;
}

void *sem_makelistf(token_s **curr, semantics_s *s, pda_s *pda, pna_s *pna, parse_s *parse, sem_paramlist_s params, unsigned pass, sem_type_s *type, bool eval, bool isfinal)
{
    llist_s *node;
    sem_type_s *t, *final, list;
    
    if((final = hashlookup(grammar_stack->ptr, *curr)))
        return final;
    
    if(!(params.ready && eval))
        return NULL;
    
    node = llpop(&params.pstack);
    
    t = node->ptr;
    free(node);
    
    list.type = ATTYPE_ARGLIST_FORMAL;
    list.q = queue_s_();
    list.lexeme = "--ARGLIST--";
    
    enqueue(list.q, t);
    final = alloc_semt(list);
    hashinsert(grammar_stack->ptr, *curr, final);
    return final;
}

void *sem_pushscope(token_s **curr, semantics_s *s, pda_s *pda, pna_s *pna, parse_s *parse, sem_paramlist_s params, unsigned pass, sem_type_s *type, bool eval, bool isfinal)
{
    llist_s *node;
    sem_type_s *final, *arg;
    check_id_s check;
    token_s *temp;
    bool declared;
    
    if((final = hashlookup(grammar_stack->ptr, *curr)))
        return final;
    
    if(!(params.ready && eval))
        return NULL;
    
    node = llpop(&params.pstack);
    arg = node->ptr;
    free(node);
    
    declared = check_redeclared(arg->str_);
    check = check_id(arg->str_);
    if(declared && check.type) {
        temp = malloc(sizeof(*temp));
        if(!temp) {
            perror("Memory Allocation Error");
            exit(EXIT_FAILURE);
        }
        temp->lineno = arg->tok->lineno;
        strcpy(temp->lexeme, arg->str_);
        add_semerror(parse, temp, "Redeclaration of identifier as procedure");
    }
    push_scope(arg->str_);
    scope_tree->full_id = scoped_label();
    final = (sem_type_s *)1;
    hashinsert(grammar_stack->ptr, *curr, final);
    return NULL;
}

void *sem_popscope(token_s **curr, semantics_s *s, pda_s *pda, pna_s *pna, parse_s *parse, sem_paramlist_s params, unsigned pass, sem_type_s *type, bool eval, bool isfinal)
{
    sem_type_s *final;
    
    if((final = hashlookup(grammar_stack->ptr, *curr)))
        return final;
    
    if(!(params.ready && eval))
        return NULL;
    pop_scope();
    final = (sem_type_s *)1;
    hashinsert(grammar_stack->ptr, *curr, final);
    return NULL;
}

void *sem_resettemps(token_s **curr, semantics_s *s, pda_s *pda, pna_s *pna, parse_s *parse, sem_paramlist_s params, unsigned pass, sem_type_s *type, bool eval, bool isfinal)
{
    tempcount = 0;
    return NULL;
}

void *sem_resolveproc(token_s **curr, semantics_s *s, pda_s *pda, pna_s *pna, parse_s *parse, sem_paramlist_s params, unsigned pass, sem_type_s *type, bool eval, bool isfinal)
{
    llist_s *node;
    check_id_s check;
    sem_type_s dummy = {0}, *proc, copy;
    
    dummy.type = ATTYPE_NOT_EVALUATED;
    if(!(params.ready && eval))
        return alloc_semt(dummy);
    
    node = llpop(&params.pstack);
    proc = node->ptr;
    
    check = check_id(proc->str_);
    if(check.isfound) {
        copy = *proc;
        copy.str_ = check.scope->full_id;
        copy.lexeme = check.scope->full_id;
        return alloc_semt(copy);
    }
    dummy.type = ATTYPE_NULL;
    return alloc_semt(dummy);
}

void *sem_getwidth(token_s **curr, semantics_s *s, pda_s *pda, pna_s *pna, parse_s *parse, sem_paramlist_s params, unsigned pass, sem_type_s *type, bool eval, bool isfinal)
{
    char *str;
    llist_s *node;
    pnode_s *p;
    sem_type_s id, width;
    check_id_s check;
    
    id.type = ATTYPE_NOT_EVALUATED;
    if(!(params.ready && eval))
        return alloc_semt(id);
    node = llpop(&params.pstack);
    id = *(sem_type_s *)node->ptr;
    p = getpnode_nterm_copy(pna, id.str_, 1);
    str = p->matched->lexeme;
    check = check_id(str);
    width.type = ATTYPE_NUMINT;
    width.int_ = check.width;
    return alloc_semt(width);
}

void *sem_low(token_s **curr, semantics_s *s, pda_s *pda, pna_s *pna, parse_s *parse, sem_paramlist_s params, unsigned pass, sem_type_s *type, bool eval, bool isfinal)
{
    char *str;
    llist_s *node;
    pnode_s *p;
    sem_type_s id, low;
    check_id_s check;
    
    id.type = ATTYPE_NOT_EVALUATED;
    if(!(params.ready && eval))
        return alloc_semt(id);
    node = llpop(&params.pstack);
    id = *(sem_type_s *)node->ptr;
    p = getpnode_nterm_copy(pna, id.str_, 1);
    str = p->matched->lexeme;
    check = check_id(str);
    low.type = ATTYPE_NUMINT;
    if(!check.isfound) {
        low.type = ATTYPE_NULL;
        low.int_ = 0;
    }
    else if(check.type->type == ATTYPE_ARRAY)
        low.int_ = check.type->low;
    else
        low.int_ = 0;
    return alloc_semt(low);
}

int arglist_cmp(token_s **curr, parse_s *parse, token_s *tok, sem_type_s formal, sem_type_s actual)
{
    llist_s *lf, *la, *la_last;
    sem_type_s *a, *f;
    int *result;
    
    if((result = hashlookup(grammar_stack->ptr, *curr)))
        return *result;
    result = malloc(sizeof(*result));
    if(!result) {
        perror("Memory Allocation Error");
        exit(EXIT_FAILURE);
    }
    switch(actual.type) {
        case ATTYPE_ARGLIST_ACTUAL:
            break;
        case ATTYPE_ID:
        case ATTYPE_ARRAY:
        default:
            add_semerror(parse, actual.tok, "Improper assignment involving procedure type");
            *result = 0;
            hashinsert(grammar_stack->ptr, *curr, result);
            return 0;
    }
    switch(formal.type) {
        case ATTYPE_ARGLIST_FORMAL:
            break;
        case ATTYPE_ID:
        case ATTYPE_ARRAY:
        default:
            add_semerror(parse, actual.tok, "Attempt to call non-procedure object");
            *result = 0;
            hashinsert(grammar_stack->ptr, *curr, result);
            return 0;
    }
    la_last = la = actual.q->head;
    for(lf = formal.q->head; lf && la; lf = lf->next, la_last = la, la = la->next) {
        f = lf->ptr;
        a = la->ptr;
        
        if(f->type == ATTYPE_ID){
            if(a->type == ATTYPE_ID){
                if(!strcmp(f->str_, "real")){
                    if(strcmp(a->str_, "real") && strcmp(a->str_, "integer")) {
                        add_semerror(parse, a->tok, "Expected real or integer but got different type");
                        *result = 0;
                    }
                }
                else if(!strcmp(f->str_, "integer")){
                    if(strcmp(a->str_, "integer")) {
                        add_semerror(parse, a->tok, "Expected integer but got different type");
                        *result = 0;
                    }
                }
            }
            else if(a->type == ATTYPE_ARRAY){
                add_semerror(parse, a->tok, "Expected real or integer but got array");
                *result = 0;
            }
            else{
                add_semerror(parse, a->tok, "Expected real or integer but got different type");
                *result = 0;
            }
        }
        else if(f->type == ATTYPE_ARRAY){
            if(a->type == ATTYPE_ARRAY){
                if(strcmp(f->str_, a->str_)) {
                    add_semerror(parse, a->tok, "Array types mismatch");
                    *result = 0;
                }
                if(f->low != a->low || f->high != a->high) {
                    add_semerror(parse, a->tok, "Array bounds mismatch");
                    *result = 0;
                }
            }
            else if(a->type == ATTYPE_ID){
                add_semerror(parse, a->tok, "Exptected array type but got numeric type");
                *result = 0;
            }
            else{
                add_semerror(parse, a->tok, "Exptected array type but got other type");
                *result = 0;
            }
        }
        else if(f->type == ATTYPE_VOID) {
            if(a->type != ATTYPE_VOID) {
                add_semerror(parse, a->tok, "Excess Parameters Used in function call");
                *result = 0;
            }
        }
    }
    if(la) {
        if(la_last->next)
            la_last = la_last->next;
        add_semerror(parse, ((sem_type_s *)la_last->ptr)->tok, "Excess Parameters Used in function call");
        *result = 0;
        
    }
    else if(lf) {
        add_semerror(parse, ((sem_type_s *)la_last->ptr)->tok, "Not Enough Arguments Used in function call");
        *result = 0;
    }
    *result = 1;
    hashinsert(grammar_stack->ptr, *curr, result);
    return *result;
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
    char *stralloc = NULL;
    
    switch(type.type) {
        case ATTYPE_ID:
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
    return stralloc;
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
    if(!p)
        return NULL;
    while(l) {
        if(((semantics_s *)l->ptr)->n->self == p->self)
            return l->ptr;
        l = l->next;
    }
    return NULL;
}

char *make_semerror(unsigned lineno, char *lexeme, char *message)
{
    char *msg;
    size_t stlenl = strlen(lexeme);
    size_t stlenm = strlen(message);
    
    msg = malloc(sizeof(TYPE_ERROR_PREFIX) + sizeof(TYPE_ERROR_SUFFIX) + FS_INTWIDTH_DEC(lineno) + stlenl + stlenm - 1);
    if(!msg) {
        perror("Memory Allocation Error");
        exit(EXIT_FAILURE);
    }
    sprintf(msg, TYPE_ERROR_PREFIX "%s" TYPE_ERROR_SUFFIX, lineno, message, lexeme);
    msg[sizeof(TYPE_ERROR_PREFIX) + sizeof(TYPE_ERROR_SUFFIX) + FS_INTWIDTH_DEC(lineno) + stlenl + stlenm - 2] = '\n';
    return msg;
}

void add_semerror(parse_s *p, token_s *t, char *message)
{
    char *err = make_semerror(t->lineno, t->lexeme, message);
    
    if(check_listing(p->listing, t->lineno, err))
        free(err);
    else
        adderror(p->listing, err, t->lineno);
}

sem_type_s sem_newtemp(token_s **curr)
{
    sem_type_s value, *hash;
    
    if((hash = hashlookup(grammar_stack->ptr, *curr)))
        return *hash;
    
    value.type = ATTYPE_TEMP;
    value.str_ = malloc(FS_INTWIDTH_DEC(tempcount)+4);
    if(!value.str_) {
        perror("Memory Allocation Error");
        exit(EXIT_FAILURE);
    }
    value.lexeme = value.str_;
    sprintf(value.str_, "_t%u", tempcount++);

    hashinsert(grammar_stack->ptr, *curr, alloc_semt(value));
    return value;
}

sem_type_s sem_newlabel(token_s **curr)
{
    sem_type_s value, *hash;
    
    if((hash = hashlookup(grammar_stack->ptr, *curr)))
        return *hash;
    
    value.type = ATTYPE_LABEL;
    value.str_ = malloc(FS_INTWIDTH_DEC(tempcount)+40);
    if(!value.str_) {
        perror("Memory Allocation Error");
        exit(EXIT_FAILURE);
    }
    value.lexeme = value.str_;
    sprintf(value.str_, "_L%u", lablecount++);
    
    hashinsert(grammar_stack->ptr, *curr, alloc_semt(value));
    return value;
}

char *scoped_label(void)
{
    bool gotfirst = false;
    char *str, *label = NULL;
    llist_s *l = NULL, *node = NULL;
    scope_s *s;
    
    for(s = scope_tree; s; s = s->parent)
        llpush(&l, s->id);
    
    while((node = llpop(&l)))  {
        str = node->ptr;
        free(node);
        
        if(gotfirst)
            safe_addstring(&label, "_");
        else {
            safe_addstring(&label, "__");
            gotfirst = true;
        }
        safe_addstring(&label, str);
    }
    return label;
}


void write_code(void)
{
    write_code_(scope_root);
}

void write_code_(scope_s *s)
{
    unsigned i;
    
    print_listing_nonum(s->code, emitdest);
    for(i = 0; i < s->nchildren; i++)
        write_code_(s->children[i].child);
}