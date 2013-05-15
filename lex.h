/*
 
 */

#ifndef LEX_H_
#define LEX_H_

#include <stdint.h>

#define MAX_LEXLEN 31
#define UEOF (u_char)EOF
#define EPSILON 219
#define EPSILONSTR_(epsilon) #epsilon
#define EPSILONSTR EPSILONSTR_(EPSILON)
#define NULLSET 232
#define NULLSETSTR_(null) #null
#define NULLSETSTR  NULLSETSTR_(NULLSET)

#ifndef u_char
typedef unsigned char u_char;
#endif

typedef struct lex_s lex_s;
typedef struct idtable_s idtable_s;
typedef struct idtnode_s idtnode_s;
typedef struct token_s token_s;

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

struct lex_s
{
    idtable_s *kwtable;
};

extern lex_s *lex_s_ (void);
extern token_s *buildlex (const char *file);
extern idtable_s *idtable_s_ (void);
extern void idtable_insert (idtable_s *table, u_char *str);
extern int idtable_lookup (idtable_s *table, u_char *str);
extern int addtok (token_s **tlist, u_char *lexeme, uint32_t lineno, uint16_t type, uint16_t attribute);

#endif
