#include "general.h"
#include "lex.h"
#include "parse.h"
#include <stdio.h>
#include <stdlib.h>

static uint32_t cfg_annotate (token_s **tlist, u_char *buf, uint32_t *lineno);

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
    token_s *list, *iter;
    
    list = lexspec (file, cfg_annotate);
    parse = calloc(1, sizeof(*parse));
    if (!parse) {
        perror("Memory Allocation Error");
        return NULL;
    }
    
    return parse;
}

uint32_t cfg_annotate (token_s **tlist, u_char *buf, uint32_t *lineno)
{
    uint32_t i;
    
    for (i = 1; buf[i] != '}'; i++);
    return i+1;
}

void pp_start (token_s **curr)
{
    pp_nonterminal (curr);
    pp_nonterminals (curr);
}

void pp_nonterminal (token_s **curr)
{
    if ((*curr)->type.val == LEXTYPE_NONTERM) {
        *curr = (*curr)->next;
        if ((*curr)->type.val == LEXTYPE_PRODSYM) {
            *curr = (*curr)->next;
            pp_production (curr);
        }
    }
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
