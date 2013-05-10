/*
 
 */

#ifndef LEX_H_
#define LEX_H_

#include <stdint.h>

#define MAX_LEXLEN 31
#define UEOF (u_char)EOF
#define EPSILON 219

#ifndef u_char
typedef unsigned char u_char;
#endif

typedef struct token_s token_s;

struct token_s
{
    struct {
        uint16_t val;
        uint16_t attribute;
    } type;
    u_char lexeme[MAX_LEXLEN + 1];
    token_s *prev;
    token_s *next;
};

extern token_s *buildlex (const char *file);
extern int addtok (token_s **tlist, u_char *lexeme, uint16_t type, uint16_t attribute);

#endif
