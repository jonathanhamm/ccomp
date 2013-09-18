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

enum lex_attr_ {
    LEXATTR_NUM,
    LEXATTR_WORD,
    LEXATTR_EQU,
    LEXATTR_COMMA,
    LEXATTR_FAKEEOF
};

enum lex_attr__ {
    LEXATTR_INT,
    LEXATTR_REAL,
    LEXATTR_LREAL
};

enum lex_types_ {
    LEXTYPE_ERROR,
    LEXTYPE_TERM,
    LEXTYPE_EOL,
    LEXTYPE_UNION,
    LEXTYPE_KLEENE,
    LEXTYPE_POSITIVE,
    LEXTYPE_ORNULL,
    LEXTYPE_RANDCHAR,
    LEXTYPE_EPSILON,
    LEXTYPE_PRODSYM,
    LEXTYPE_NONTERM,
    LEXTYPE_OPENPAREN,
    LEXTYPE_CLOSEPAREN,
    LEXTYPE_EOF,
    LEXTYPE_NULLSET,
    LEXTYPE_START,
    LEXTYPE_CROSS,
    LEXTYPE_CODE,
    LEXTYPE_DOT,
    /* 
     Add lexical types here, then increment the
     value of the macro MACHID_START in semantics.c 
     */
    LEXTYPE_ANNOTATE
};

#define LEXID_START         LEXTYPE_ANNOTATE

#define MAX_LEXLEN 31

#define LEXATTR_DEFAULT     0
#define LEXATTR_WSPACEEOL   1
#define LEXATTR_ERRTOOLONG  0
#define LEXATTR_EOLNEWPROD  1
#define LEXATTR_CHARDIG     0
#define LEXATTR_NCHARDIG    1
#define LEXATTR_BEGINDIG    2

#define CLOSTYPE_NONE       0
#define CLOSTYPE_KLEENE     1
#define CLOSTYPE_POS        2
#define CLOSTYPE_ORNULL     3

#define IDT_MANUAL          0
#define IDT_AUTOINC_TYPE    1
#define IDT_AUTOINC_ATT     2

typedef struct lex_s lex_s;
typedef struct tdat_s tdat_s;
typedef struct annotation_s annotation_s;
typedef struct tlookup_s tlookup_s;
typedef struct idtable_s idtable_s;
typedef struct idtnode_s idtnode_s;
typedef struct type_s type_s;
typedef struct token_s token_s;
typedef struct nfa_s nfa_s;
typedef struct nfa_node_s nfa_node_s;
typedef struct nfa_edge_s nfa_edge_s;
typedef struct mach_s mach_s;
typedef struct lextok_s lextok_s;

typedef unsigned (*annotation_f) (token_s **, char *, unsigned *, void *);

struct type_s
{
    unsigned short val;
    unsigned short attribute;
};

struct token_s
{
    type_s type;
    char *stype;
    unsigned lineno;
    char lexeme[MAX_LEXLEN + 1];
    char *lexeme_;
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

struct annotation_s
{
    int attribute;
    int length;
    bool attcount;
    char *type;
};

struct tlookup_s
{
    bool is_found;
    tdat_s tdat;
};

struct idtnode_s
{
    bool isterm;
    char c;
    tdat_s tdat;
    uint8_t nchildren;
    idtnode_s **children;
};

struct idtable_s
{
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
    annotation_s annotation;
    token_s *token;
    nfa_node_s *state;
};


struct mach_s
{
    bool    attr_id;
    bool    composite;
    bool    typecount;
    long    lexlen;
    token_s *nterm;
    nfa_s   *nfa;
    nfa_s   *follow;
    mach_s *next;
};

struct lex_s
{
    int typestart;
    uint16_t nmachs;
    mach_s *machs;
    idtable_s *kwtable;
    idtable_s *idtable;
    hash_s *tok_hash;
    llist_s *patch;
    linetable_s *listing;
};

struct lextok_s
{
    lex_s *lex;
    uint32_t lines;
    token_s *tokens;
};

extern lextok_s lexf (lex_s *lex, char *buf, uint32_t linestart, bool listing);
extern lex_s *buildlex (const char *file);
extern token_s *lexspec (const char *file, annotation_f af, void *data);
extern idtable_s *idtable_s_ (void);
extern int ntstrcmp (char *nterm, char *str);
extern void addstate (mach_s *mach, token_s *tok);
extern void addmachine (lex_s *lex, token_s *tok);
extern void *idtable_insert (idtable_s *table, char *str, tdat_s tdat);
extern tlookup_s idtable_lookup (idtable_s *table, char *str);
extern int addtok (token_s **tlist, char *lexeme, uint32_t lineno, uint16_t type, uint16_t attribute, char *stype);
extern inline bool hashname(lex_s *lex, unsigned long token_val, char *name);
extern inline char *getname(lex_s *lex, unsigned long token_val);
extern type_s gettype (lex_s *lex, char *buf);

#endif
