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

#define LLFF(node) ((ffnode_s *)node->ptr)
#define LLTOKEN(node) (LLFF(node)->token)

typedef struct follow_s follow_s;
typedef struct ffnode_s ffnode_s;

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

struct ffnode_s
{
    uint16_t prod;
    token_s *token;
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

static ffnode_s *makeffnode (token_s *token, uint16_t prod);
static bool isespsilon (production_s *production);
static bool hasepsilon (parse_s *parser, pnode_s *nonterm);
static llist_s *getfirsts (parse_s *parser, pda_s *pda);
static void getfollows (follow_s *fparams);
static inline ffnode_s *makeEOF (void);
static follow_s *get_neighbor_params (follow_s *table, pda_s *pda);
static llist_s *lldeep_concat_foll (llist_s *first, llist_s *second);
static bool llpnode_contains(llist_s *list, token_s *tok);
static void add_inherit (follow_s *nonterm, follow_s *dependent);
static llist_s *inherit_follows (follow_s *params, llist_s **stack);
static void compute_firstfollows (parse_s *parser);

static bool lllex_contains (llist_s *list, u_char *lex);
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
    build_parse_table (parse, head);
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

ffnode_s *makeffnode (token_s *token, uint16_t prod)
{
    ffnode_s *ffnode;
    
    ffnode = malloc(sizeof(*ffnode));
    if (!ffnode) {
        perror("Memory Allocation Error");
        exit(EXIT_FAILURE);
    }
    ffnode->token = token;
    ffnode->prod = prod;
    return ffnode;
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
            llpush(&list, makeffnode(pda->productions[i].start->token, i));
        else {
            iter = pda->productions[i].start;
            if (!iter)
                return NULL;
            do {
                if (iter->token->type.val == LEXTYPE_TERM)
                    llpush(&list, makeffnode(iter->token, i));
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
    ffnode_s *tmpeof;
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
                                    llpush(&fparams->follows, makeffnode(iter->token, 0));
                            }
                            else {
                                nterm = get_pda(fparams->parser, iter->token->lexeme);
                                neighbor = get_neighbor_params(fparams->table, nterm);
                                fparams->follows = lldeep_concat_foll(fparams->follows, neighbor->firsts);
                                if (has_epsilon)
                                    add_inherit(fparams, neighbor);
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

inline ffnode_s *makeEOF (void)
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
    return makeffnode(tok, 1);
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
    while (LLTOKEN(second)->type.val == LEXTYPE_EPSILON ||
           llpnode_contains(first, LLTOKEN(second))) {
        second = second->next;
        if (!second)
            return first;
    }
    second_ = iter = llcopy(second);
    for (second = second->next; second; second = second->next) {
        if (LLTOKEN(second)->type.val == LEXTYPE_EPSILON ||
            llpnode_contains(first, LLTOKEN(second)))
            continue;
        iter->next = llcopy(second);
        iter = iter->next;
    }
    return llconcat (first, second_);
}

bool llpnode_contains(llist_s *list, token_s *tok)
{
    while (list) {
        if (!strcmp(LLTOKEN(list)->lexeme, tok->lexeme))
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
    llist_s *tmp;
    
    if (llcontains(*stack, nterm)) {
        return NULL;
    }
    else
        llpush(stack, nterm);
    for (i = 0; i < nterm->ninherit; i++) {
        nterm->follows = lldeep_concat_foll (nterm->follows, inherit_follows(nterm->inherit[i], stack));
    }
    tmp = llpop(stack);
    free(tmp);
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
            printf("%s, ", LLTOKEN(iter)->lexeme);
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
            printf("%s, ", LLTOKEN(iter)->lexeme);
        printf("\n\n");
    }
    pthread_mutex_destroy(&jlock);
    pthread_cond_destroy(&jcond);
    free(ftable);
}

bool lllex_contains (llist_s *list, u_char *lex)
{
    while (list) {
        if (!strcmp(((token_s *)list->ptr)->lexeme, lex))
            return true;
        list = list->next;
    }
    return false;
}

void build_parse_table (parse_s *parse, token_s *tokens)
{
    uint16_t    i, j, k,
                n_terminals = 0,
                n_nonterminals = 0;
    llist_s *terminals = NULL,
            *nonterminals = NULL, *tmp;
    parsetable_s *ptable;
    llist_s *first_iter, *foll_iter;
    pda_s *curr;
    hrecord_s *hcurr;
    hashiterator_s *hiterator;

    while (tokens) {
        if (tokens->type.val == LEXTYPE_TERM || tokens->type.val == LEXTYPE_EPSILON) {
            if (!lllex_contains(terminals, tokens->lexeme)) {
                llpush(&terminals, tokens);
                n_terminals++;
            }
        }
        else if (tokens->type.val == LEXTYPE_NONTERM) {
            if (!lllex_contains(nonterminals, tokens->lexeme)) {
                llpush(&nonterminals, tokens);
                n_nonterminals++;
            }
        }
        tokens = tokens->next;
    }
    ptable = malloc(sizeof(*ptable));
    if (!ptable) {
        perror("Memory Allocation Error");
        exit(EXIT_FAILURE);
    }
    ptable->n_terminals = n_terminals;
    ptable->n_nonterminals = n_nonterminals;
    ptable->table = malloc(n_nonterminals * sizeof(*ptable->table));
    if (!ptable->table) {
        perror("Memory Allocation Error");
        exit(EXIT_FAILURE);
    }
    for (i = 0; i < n_nonterminals; i++) {
        ptable->table[i] = malloc(n_terminals * sizeof(**ptable->table));
        if (!ptable->table[i]) {
            perror("Memory Allocation Error");
            exit(EXIT_FAILURE);
        }
        for (j = 0; j < n_terminals; j++)
            ptable->table[i][j] = -1;
    }
    ptable->terms = calloc(sizeof(*ptable->terms), n_terminals);
    ptable->nterms = calloc(sizeof(*ptable->nterms), n_nonterminals);
    if (!ptable->terms || !ptable->nterms) {
        perror("Memory Allocation Error");
        exit(EXIT_FAILURE);
    }
    for (i = 0; i < n_terminals; i++) {
        tmp = llpop(&terminals);
        ptable->terms[i] = (token_s *)tmp->ptr;
        free(tmp);
    }
    for (i = 0; i < n_nonterminals; i++) {
        tmp = llpop(&nonterminals);
        ptable->nterms[i] = (token_s *)tmp->ptr;
        free(tmp);
    }
    hiterator = hashiterator_(parse->phash);
    for (hcurr = hashnext(hiterator); hcurr; hcurr = hashnext(hiterator)) {
        curr = (pda_s *)hcurr->data;
        for (i = 0; i < n_nonterminals; i++) {
            if (!strcmp(curr->nterm->lexeme, ptable->nterms[i]->lexeme))
                break;
        }
        printf("\n\n");

        for (first_iter = curr->firsts; first_iter; first_iter = first_iter->next) {
            for (j = 0; j < n_terminals; j++) {
                if (!strcmp(LLTOKEN(first_iter)->lexeme, ptable->terms[j]->lexeme)) {
                    if (ptable->table[i][j] != -1)
                        for(;;)printf("%s\n", ptable->terms[j]->lexeme);
                    ptable->table[i][j] = LLFF(first_iter)->prod;
                    
                    if (LLTOKEN(first_iter)->type.val == LEXTYPE_EPSILON) {
                        for (foll_iter = curr->follows; foll_iter; foll_iter = foll_iter->next) {
                            for (k = 0; k < n_nonterminals; k++) {
                                if (!strcmp(LLTOKEN(foll_iter)->lexeme, ptable->terms[k]->lexeme)) {
                                    if (ptable->table[i][k] != -1)
                                        ;//   for(;;)printf("ambiguity detected\n");
                                    ptable->table[i][k] = LLFF(foll_iter)->prod;

                                }
                            }
                        }
                    }
                }
            }
        }
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