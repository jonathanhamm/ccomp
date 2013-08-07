/*
 lex.h
 Author: Jonathan Hamm
 
 Description:
    Library for a lexical analyzer generator. This reads in a regular
    expression and source file specified by the user. It then tokenizes the source
    file in conformance to the specified regular expression.
*/

#ifndef LEX_H_
#define LEX_H_

#include "general.h"

#define MAX_LEXLEN 31
#define NULLSET (char)232
#define NULLSETSTR_(null) #null
#define NULLSETSTR  NULLSETSTR_(NULLSET)

#define LEXTYPE_ERROR       0
#define LEXTYPE_TERM        1
#define LEXTYPE_EOL         2
#define LEXTYPE_UNION       3
#define LEXTYPE_KLEENE      4
#define LEXTYPE_POSITIVE    5
#define LEXTYPE_ORNULL      6
#define LEXTYPE_RANDCHAR    8
#define LEXTYPE_EPSILON     11
#define LEXTYPE_PRODSYM     12
#define LEXTYPE_NONTERM     13
#define LEXTYPE_OPENPAREN   14
#define LEXTYPE_CLOSEPAREN  15
#define LEXTYPE_EOF         16
#define LEXTYPE_NULLSET     17
#define LEXTYPE_START       18
#define LEXTYPE_ANNOTATE    19

#define LEXID_START         LEXTYPE_ANNOTATE

#define LEXATTR_DEFAULT     0
#define LEXATTR_WSPACEEOL   1
#define LEXATTR_ERRTOOLONG  0
#define LEXATTR_EOLNEWPROD  1
#define LEXATTR_CHARDIG     0
#define LEXATTR_NCHARDIG    1
#define LEXATTR_BEGINDIG    2
#define LEXATTR_NUM         0
#define LEXATTR_WORD        1
#define LEXATTR_FAKEEOF     1

#define CLOSTYPE_NONE       0
#define CLOSTYPE_KLEENE     1
#define CLOSTYPE_POS        2
#define CLOSTYPE_ORNULL     3

typedef struct lex_s lex_s;
typedef struct tdat_s tdat_s;
typedef struct tlookup_s tlookup_s;
typedef struct idtable_s idtable_s;
typedef struct idtnode_s idtnode_s;
typedef struct token_s token_s;
typedef struct nfa_s nfa_s;
typedef struct nfa_node_s nfa_node_s;
typedef struct nfa_edge_s nfa_edge_s;
typedef struct mach_s mach_s;
typedef struct lextok_s lextok_s;

typedef uint32_t (*annotation_f) (token_s **, char *, uint32_t *);

struct token_s
{
    struct {
        uint16_t val;
        uint16_t attribute;
    } type;
    uint32_t lineno;
    char lexeme[MAX_LEXLEN + 1];
    token_s *prev;
    token_s *next;
};

struct tdat_s
{
    bool is_string;
    union {
        char *stype;
        int itype;
    };
    int att;
};

struct tlookup_s
{
    bool is_found;
    tdat_s tdat;
};

struct idtnode_s
{
    char c;
    tdat_s tdat;
    uint8_t nchildren;
    idtnode_s **children;
};

struct idtable_s
{
    uint16_t typecount;
    idtnode_s *root;
};

struct nfa_s
{
    nfa_node_s *start;
    nfa_node_s *final;
};

struct nfa_node_s
{
    uint16_t nedges;
    nfa_edge_s **edges;
};

struct nfa_edge_s
{
    int annotation;
    token_s *token;
    nfa_node_s *state;
};


struct mach_s
{
    bool    attr_auto;
    bool    composite;
    uint16_t attrcount;
    uint16_t tokid;
    token_s *nterm;
    nfa_s  *nfa;
    mach_s *next;
};

struct lex_s
{
    uint16_t typecount;
    uint16_t nmachs;
    mach_s *machs;
    idtable_s *kwtable;
    idtable_s *idtable;
    hash_s *tok_hash;
    llist_s *patch;
};

struct lextok_s
{
    lex_s *lex;
    token_s *tokens;
};

extern lextok_s lex (lex_s *lex, char *buf);
extern lex_s *buildlex (const char *file);
extern token_s *lexspec (const char *file, annotation_f af);
extern idtable_s *idtable_s_ (int idstart);
extern void addstate (mach_s *mach, token_s *tok);
extern void addmachine (lex_s *lex, token_s *tok);
extern void *idtable_insert (idtable_s *table, char *str, tdat_s tdat);
extern tlookup_s idtable_lookup (idtable_s *table, char *str);
extern int addtok (token_s **tlist, char *lexeme, uint32_t lineno, uint16_t type, uint16_t attribute);
extern inline bool hashname(lex_s *lex, unsigned long token_val, char *name);
extern inline char *getname(lex_s *lex, unsigned long token_val);

#endif
