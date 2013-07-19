/*
 
 */

#ifndef LEX_H_
#define LEX_H_

#include "general.h"

#define MAX_LEXLEN 31
#define NULLSET 232
#define NULLSETSTR_(null) #null
#define NULLSETSTR  NULLSETSTR_(NULLSET)

typedef struct lex_s lex_s;
typedef struct idtable_s idtable_s;
typedef struct idtnode_s idtnode_s;
typedef struct token_s token_s;
typedef struct nfa_s nfa_s;
typedef struct nfa_node_s nfa_node_s;
typedef struct nfa_edge_s nfa_edge_s;
typedef struct mach_s mach_s;

struct token_s
{
    struct {
        uint16_t val;
        uint16_t attribute;
    } type;
    uint32_t lineno;
    u_char lexeme[MAX_LEXLEN + 1];
    token_s *prev;
    token_s *next;
};

struct idtnode_s
{
    u_char c;
    uint16_t type;
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
    uint16_t ncycles;
    nfa_edge_s **edges;
    nfa_edge_s **cycles;
};

struct nfa_edge_s
{
    int annotation;
    token_s *token;
    nfa_node_s *state;
};


struct mach_s
{
    token_s *nterm;
    nfa_s  *nfa;
    mach_s *next;
};

struct lex_s
{
    uint16_t nmachs;
    mach_s *curr;
    mach_s *machs;
    idtable_s *kwtable;
};

extern token_s *lex (lex_s *lex, u_char *buf);
extern lex_s *buildlex (const char *file);
extern idtable_s *idtable_s_ (void);
extern void addstate (mach_s *mach, token_s *tok);
extern void addmachine (lex_s *lex, token_s *tok);
extern void idtable_insert (idtable_s *table, u_char *str);
extern int idtable_lookup (idtable_s *table, u_char *str);
extern int addtok (token_s **tlist, u_char *lexeme, uint32_t lineno, uint16_t type, uint16_t attribute);

#endif
