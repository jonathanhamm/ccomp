#include "general.h"
#include "lex.h"
#include "parse.h"
#include <stdio.h>
#include <stdlib.h>

static void pp_start (token_s **curr);
static void pp_nonterminal (token_s **curr);
static void pp_nonterminals (token_s **curr);
static void pp_production (token_s **curr);
static void pp_productions (token_s **curr);
static void pp_token (token_s **curr);
static void pp_tokens (token_s **curr);
static void pp_decoration (token_s **curr);

parse_s *build_parse (const char *file)
{
    u_char *buf;
    parse_s *parse;
    
    buf = readfile(file);
    parse = calloc(1, sizeof(*parse));
    if (!parse) {
        perror("Heap Allocation Error");
        return NULL;
    }
    
    return parse;
}

void pp_start (token_s **curr)
{
    
}

void pp_nonterminal (token_s **curr)
{
    
}

void pp_nonterminals (token_s **curr)
{
    
}

void pp_production (token_s **curr)
{
    
}

void pp_productions (token_s **curr)
{
    
}

void pp_token (token_s **curr)
{
    
}

void pp_tokens (token_s **curr)
{
    
}

void pp_decoration (token_s **curr)
{
    
}
