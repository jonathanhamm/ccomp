/*
 parse.h
 Author: Jonathan Hamm
 
 Description:
    Implementation of parser generator. This reads in a specified Backus-Naur
    form and source file. The source file's syntax is checked in
    conformance to the specified LL(1) grammar.
 */

#include "general.h"
#include "parse.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>

#define LLTOKEN(node) ((token_s *)node->ptr)

typedef struct follow_s follow_s;

struct follow_s
{
    pda_s *pda;
    parse_s *parser;
    uint16_t index;
    pthread_t thread;
    follow_s *table;
    llist_s *firsts;
    llist_s *follows;
    bool *ready;
    pthread_mutex_t *jlock;
    pthread_cond_t *jcond;
    uint16_t ninherit;
    follow_s **inherit;
};

static void match_phase (lextok_s regex, token_s *cfg);
static token_s *findtok(mach_s *mlist, u_char *lexeme);
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

static bool isespsilon (production_s *production);
static bool hasepsilon (parse_s *parser, pnode_s *nonterm);
static llist_s *getfirsts (parse_s *parser, pda_s *pda);
static void getfollows (follow_s *fparams);
static inline pnode_s *makeEOF (void);
static follow_s *get_neighbor_params (follow_s *table, pda_s *pda);
static llist_s *lldeep_concat_foll (llist_s *first, llist_s *second);
static bool llpnode_contains(llist_s *list, token_s *tok);
static void add_inherit (follow_s *nonterm, follow_s *dependent);
static llist_s *inherit_follows (follow_s *params, llist_s **stack);
static void compute_firstfollows (parse_s *parser);

static void build_parse_table (parse_s *parse, token_s *tokens);

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

parse_s *build_parse (const char *file, lextok_s lextok)
{
    u_char *buf;
    parse_s *parse;
    token_s *list, *iter, *head;
    llist_s *firsts, *fiter;
    
    list = lexspec (file, cfg_annotate);
    head = list;
    parse = parse_();
    pp_start(parse, &list);
    /*
    for (iter = list; iter; iter = iter->next) {
        if (iter->type.val != LEXTYPE_NONTERM)
            printf("%s %d\n", iter->lexeme, iter->type.val);
    }
    printf("\n");*/
    compute_firstfollows (parse);
    firsts = getfirsts (parse, get_pda(parse, parse->start->nterm->lexeme));
    build_parse_table (parse, list);
    for (fiter = firsts; fiter; fiter = fiter->next)
        printf("%s, ", ((pnode_s *)fiter->ptr)->token->lexeme);
    match_phase (lextok, head);

    return parse;
}

void match_phase (lextok_s regex, token_s *cfg)
{
    token_s *token;
    idtlookup_s result;
    
    while (cfg) {
        token = findtok(regex.lex->machs, cfg->lexeme);
        if (token) {
            cfg->type = token->type;
        }
        else {
            result = idtable_lookup(regex.lex->kwtable, cfg->lexeme);
            if (result.type > 0) {
                cfg->type.val = result.type;
                cfg->type.attribute = result.att;
            }
        }
        cfg = cfg->next;
    }
}

token_s *findtok(mach_s *mlist, u_char *lexeme)
{
    uint8_t i;
    u_char buf[MAX_LEXLEN+1], *ptr;
    
    while (mlist) {
        ptr = &mlist->nterm->lexeme[1];
        for (i = 0; ptr[i] != '>'; i++)
            buf[i] = ptr[i];
        buf[i] = '\0';
        if (!strcmp(buf, lexeme))
            return mlist->nterm;
        mlist = mlist->next;
    }
    return NULL;
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
    pnode->prev = NULL;
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
            break;
    }
}

void pp_production (parse_s *parse, token_s **curr, pda_s *pda)
{
    pnode_s *token;
    production_s *production = NULL;
    
    switch ((*curr)->type.val) {
        case LEXTYPE_TERM:
        case LEXTYPE_NONTERM:
        case LEXTYPE_EPSILON:
            production = addproduction(pda);
            production->start = pnode_(*curr);
            *curr = (*curr)->next;
            token = pp_tokens(parse, curr);
            production->start->next = token;
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
    pnode_s *pnode = NULL, *token;
    
    switch ((*curr)->type.val) {
        case LEXTYPE_TERM:
        case LEXTYPE_NONTERM:
        case LEXTYPE_EPSILON:
            pnode = pnode_(*curr);
            *curr = (*curr)->next;
            token = pp_tokens(parse, curr);
            if (token) {
                pnode->next = token;
                token->prev = pnode;
            }
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

bool isespsilon (production_s *production)
{
    return production->start->token->type.val == LEXTYPE_EPSILON && !production->start->next;
}

bool hasepsilon (parse_s *parser, pnode_s *nonterm)
{
    pda_s *pda;
    uint16_t i;
    
    if (!nonterm || nonterm->token->type.val == LEXTYPE_TERM)
        return false;
    pda = get_pda(parser, nonterm->token->lexeme);
    for (i = 0; i < pda->nproductions; i++) {
        if (isespsilon(&pda->productions[i]))
            return true;
    }
    return false;
}

llist_s *getfirsts (parse_s *parser, pda_s *pda)
{
    uint16_t i;
    pda_s *tmp;
    pnode_s *iter;
    llist_s *list = NULL;
    
    for (i = 0; i < pda->nproductions; i++) {
        if (isespsilon(&pda->productions[i]))
            llpush(&list, pda->productions[i].start);            
        else {
            iter = pda->productions[i].start;
            if (!iter)
                return NULL;
            do {
                if (iter->token->type.val == LEXTYPE_TERM)
                    llpush(&list, iter);
                else {
                    printf("nonterm: %s\n", iter->token->lexeme);
                    tmp = get_pda(parser, iter->token->lexeme);
                    assert(tmp);
                    list = llconcat(list, getfirsts(parser, tmp));
                }
            }
            while (hasepsilon(parser, iter) && (iter = iter->next));
        }
    }
    return list;
}

void getfollows (follow_s *fparams)
{
    uint16_t i;
    bool has_epsilon;
    pnode_s *tmpeof;
    pda_s *pda, *nterm;
    pnode_s *iter;
    hrecord_s *curr;
    follow_s *neighbor;
    hashiterator_s *iterator;
    
    pthread_mutex_lock(fparams->jlock);
    while (!*fparams->ready)
        pthread_cond_wait(fparams->jcond, fparams->jlock);    
    pthread_mutex_unlock(fparams->jlock);

    if (fparams->pda == fparams->parser->start) {
        tmpeof = makeEOF();
        if (!llpnode_contains(fparams->follows, tmpeof->token))
            llpush(&fparams->follows, tmpeof);
        else
            free(tmpeof);
    }
    iterator = hashiterator_(fparams->parser->phash);
    for (curr = hashnext(iterator); curr; curr = hashnext(iterator)) {
        pda = (pda_s *)curr->data;
        for (i = 0; i < pda->nproductions; i++) {
            for (iter = pda->productions[i].start; iter; iter = iter->next) {
                if (get_pda(fparams->parser, iter->token->lexeme) == fparams->pda) {
                    do {
                        if (!iter->next) {
                            has_epsilon = false;
                            neighbor = get_neighbor_params(fparams->table, pda);
                            add_inherit(fparams, neighbor);
                        }
                        else {
                            iter = iter->next;
                            has_epsilon = hasepsilon(fparams->parser, iter);
                            if (iter->token->type.val == LEXTYPE_TERM) {
                                if (!llpnode_contains(fparams->follows, iter->token))
                                    llpush(&fparams->follows, iter);
                            }
                            else {
                                nterm = get_pda(fparams->parser, iter->token->lexeme);
                                neighbor = get_neighbor_params(fparams->table, nterm);
                                fparams->follows = lldeep_concat_foll(fparams->follows, neighbor->firsts);
                                if (has_epsilon) {
                                    add_inherit(fparams, neighbor);
                                }
                            }
                        }                        
                    }
                    while (has_epsilon);
                }
            }
        }
    }
    free(iterator);
    pthread_exit(0);
}

inline pnode_s *makeEOF (void)
{
    token_s *tok;
    
    tok = calloc(1, sizeof(*tok));
    if (!tok) {
        perror("Memory Allocation Error");
        exit(EXIT_FAILURE);
    }
    tok->lexeme[0] = '$';
    tok->type.val = LEXTYPE_EOF;
    tok->type.attribute = LEXATTR_FAKEEOF;
    return pnode_(tok);
}

follow_s *get_neighbor_params (follow_s *table, pda_s *pda)
{
    while (table->pda != pda)
        ++table;
    return table;
}

llist_s *lldeep_concat_foll (llist_s *first, llist_s *second)
{
    llist_s *second_ = NULL, *iter = NULL;
    
   if (!second)
       return first;
    while (((pnode_s *)second->ptr)->token->type.val == LEXTYPE_EPSILON ||
           llpnode_contains(first, ((pnode_s *)second->ptr)->token)) {
        second = second->next;
        if (!second)
            return first;
    }
    second_ = iter = llcopy(second);
    for (second = second->next; second; second = second->next) {
        if (((pnode_s *)second->ptr)->token->type.val == LEXTYPE_EPSILON ||
            llpnode_contains(first, ((pnode_s *)second->ptr)->token))
            continue;
        iter->next = llcopy(second);
        iter = iter->next;
    }
    return llconcat (first, second_);
}

bool llpnode_contains(llist_s *list, token_s *tok)
{
    while (list) {
        if (!strcmp(((pnode_s *)list->ptr)->token->lexeme, tok->lexeme))
            return true;
        list = list->next;
    }
    return false;
}

void add_inherit (follow_s *nonterm, follow_s *dependent)
{
    uint16_t i;
    
    if (nonterm->ninherit)
        nonterm->inherit = realloc(nonterm->inherit, (nonterm->ninherit + 1) * sizeof(*nonterm->inherit));
    else
        nonterm->inherit = malloc(sizeof(*nonterm->inherit));
    if (!nonterm->inherit) {
        perror("Memory Allocation Error");
        exit(EXIT_FAILURE);
    }
    for (i = 0; i < nonterm->ninherit; i++) {
        if (nonterm->inherit[i] == dependent)
            return;
    }
    nonterm->inherit[nonterm->ninherit] = dependent;
    nonterm->ninherit++;
}

llist_s *inherit_follows (follow_s *nterm, llist_s **stack)
{
    uint16_t i;
    
    if (llcontains(*stack, nterm)) {
        return NULL;
    }
    else
        llpush(stack, nterm);
    for (i = 0; i < nterm->ninherit; i++) {
        nterm->follows = lldeep_concat_foll (nterm->follows, inherit_follows(nterm->inherit[i], stack));
    }
    llpop(stack);
    return nterm->follows;
}

void compute_firstfollows (parse_s *parser)
{
    bool ready = false;
    int i, status, nitems;
    pda_s *tmp;
    hrecord_s *curr;
    llist_s *iter;
    hashiterator_s *iterator;
    follow_s *ftable;
    pthread_mutex_t jlock;
    pthread_cond_t jcond;
    llist_s *stack = NULL;
    void *attr = NULL;
    
    nitems = parser->phash->nitems;
    ftable = calloc(nitems, sizeof(*ftable));
    if (!ftable) {
        perror("Memory Allocation Error");
        exit(EXIT_FAILURE);
    }
    status = pthread_mutex_init(&jlock, NULL);
    if (status) {
        perror("Error: Failed to initialize mutex lock");
        exit(status);
    }
    status = pthread_cond_init(&jcond, NULL);
    if (status) {
        perror("Error: Failed to initialize condition variable");
        exit(status);
    }
    iterator = hashiterator_(parser->phash);
    for (curr = hashnext(iterator), i = 0; curr; curr = hashnext(iterator), i++) {
        tmp = (pda_s *)curr->data;
        tmp->firsts = getfirsts(parser, tmp);
        printf("Printing Firsts for %s\n", (char *)curr->key);
        for (iter = tmp->firsts; iter; iter = iter->next)
            printf("%s, ", ((pnode_s *)iter->ptr)->token->lexeme);
        printf("\n\n");
        ftable[i].parser = parser;
        ftable[i].table = ftable;
        ftable[i].index = i;
        ftable[i].pda = curr->data;
        ftable[i].firsts = tmp->firsts;
        ftable[i].jlock = &jlock;
        ftable[i].jcond = &jcond;
        ftable[i].ready = &ready;
        ftable[i].ninherit = 0;
    }
    free(iterator);
    for (i = 0; i < nitems; i++) {
        status = pthread_create(&ftable[i].thread, NULL, (void *(*)())getfollows, &ftable[i]);
        if (status) {
            perror("Error: Could not start new thread");
            exit(status);
        }
    }
    
    pthread_mutex_lock(&jlock);
    ready = true;
    pthread_cond_broadcast(&jcond);
    pthread_mutex_unlock(&jlock);
    
    for (i = 0; i < nitems; i++)
        status = pthread_join(ftable[i].thread, &attr);
    for (i = 0; i < nitems; i++)
        ftable[i].pda->follows = inherit_follows(&ftable[i], &stack);
    for (i = 0; i < nitems; i++) {
        printf("Printing Follows for %s\n", ftable[i].pda->nterm->lexeme);
        for (iter = ftable[i].follows; iter; iter = iter->next)
            printf("%s, ", ((pnode_s *)iter->ptr)->token->lexeme);
        printf("\n\n");
    }
    pthread_mutex_destroy(&jlock);
    pthread_cond_destroy(&jcond);
    free(ftable);
}

void build_parse_table (parse_s *parse, token_s *tokens)
{
    uint16_t    n_terminals = 0,
                n_nonterminals = 0;
    llist_s *terminals = NULL,
            *nonterminals = NULL;
    parsetable_s *table;
    
    while (tokens) {
        if (tokens->type.val == LEXTYPE_TERM) {
            llpush(&terminals, tokens);
            n_terminals++;
        }
        else if (tokens->type.val == LEXTYPE_NONTERM) {
            llpush(&nonterminals, tokens);
            n_nonterminals++;
        }
        tokens = tokens->next;
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