/*
 Author: Jonathan Hamm
 
 lex.c
 */

#include "lex.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>

#define LEXTYPE_ERROR       0
#define LEXTYPE_TERM        1
#define LEXTYPE_EOL         2
#define LEXTYPE_UNION       3
#define LEXTYPE_KLEENE      4
#define LEXTYPE_POSITIVE    5
#define LEXTYPE_ORNULL      6
#define LEXTYPE_RANDCHAR    8
#define LEXTYPE_EPSILON     11
#define LEXTYPE_PRODSYM     12
#define LEXTYPE_NONTERM     13
#define LEXTYPE_OPENPAREN   14
#define LEXTYPE_CLOSEPAREN  15
#define LEXTYPE_EOF         16
#define LEXTYPE_NULLSET     17
#define LEXTYPE_START       18

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

#define OP_NOP              0
#define OP_CONCAT           1
#define OP_UNION            2

typedef struct exp__s exp__s;
typedef struct nodelist_s nodelist_s;
typedef struct lexargs_s lexargs_s;
typedef struct pnonterm_s pnonterm_s;

struct exp__s
{
    int8_t op;
    nfa_s *nfa;
};

struct lexargs_s
{
    lex_s *lex;
    mach_s *machine;
    u_char *buf;
    uint16_t bread;
    bool accepted;
};

struct pnonterm_s
{
    bool success;
    uint32_t offset;
};

static void printlist (token_s *list);
static void parray_insert (idtnode_s *tnode, uint8_t index, idtnode_s *child);
static uint16_t bsearch_tr (idtnode_s *tnode, u_char key);
static int trie_insert (idtable_s *table, idtnode_s *trie, u_char *str);
static int trie_lookup (idtnode_s *trie, u_char *str);

static lex_s *lex_s_ (void);
static void parseregex (lex_s *lex, token_s **curr);
static void prx_keywords (lex_s *lex, token_s **curr);
static void prx_tokens (lex_s *lex, token_s **curr);
static void prx_tokens_ (lex_s *lex, token_s **curr);
static void prx_texp (lex_s *lex, token_s **curr);
static nfa_s *prx_expression (lex_s *lex, token_s **curr, nfa_s **unfa, nfa_s **concat);
static exp__s prx_expression_ (lex_s *lex, token_s **curr, nfa_s **unfa, nfa_s **concat);
static nfa_s *prx_term (lex_s *lex, token_s **curr, nfa_s **unfa, nfa_s **concat);
static int prx_closure (lex_s *lex, token_s **curr);

static void addcycle (nfa_node_s *start, nfa_node_s *dest);

static void mscan (lexargs_s *args);
static mach_s *getmach(lex_s *lex, u_char *id);

lex_s *buildlex (const char *file)
{
    uint32_t i, j, lineno;
    u_char *buf;
    uint8_t bpos;
    u_char lbuf[2*MAX_LEXLEN + 1];
    token_s *list, *backup;
    lex_s *lex;
    
    list = NULL;
    buf = readfile(file);
    if (!buf)
        return NULL;
    for (i = 0, j = 0, lineno = 1, bpos = 0; buf[i] != UEOF; i++) {
        switch (buf[i]) {
            case '|':
                addtok (&list, "|", lineno, LEXTYPE_UNION, LEXATTR_DEFAULT);
                break;
            case '(':
                addtok (&list, "(", lineno, LEXTYPE_OPENPAREN, LEXATTR_DEFAULT);
                break;
            case ')':
                addtok (&list, ")", lineno, LEXTYPE_CLOSEPAREN, LEXATTR_DEFAULT);
                break;
            case '*':
                addtok (&list, "*", lineno, LEXTYPE_KLEENE, LEXATTR_DEFAULT);
                break;
            case '+':
                addtok (&list, "+", lineno, LEXTYPE_POSITIVE, LEXATTR_DEFAULT);
                break;
            case '?':
                addtok (&list, "?", lineno, LEXTYPE_ORNULL, LEXATTR_DEFAULT);
                break;
            case '\n':
                lineno++;
                addtok (&list, "EOL", lineno, LEXTYPE_EOL, LEXATTR_DEFAULT);
                break;
            case '=':
                if (buf[i+1] == '>') {
                    addtok (&list, "=>", lineno, LEXTYPE_PRODSYM, LEXATTR_DEFAULT);
                    if (list->prev) {
                        if (list->prev->type.val ==  LEXTYPE_NONTERM && list->prev->prev) {
                            if (list->prev->prev->type.val == LEXTYPE_EOL)
                                list->prev->prev->type.attribute = LEXATTR_EOLNEWPROD;
                        }
                    }
                    i++;
                }
                else
                    goto fallthrough_;
                break;
            case '<':
                lbuf[0] = '<';
                for (bpos = 1, i++, j = i; ((buf[i] >= 'a' && buf[i] <= 'z') || (buf[i] >= 'A' && buf[i] <= 'Z')
                                || (buf[i] >= '0' && buf[i] <= '9') || buf[i] == '_'); bpos++, i++)
                {
                    if (bpos == MAX_LEXLEN) {
                        addtok (&list, lbuf, lineno, LEXTYPE_ERROR, LEXATTR_ERRTOOLONG);
                        goto doublebreak_;
                    }
                    lbuf[bpos] = buf[i];
                }
                if (i == j) {
                    i--;
                    goto fallthrough_;
                }
                else if (buf[i] == '>' && bpos != MAX_LEXLEN) {
                    lbuf[bpos] = '>';
                    lbuf[bpos + 1] = '\0';
                    addtok (&list, lbuf, lineno, LEXTYPE_NONTERM, LEXATTR_DEFAULT);
                }
                else {
                    lbuf[bpos] = '\0';
                    addtok (&list, &lbuf[0], lineno, LEXTYPE_TERM, LEXATTR_DEFAULT);
                    i--;
                }
                break;
            case NULLSET:
                addtok (&list, NULLSETSTR, lineno, LEXTYPE_NULLSET, LEXATTR_DEFAULT);
                break;
fallthrough_:
            default:
                if (buf[i] <= ' ')
                    break;
                for (bpos = 0, j = LEXATTR_CHARDIG; buf[i] > ' ' && buf[i] != UEOF; bpos++, i++)
                {
                    if (bpos == MAX_LEXLEN) {
                        lbuf[bpos] = '\0';
                        addtok (&list, lbuf, lineno, LEXTYPE_ERROR, LEXATTR_ERRTOOLONG);
                        break;
                    }
                    switch(buf[i]) {
                        case '(':
                        case ')':
                        case '*':
                        case '+':
                        case '?':
                        case '|':
                            if (bpos > 0) {
                                lbuf[bpos] = '\0';
                                addtok (&list, lbuf, lineno, LEXTYPE_TERM, j);
                            }
                            i--;
                            goto doublebreak_;
                        case NULLSET:
                            addtok (&list, NULLSETSTR, lineno, LEXTYPE_NULLSET, LEXATTR_DEFAULT);
                            goto doublebreak_;
                        default:
                            break;
                    }
                    if (buf[i] == '\\') {
                        i++;
                        if (buf[i] == 'E') {
                            addtok(&list, "EPSILON", lineno, LEXTYPE_EPSILON, LEXATTR_DEFAULT);
                            goto doublebreak_;
                        }
                    }
                    lbuf[bpos] = buf[i];
                    if (!((buf[i] >= 'A' && buf[i] <= 'Z') || (buf[i] >= 'a' && buf[i] <= 'z') || (buf[i] >= '0' && buf[i] <= '9')))
                        j = LEXATTR_NCHARDIG;
                }
                if (!bpos) {
                    lbuf[0] = buf[i];
                    lbuf[1] = '\0';
                    addtok (&list, lbuf, lineno, LEXTYPE_TERM, j);
                }
                else {
                    lbuf[bpos] = '\0';
                    if (buf[0] >= '0' && buf[0] <= '9')
                        addtok (&list, lbuf, lineno, LEXTYPE_TERM, j);
                    else 
                        addtok (&list, lbuf, lineno, LEXTYPE_TERM, LEXATTR_BEGINDIG);
                    i--;
                }
                break;
        }
doublebreak_:
        ;
    }
    addtok (&list, "$", lineno, LEXTYPE_EOF, LEXATTR_DEFAULT);
    while (list->prev) {
        if (list->type.val == LEXTYPE_EOL && list->type.attribute == LEXATTR_DEFAULT) {
            backup = list;
            list->prev->next = list->next;
            if (list->next)
                list->next->prev = list->prev;
            list = list->prev;
            free(backup);
        }
        else
            list = list->prev;
    }
    if (list->type.val == LEXTYPE_EOL && list->type.attribute == LEXATTR_DEFAULT) {
        backup = list;
        list = list->next;
        if (list)
            list->prev = NULL;
        free(backup);
    }
    token_s *iter;
    for (iter = list; iter; iter = iter->next)
        printf("%s\n", iter->lexeme);
    lex = lex_s_();
    parseregex(lex, &list);
    printf("\n\n---------\n\n");
    mach_s *curr = lex->machs;
    for (; curr; curr = curr->next) {
        printf("%s\n", curr->nterm->lexeme);
       /* for (mcurr = curr->start; mcurr; ) {
            printf("%s\n", mcurr->token->lexeme);
            if (mcurr->branches)
                mcurr = mcurr->branches[0];
            else
                break;
        }*/
    }
    return lex;
}

int addtok (token_s **tlist, u_char *lexeme, uint32_t lineno, uint16_t type, uint16_t attribute)
{
    token_s *ntok;
    
  /*  if(lexeme[0] >= 0x80) {
        printf("%s   %u , %d\n", lexeme, lexeme[0], lineno);
        asm("hlt");
    }*/
    
    ntok = calloc(1, sizeof(*ntok));
    if (!ntok) {
        perror("Heap Allocation Error");
        return -1;
    }
    ntok->type.val = type;
    ntok->type.attribute = attribute;
    ntok->lineno = lineno;
    strcpy(ntok->lexeme, lexeme);
    if (!*tlist)
        *tlist = ntok;
    else {
        (*tlist)->next = ntok;
        ntok->prev = *tlist;
        *tlist = ntok;
        
    }
    printf("lexeme: %s\n", lexeme);
    return 0;
}

void printlist (token_s *list)
{
    while (list) {
        printf("%s\n", list->lexeme);
        list = list->next;
    }
}

void parseregex (lex_s *lex, token_s **list)
{
    prx_keywords(lex, list);
    if ((*list)->type.val != LEXTYPE_EOL)
        printf("Syntax Error at line %u: Expected EOL but got %s\n", (*list)->lineno, (*list)->lexeme);
    *list = (*list)->next;
    prx_tokens(lex, list);
    if ((*list)->type.val != LEXTYPE_EOF)
        printf("Syntax Error at line %u: Expected $ but got %s\n", (*list)->lineno, (*list)->lexeme);
}

void prx_keywords (lex_s *lex, token_s **curr)
{
    while ((*curr)->type.val == LEXTYPE_TERM) {
        idtable_insert(lex->kwtable, (*curr)->lexeme);
        *curr = (*curr)->next;
    }
}

void prx_tokens (lex_s *lex, token_s **curr)
{
    prx_texp (lex, curr);
    prx_tokens_(lex, curr);
}

void prx_tokens_ (lex_s *lex, token_s **curr)
{
    if ((*curr)->type.val == LEXTYPE_EOL) {
        *curr = (*curr)->next;
        prx_texp (lex, curr);
        prx_tokens_ (lex, curr);
    }
}
/*********************************** EXPIRIMENTAL NEW ROUTINES *************************************/
inline nfa_s *nfa_ (void)
{
    return calloc(1, sizeof(nfa_s));
}

inline nfa_node_s *nfa_node_s_ (void)
{
    return calloc(1, sizeof(nfa_node_s));
}

token_s *make_epsilon (void)
{
    token_s *new;
    
    new = calloc(1, sizeof(*new));
    if (!new) {
        perror("Heap Allocation Error");
        exit(EXIT_FAILURE);
    }
    strcpy(new->lexeme, "EPSILON");
    new->type.val = LEXTYPE_EPSILON;
    new->type.attribute = LEXATTR_DEFAULT;
    return new;
}

nfa_edge_s *nfa_edge_s_(token_s *token, nfa_node_s *state)
{
    nfa_edge_s *edge;
    
    edge = calloc(1, sizeof(*edge));
    if (!edge) {
        perror("Heap Allocation error");
        exit(EXIT_FAILURE);
    }
    edge->token = token;
    edge->state = state;
    return edge;
}

void addedge (nfa_node_s *start, nfa_edge_s *edge)
{
    if (start->nedges)
        start->edges = realloc(start->edges, sizeof(*start->edges) * (start->nedges+1));
    else
        start->edges = malloc(sizeof(*start->edges));
    if (!start->edges) {
        perror("Heap Allocation error");
        exit(EXIT_FAILURE);
    }
    start->edges[start->nedges] = edge;
    start->nedges++;
}

void addcycle (nfa_node_s *start, nfa_node_s *dest)
{
    nfa_edge_s *edge;
    
    if (start->cycles)
        start->cycles = realloc(start->cycles, sizeof(*start->cycles) * (start->ncycles+1));
    else
        start->cycles = malloc(sizeof(*start->cycles));
    if (!start->cycles) {
        perror("Heap Allocation error");
        exit(EXIT_FAILURE);
    }
    edge = nfa_edge_s_(make_epsilon(), dest);
    start->cycles[start->ncycles] = edge;
    start->ncycles++;
}

void reparent (nfa_node_s *parent, nfa_node_s *oldparent)
{
    uint16_t i;
    
    for (i = 0; i < oldparent->nedges; i++)
        addedge (parent, oldparent->edges[i]);
}

void insert_at_branch (nfa_s *unfa, nfa_s *concat, nfa_s *insert)
{
    int i;
    
    for (i = unfa->start->nedges-1; i >= 0; i--) {
        if (unfa->start->edges[i]->state == concat->start) {
            reparent (insert->final, concat->start);
            insert->final = concat->final;
            unfa->start->edges[i]->state = insert->start;
            break;
        }
    }
}

/******************************************************************************************************/

void print_nfa(nfa_node_s *start, nfa_node_s *final)
{
    uint16_t i;
 
    if (start == final) {
        printf("breakout\n");
        return;
    }
    printf("start->nedges: %d\n", start->nedges);
    for (i = 0; i < start->nedges; i++) {
        printf("edge: %s to %p\n", start->edges[i]->token->lexeme, start->edges[i]->state);
        if (start != start->edges[i]->state)
           print_nfa(start->edges[i]->state, final);
        else
            printf("aaaaaaa\n");
    }
    for (i = 0; i < start->ncycles; i++)
        printf("cycle: %s to %p\n", start->cycles[i]->token->lexeme, start->cycles[i]->state);
    printf("end loop\n");
}

void prx_texp (lex_s *lex, token_s **curr)
{
    nfa_s *NULL_1 = NULL, *NULL_2 = NULL;
    token_s *nterm;
    
    if ((*curr)->type.val == LEXTYPE_NONTERM) {
        addmachine (lex, *curr);
        *curr = (*curr)->next;
        if ((*curr)->type.val == LEXTYPE_PRODSYM) {
            *curr = (*curr)->next;
            nterm = malloc(sizeof(*nterm));
            if (!nterm) {
                perror("Heap Allocation Error");
                return;
            }
            nterm->lineno = 0;
            nterm->type.val = LEXTYPE_START;
            nterm->type.attribute = LEXATTR_DEFAULT;
            nterm->next = NULL;
            nterm->prev = NULL;
            snprintf(nterm->lexeme, MAX_LEXLEN, "Start: %s", (*curr)->prev->prev->lexeme);
            lex->machs->nfa = prx_expression(lex , curr, &NULL_1, &NULL_2);
            if (NULL_1) {
                insert_at_branch (NULL_1, NULL_2, lex->machs->nfa);
                lex->machs->nfa = NULL_1;
            }
            printf("%s: %d %s\n", nterm->lexeme, lex->machs->nfa->start->nedges, lex->machs->nfa->start->edges[0]->token->lexeme);
            print_nfa(lex->machs->nfa->start, lex->machs->nfa->final);
            printf("\n\n");
        }
        else
            printf("Syntax Error at line %u: Expected '=>' but got: %s\n", (*curr)->lineno, (*curr)->lexeme);
    }
    else
        printf("Syntax Error at line %u: Expected nonterminal: <...>, but got: %s\n", (*curr)->lineno, (*curr)->lexeme);
}

nfa_s *prx_expression (lex_s *lex, token_s **curr, nfa_s **unfa, nfa_s **concat)
{
    exp__s exp_;
    nfa_s *nfa, *term;
    
    term = prx_term(lex, curr, unfa, concat);
    nfa = term;
    switch (prx_closure(lex, curr)) {
        case CLOSTYPE_KLEENE:
            nfa = nfa_();
            nfa->start = nfa_node_s_();
            nfa->final = nfa_node_s_();
            addedge(nfa->start, nfa_edge_s_(make_epsilon(), term->start));
            addedge(term->final, nfa_edge_s_(make_epsilon(), nfa->final));
            addcycle(term->final, term->start);
            break;
        case CLOSTYPE_POS:
            break;
        case CLOSTYPE_ORNULL:
            break;
        case CLOSTYPE_NONE:
            break;
        default:
            break;
    }
    exp_ = prx_expression_(lex, curr, unfa, concat);
    if (exp_.op == OP_UNION) {
        if (*unfa) {
            addedge((*unfa)->start, nfa_edge_s_(make_epsilon(), nfa->start));
            addedge(nfa->final, nfa_edge_s_(make_epsilon(), (*unfa)->final));
        }
        else {
            *unfa = nfa;
            nfa->start = nfa_node_s_();
            nfa->final = nfa_node_s_();
            addedge(nfa->start, nfa_edge_s_(make_epsilon(), term->start));
            addedge(nfa->start, nfa_edge_s_(make_epsilon(), exp_.nfa->start));
            addedge(term->final, nfa_edge_s_(make_epsilon(), nfa->final));
            addedge(exp_.nfa->final, nfa_edge_s_(make_epsilon(), nfa->final));
        }
        *concat = term;
    }
    else if (exp_.op == OP_CONCAT) {
        if (*unfa) {
            insert_at_branch (*unfa, *concat, nfa);
            *concat = term;
        }
        else {
            reparent(term->final, nfa->start);
            term->final = nfa->final;
        }
    }
    return nfa;
}

exp__s prx_expression_ (lex_s *lex, token_s **curr, nfa_s **unfa, nfa_s **concat)
{
    if ((*curr)->type.val == LEXTYPE_UNION) {
        *curr = (*curr)->next;
        switch ((*curr)->type.val) {
            case LEXTYPE_OPENPAREN:
            case LEXTYPE_TERM:
            case LEXTYPE_NONTERM:
            case LEXTYPE_EPSILON:
                return (exp__s){.op = OP_UNION, .nfa = prx_expression(lex, curr, unfa, concat)};
            default:
                *curr = (*curr)->next;
                printf("Syntax Error line %u: Expected '(' , terminal, or nonterminal, but got: %s\n", (*curr)->lineno, (*curr)->lexeme);
                return (exp__s){.op = -1, .nfa = NULL};
        }
    }
    switch ((*curr)->type.val) {
        case LEXTYPE_OPENPAREN:
        case LEXTYPE_TERM:
        case LEXTYPE_NONTERM:
        case LEXTYPE_EPSILON:
            return (exp__s){.op = OP_CONCAT, .nfa = prx_expression(lex, curr, unfa, concat)};
            break;
        default:
            return (exp__s){.op = OP_NOP, .nfa = NULL};
    }
}

nfa_s *prx_term (lex_s *lex, token_s **curr, nfa_s **unfa, nfa_s **concat)
{
    exp__s exp_;
    nfa_s *nfa, *NULL_1 = NULL, *NULL_2, *backup1, *backup2;
    nfa_node_s *start, *final;
    
    switch((*curr)->type.val) {
        case LEXTYPE_OPENPAREN:
            *curr = (*curr)->next;
            backup1 = *unfa;
            backup2 = *concat;
            nfa = prx_expression(lex, curr, &NULL_1, &NULL_2);
            if ((*curr)->type.val != LEXTYPE_CLOSEPAREN)
                printf("Syntax Error at line %u: Expected ')' , but got: %s\n", (*curr)->lineno, (*curr)->lexeme);
            *curr = (*curr)->next;
            *unfa = backup1;
            *concat = backup2;
            return nfa;
        case LEXTYPE_TERM:
        case LEXTYPE_NONTERM:
        case LEXTYPE_EPSILON:
            nfa = nfa_();
            nfa->start = nfa_node_s_();
            nfa->final = nfa_node_s_();
            addedge(nfa->start, nfa_edge_s_(*curr, nfa->final));
            *curr = (*curr)->next;
            exp_ = prx_expression_(lex, curr, unfa, concat);
            if (exp_.op == OP_UNION) {
                if (*unfa) {
                    addedge((*unfa)->start, nfa_edge_s_(make_epsilon(), exp_.nfa->start));
                    addedge(exp_.nfa->final, nfa_edge_s_(make_epsilon(), (*unfa)->final));
                }
                else {
                    start = nfa_node_s_();
                    final = nfa_node_s_();
                    addedge(start, nfa_edge_s_(make_epsilon(), nfa->start));
                    addedge(start, nfa_edge_s_(make_epsilon(), exp_.nfa->start));
                    addedge(nfa->final, nfa_edge_s_(make_epsilon(), final));
                    addedge(exp_.nfa->final, nfa_edge_s_(make_epsilon(), final));
                    *unfa = nfa;
                    nfa->start = start;
                    nfa->final = final;
                }
                *concat = nfa;
            }
            else if (exp_.op == OP_CONCAT) {
                if (*unfa) {
                    insert_at_branch (*unfa, *concat, exp_.nfa);
                    *concat = exp_.nfa;
                }
                else {
                    reparent (nfa->final, exp_.nfa->start);
                    nfa->final = exp_.nfa->final;
                }
            }
            break;
        default:
            printf("Syntax Error at line %u: Expected '(' , terminal , or nonterminal, but got: %s\n", (*curr)->lineno, (*curr)->lexeme);
            return NULL;
    }
    return nfa;
} 

int prx_closure (lex_s *lex, token_s **curr)
{
    int type;
    
    switch((*curr)->type.val) {
        case LEXTYPE_KLEENE:
            *curr = (*curr)->next;
            type = CLOSTYPE_KLEENE;
            break;
        case LEXTYPE_POSITIVE:
            *curr = (*curr)->next;
            type = CLOSTYPE_POS;
            break;
        case LEXTYPE_ORNULL:
            *curr = (*curr)->next;
            type = CLOSTYPE_ORNULL;
            break;
        default:
            return CLOSTYPE_NONE;
    }
    switch (prx_closure (lex, curr)) {
        case CLOSTYPE_NONE:
            return type;
        case CLOSTYPE_KLEENE:
        case CLOSTYPE_POS:
        case CLOSTYPE_ORNULL:
        default:
            return type;
    }
}

lex_s *lex_s_ (void)
{
    lex_s *lex;
    
    lex = calloc(1,sizeof(*lex));
    if (!lex) {
        perror("Heap Allocation Error");
        return NULL;
    }
    lex->kwtable = idtable_s_();
    return lex;
}

idtable_s *idtable_s_ (void)
{
    idtable_s *table;
    
    table = calloc(1, sizeof(*table));
    if (!table) {
        perror("Heap Allocation Error");
        return NULL;
    }
    table->root = calloc(1, sizeof(*table->root));
    if (!table->root) {
        perror("Heap Allocation Error");
        return NULL;
    }
    return table;
}

void idtable_insert (idtable_s *table, u_char *str)
{
    table->typecount++;
    trie_insert(table, table->root, str);
}

int idtable_lookup (idtable_s *table, u_char *str)
{
    return trie_lookup(table->root, str);
}

int trie_insert (idtable_s *table, idtnode_s *trie, u_char *str)
{
    int search;
    idtnode_s *nnode;
        
    if (!trie->nchildren) {
        search = 0; 
        trie->children = malloc(sizeof(*trie->children));
    }
    else {
        search = bsearch_tr(trie, *str);
        if (search & 0x8000)
            search &= ~0x8000;
        else
            return trie_insert(table, trie->children[search], str+1);
    }
    nnode = calloc (1, sizeof(*nnode));
    if (!(nnode && trie->children)) {
        perror("Heap Allocation Error");
        return -1;
    }
    nnode->c = *str;
    if (!*str) {
        nnode->type = table->typecount;
        parray_insert (trie, search, nnode);
        return 1;
    }
    parray_insert (trie, search, nnode);
    return trie_insert(table, trie->children[search], str+1);
}

void parray_insert (idtnode_s *tnode, uint8_t index, idtnode_s *child)
{
    uint8_t i, j, n;
    idtnode_s **children;
    
    children = tnode->children;
    n = tnode->nchildren - index;
    for (i = 0, j = tnode->nchildren; i < n; i++, j--)
        children[j] = children[j-1];
    children[j] = child;
    tnode->nchildren++;
}

int trie_lookup (idtnode_s *trie, u_char *str)
{
    uint16_t search;
    
    search = bsearch_tr(trie, *str);
    if (search & 0x8000)
        return -(search & ~0x8000);
    if (!*str)
        return trie->children[search]->type;
    return trie_lookup(trie->children[search], str+1);
}

uint16_t bsearch_tr (idtnode_s *tnode, u_char key)
{
	int16_t mid, low, high;
    
    low = 0;
    high = tnode->nchildren-1;
    for (mid = low+(high-low)/2; high >= low; mid = low+(high-low)/2) {
        if (key < tnode->children[mid]->c)
            high = mid - 1;
        else if (key > tnode->children[mid]->c)
            low = mid + 1;
        else
            return mid;
    }
    if (mid == tnode->nchildren)
        mid--;
    if (high < low)
        return mid | 0x8000;
    return mid;
}


void addmachine (lex_s *lex, token_s *tok)
{
    mach_s *nm;
    
    nm = calloc(1, sizeof(*nm));
    if (!nm) {
        perror("Heap Allocation Error");
        return; 
    }
    nm->nterm = tok;
    if (lex->machs)
        nm->next = lex->machs;
    lex->machs = nm;
    lex->curr = nm;
    lex->nmachs++;
}

token_s *lex (lex_s *lex, u_char *buf)
{
    int32_t tmp;
    uint16_t i, j, threadsmade;
    lexargs_s *largs, *chosen;
    pthread_t *threads;
    mach_s *machine;
    /*
    largs = malloc(lex->nmachs * sizeof(*largs));
    threads = malloc(lex->nmachs * sizeof(*threads));
    if (!(largs && threads)) {
        perror("Heap Allocation Error");
        return NULL;
    }
    while (*buf != UEOF) {
        while (*buf <= ' ')
            buf++;
        for (i = 0, threadsmade = 0, machine = lex->machs; machine; i++, machine = machine->next) {
            for (j = 0, tmp = 0; j < machine->start->nbranches; j++) {
                tmp = rlmatch (lex, machine, machine->start->branches[j], buf);
                if (tmp) {
                    printf("at machine: %s\n", machine->nterm->lexeme);
                    largs[threadsmade].accepted = false;
                    largs[threadsmade].bread = 0;
                    largs[threadsmade].buf = buf;
                    largs[threadsmade].lex = lex;
                    largs[threadsmade].machine = machine;
                    pthread_create(&threads[threadsmade], NULL, (void *(*)(void *))mscan, &largs[threadsmade]);
                    threadsmade++;
                    break;
                }
            }
        }
        for (i = 0; i < threadsmade; i++)
            pthread_join(threads[i], NULL);
        chosen = NULL;
        for (i = 0; i < threadsmade; i++) {
            printf("checking with: %s\n", largs[i].machine->start->token->lexeme);
            if (largs[i].accepted && (!chosen || largs[i].bread > chosen->bread)) {
                tmp = largs[i].bread;
                chosen = &largs[i];
            }
        }
        if (chosen) {
            char c = *(buf + chosen->bread);
            *(buf + tmp) = '\0';
            printf("\n\nsuccessfully parsed %s, %d from %s\n\n", buf, chosen->bread, chosen->machine->nterm->lexeme);
            *(buf + chosen->bread) = c;
            buf += chosen->bread;
        }
        else
            printf("lexical error %s\n", buf);
    }
    free(largs);
    free(threads);*/
}

mach_s *getmach(lex_s *lex, u_char *id)
{
    mach_s *iter;

    for (iter = lex->machs; iter && strcmp(iter->nterm->lexeme, id); iter = iter->next);
    return iter;
}


void mscan (lexargs_s *args)
{
    pnonterm_s result;
    
    //result = callnonterm (args->lex, args->buf, args->machine, args->machine->start);
    args->bread = result.offset;
    args->accepted = result.success;
}

