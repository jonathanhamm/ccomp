#include "general.h"
#include "parse.h"
#include <stdio.h>
#include <stdlib.h>

static uint32_t cfg_annotate (token_s **tlist, u_char *buf, uint32_t *lineno);
static parse_s *parse_(void);
static pda_s *pda_(token_s *token);
static production_s *addproduction (pda_s *pda);
static pnode_s *pnode_(token_s *token);

static void pp_start (parse_s *parse, token_s **curr);
static void pp_nonterminal (parse_s *parse, token_s **curr);
static void pp_nonterminals (parse_s *parse, token_s **curr);
static void pp_production (parse_s *parse, token_s **curr, pda_s *pda);
static void pp_productions (parse_s *parse, token_s **curr, pda_s *pda);
static pnode_s *pp_tokens (parse_s *parse, token_s **curr);
static void pp_decoration (parse_s *parse, token_s **curr, pda_s *pda);

uint16_t str_hashf (void *key);
bool str_isequalf(void *key1, void *key2);


void printpda(pda_s *start)
{
    int i;
    pnode_s *iter;
    
    for (i = 0; i < start->nproductions; i++) {
        printf("\nnew production\n");
        for (iter = start->productions[i].start; iter; iter = iter->next)
            printf("%s\n", iter->token->lexeme);
    }
}

parse_s *build_parse (const char *file)
{
    u_char *buf;
    parse_s *parse;
    token_s *list, *iter;
    
    list = lexspec (file, cfg_annotate);
    for (iter = list; iter; iter = iter->next)
        printf("%s\n", iter->lexeme);
    parse = parse_();
    pp_start(parse, &list);
    printpda(parse->start);
    return parse;
}

uint32_t cfg_annotate (token_s **tlist, u_char *buf, uint32_t *lineno)
{
    uint32_t i;
    
    for (i = 1; buf[i] != '}'; i++);
    return i+1;
}

parse_s *parse_(void)
{
    parse_s *parse;
    
    parse = malloc(sizeof(*parse));
    if (!parse) {
        perror("Memory Allocation Error");
        exit(EXIT_FAILURE);
    }
    parse->start = NULL;
    parse->phash = hash_(str_hashf, str_isequalf);
    return parse;
}

pda_s *pda_(token_s *token)
{
    pda_s *pda;
    
    pda = malloc(sizeof(*pda));
    if (!pda) {
        perror("Memory Allocation Error");
        exit(EXIT_FAILURE);
    }
    pda->nterm = token;
    pda->nproductions = 0;
    pda->productions = NULL;
    return pda;
}

production_s *addproduction (pda_s *pda)
{
    if (pda->productions)
        pda->productions = realloc(pda->productions, sizeof(*pda->productions)*(pda->nproductions + 1));
    else
        pda->productions = malloc(sizeof(*pda->productions));
    if (!pda->productions) {
        perror("Memory Allocation Error");
        exit(EXIT_FAILURE);
    }
    pda->productions[pda->nproductions].start = NULL;
    pda->nproductions++;
    return &pda->productions[pda->nproductions-1];
}

pnode_s *pnode_(token_s *token)
{
    pnode_s *pnode;
    
    pnode = malloc(sizeof(*pnode));
    if (!pnode) {
        perror("Memory Allocation Error");
        exit(EXIT_FAILURE);
    }
    pnode->token = token;
    pnode->next = NULL;
    return pnode;
}

void pp_start (parse_s *parse, token_s **curr)
{
    pp_nonterminal(parse, curr);
    pp_nonterminals(parse, curr);
}

void pp_nonterminal (parse_s *parse, token_s **curr)
{
    pda_s *pda = NULL;
    
    if ((*curr)->type.val == LEXTYPE_EOL)
        *curr = (*curr)->next;
    if ((*curr)->type.val == LEXTYPE_NONTERM) {
        pda = pda_(*curr);
        if (!hash_pda (parse, (*curr)->lexeme, pda))
            printf("Error: Redefinition of production: %s\n", (*curr)->lexeme);
        *curr = (*curr)->next;
        if ((*curr)->type.val == LEXTYPE_PRODSYM) {
            *curr = (*curr)->next;
            pp_production(parse, curr, pda);
            pp_productions(parse, curr, pda);
            pp_decoration(parse, curr, pda);
        }
        else
            printf("Syntax Error: Expected '=>' but got %s\n", (*curr)->lexeme);
    }
    else
        printf("Syntax Error: Expected nonterminal but got %s\n", (*curr)->lexeme);
}

void pp_nonterminals (parse_s *parse, token_s **curr)
{    
    switch ((*curr)->type.val) {
        case LEXTYPE_EOL:
            *curr = (*curr)->next;
            pp_nonterminal(parse, curr);
            pp_nonterminals(parse, curr);
            break;
        case LEXTYPE_EOF:
            break;
        default:
            printf("Syntax Error: Expected EOL or $ but got %s\n", (*curr)->lexeme);
    }
}

void pp_production (parse_s *parse, token_s **curr, pda_s *pda)
{
    production_s *production = NULL;
    
    switch ((*curr)->type.val) {
        case LEXTYPE_TERM:
        case LEXTYPE_NONTERM:
        case LEXTYPE_EPSILON:
            production = addproduction(pda);
            production->start = pnode_(*curr);
            *curr = (*curr)->next;
            production->start->next = pp_tokens(parse, curr);
            break;
        default:
            printf("Syntax Error: Expected token but got %s\n", (*curr)->lexeme);
            break;
    }
}

void pp_productions (parse_s *parse, token_s **curr, pda_s *pda)
{    
    switch ((*curr)->type.val) {
        case LEXTYPE_UNION:
            *curr = (*curr)->next;
            pp_production(parse, curr, pda);
            pp_productions(parse, curr, pda);
            break;
        case LEXTYPE_ANNOTATE:
        case LEXTYPE_NONTERM:
        case LEXTYPE_EOL:
        case LEXTYPE_EOF:
            break;
        default:
            printf("Syntax Error: Expected '|', annotation, nonterm, or $, but got %s\n", (*curr)->lexeme);
            break;
    }
}

pnode_s *pp_tokens (parse_s *parse, token_s **curr)
{
    pnode_s *pnode = NULL;
    
    switch ((*curr)->type.val) {
        case LEXTYPE_TERM:
        case LEXTYPE_NONTERM:
        case LEXTYPE_EPSILON:
            pnode = pnode_(*curr);
            *curr = (*curr)->next;
            pnode->next = pp_tokens(parse, curr);
            break;
        case LEXTYPE_ANNOTATE:
        case LEXTYPE_UNION:
        case LEXTYPE_EOL:
        case LEXTYPE_EOF:
            break;
        default:
            printf("Syntax Error: Expected token, annotation, nonterm, or $, but got %s\n", (*curr)->lexeme);
            break;
    }
    return pnode;
}

void pp_decoration (parse_s *parse, token_s **curr, pda_s *pda)
{
    switch ((*curr)->type.val) {
        case LEXTYPE_ANNOTATE:
            while ((*curr)->type.val == LEXTYPE_ANNOTATE)
                *curr = (*curr)->next;
            break;
        case LEXTYPE_NONTERM:
        case LEXTYPE_EOL:
        case LEXTYPE_EOF:
            break;
        default:
            printf("Syntax Error: Expected annotation, nonterm, or $, but got %s\n", (*curr)->lexeme);
            break;
    }
}

pda_s *get_pda (parse_s *parser, u_char *name)
{
    return hashlookup (parser->phash, name);
}

bool hash_pda (parse_s *parser, u_char *name, pda_s *pda)
{
    if (!parser->start)
        parser->start = pda;
    return hashinsert (parser->phash, name, pda);
}

uint16_t str_hashf (void *key)
{
    return *(uint64_t *)key % HTABLE_SIZE;
}

bool str_isequalf(void *key1, void *key2)
{
    int i;
    
    for (i = 0; i < (MAX_LEXLEN + 1) / sizeof(uint64_t); i++) {
        if (((uint64_t *)key1)[i] != ((uint64_t *)key2)[i])
            return false;
    }
    return true;
}