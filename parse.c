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
    pp_start(&list);
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
            pp_productions(curr);
            pp_decoration(curr);
        }
        else
            printf("Syntax Error: Expected '=>' but got %s\n", (*curr)->lexeme);
    }
    else
        printf("Syntax Error: Expected nonterminal but got %s\n", (*curr)->lexeme);
}

void pp_nonterminals (token_s **curr)
{
    switch ((*curr)->type.val) {
        case LEXTYPE_NONTERM:
            pp_nonterminal(curr);
            pp_nonterminals (curr);
            break;
        case LEXTYPE_EOF:
            break;
        default:
            printf("Syntax Error: Expected nonterminal or $ but got %s\n", (*curr)->lexeme);
    }
}

void pp_production (token_s **curr)
{
    switch ((*curr)->type.val) {
        case LEXTYPE_TERM:
        case LEXTYPE_NONTERM:
            *curr = (*curr)->next;
            pp_tokens(curr);
            break;
        default:
            printf("Syntax Error: Expected token but got %s\n", (*curr)->lexeme);
            break;
    }
}

void pp_productions (token_s **curr)
{
    switch ((*curr)->type.val) {
        case LEXTYPE_UNION:
            *curr = (*curr)->next;
            pp_production(curr);
            pp_productions(curr);
            break;
        case LEXTYPE_ANNOTATE:
        case LEXTYPE_NONTERM:
        case LEXTYPE_EOF:
            break;
        default:
            printf("Syntax Error: Expected '|', annotation, nonterm, or $, but got %s\n", (*curr)->lexeme);
            break;
    }
}

void pp_tokens (token_s **curr)
{
    switch ((*curr)->type.val) {
        case LEXTYPE_TERM:
        case LEXTYPE_NONTERM:
            *curr = (*curr)->next;
            pp_tokens(curr);
            break;
        case LEXTYPE_ANNOTATE:
        case LEXTYPE_EOF:
            break;
        default:
            printf("Syntax Error: Expected token, annotation, nonterm, or $, but got %s\n", (*curr)->lexeme);
            break;
    }
}

void pp_decoration (token_s **curr)
{
    switch ((*curr)->type.val) {
        case LEXTYPE_ANNOTATE:
            while ((*curr)->type.val == LEXTYPE_ANNOTATE)
                *curr = (*curr)->next;
            break;
        case LEXTYPE_NONTERM:
        case LEXTYPE_EOF:
            break;
        default:
            printf("Syntax Error: Expected annotation, nonterm, or $, but got %s\n", (*curr)->lexeme);
            break;
    }
}
