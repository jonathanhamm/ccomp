/*
 Author: Rasputin
 
 lex.c
 */

#include "lex.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>

#define OP_NOP              0
#define OP_CONCAT           1
#define OP_UNION            2

typedef struct exp__s exp__s;
typedef struct nodelist_s nodelist_s;
typedef struct lexargs_s lexargs_s;
typedef struct pnonterm_s pnonterm_s;
typedef struct match_s match_s;

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

struct match_s
{
    int n;
    int attribute;
    bool success;
};

static void printlist (token_s *list);
static void parray_insert (idtnode_s *tnode, uint8_t index, idtnode_s *child);
static uint16_t bsearch_tr (idtnode_s *tnode, u_char key);
static int trie_insert (idtable_s *table, idtnode_s *trie, u_char *str, int type, int att);
static idtlookup_s trie_lookup (idtnode_s *trie, u_char *str);
static uint32_t regex_annotate (token_s **tlist, u_char *buf, uint32_t *lineno);

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
static void prx_annotation (nfa_edge_s *edge, token_s **curr);
static int prx_tokenid (mach_s *mach, token_s **curr);


static void addcycle (nfa_node_s *start, nfa_node_s *dest);

static void mscan (lexargs_s *args);
static mach_s *getmach(lex_s *lex, u_char *id);

uint16_t tok_hashf (void *key);
bool tok_isequalf(void *key1, void *key2);

lex_s *buildlex (const char *file)
{
    token_s *list, *iter;
    lex_s *lex;
    
    lex = lex_s_();
    list = lexspec (file, regex_annotate);
    for (iter = list; iter; iter = iter->next)
        printf("%s %d\n", iter->lexeme, iter->lineno);
    parseregex(lex, &list);
    return lex;
}

token_s *lexspec (const char *file, annotation_f af)
{
    uint32_t i, j, lineno, tmp;
    u_char *buf;
    uint8_t bpos;
    u_char lbuf[2*MAX_LEXLEN + 1];
    token_s *list = NULL, *backup;
    
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
            case '{':
                tmp = af(&list, &buf[i], &lineno);
                if (tmp)
                    i += tmp;
                else {
                    perror("Error Parsing Regex");
                    exit(EXIT_FAILURE);
                }
                break;
            case '=':
                if (buf[i+1] == '>') {
                    addtok (&list, "=>", lineno, LEXTYPE_PRODSYM, LEXATTR_DEFAULT);
                    if (list->prev) {
                        if (list->prev->type.val == LEXTYPE_ANNOTATE) {
                            if (list->prev->prev) {
                                if (list->prev->prev->type.val ==  LEXTYPE_NONTERM && list->prev->prev->prev) {
                                    if (list->prev->prev->prev->type.val == LEXTYPE_EOL)
                                        list->prev->prev->prev->type.attribute = LEXATTR_EOLNEWPROD;
                                }
                            }
                        }
                        else if (list->prev->type.val ==  LEXTYPE_NONTERM && list->prev->prev) {
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
                                || (buf[i] >= '0' && buf[i] <= '9') || buf[i] == '_' || buf[i] == '\''); bpos++, i++)
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
                        case '{':
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
    return list;
}

uint32_t regex_annotate (token_s **tlist, u_char *buf, uint32_t *lineno)
{
    uint32_t bpos = 0;
    u_char tmpbuf[MAX_LEXLEN+1];

    buf++;
    while (buf[bpos] != '}') {
        while (buf[bpos] <= ' ') {
            if (buf[bpos] == '\n')
                ++*lineno;
            bpos++;
        }
        if ((buf[bpos] >= 'a' && buf[bpos] <= 'z') || (buf[bpos] >= 'A' && buf[bpos] <= 'Z')) {
            tmpbuf[bpos] = buf[bpos];
            for (bpos++; ((buf[bpos] >= 'a' && buf[bpos] <= 'z') || (buf[bpos] >= 'A' && buf[bpos] <= 'Z')); bpos++) {
                if (bpos == MAX_LEXLEN)
                    return false;
                tmpbuf[bpos] = buf[bpos];
            }
            tmpbuf[bpos] = '\0';
            addtok(tlist, tmpbuf, *lineno, LEXTYPE_ANNOTATE, LEXATTR_WORD);
        }
        else if (buf[bpos] >= '0' && buf[bpos] <= '9') {
            tmpbuf[bpos] = buf[bpos];
            for (bpos++; buf[bpos] >= '0' && buf[bpos] <= '9'; bpos++) {
                if (bpos == MAX_LEXLEN)
                    return false;
                tmpbuf[bpos] = buf[bpos];
            }
            tmpbuf[bpos] = '\0';
            addtok(tlist, tmpbuf, *lineno, LEXTYPE_ANNOTATE, LEXATTR_NUM);
        }
    }
    return bpos+1;
}

int addtok (token_s **tlist, u_char *lexeme, uint32_t lineno, uint16_t type, uint16_t attribute)
{
    token_s *ntok;
    
    printf("adding: %s\n", lexeme);
    ntok = calloc(1, sizeof(*ntok));
    if (!ntok) {
        perror("Memory Allocation Error");
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
        idtable_insert(lex->kwtable, (*curr)->lexeme, -1, -1);
        *curr = (*curr)->next;
    }
}

void prx_tokens (lex_s *lex, token_s **curr)
{
    lex->typecount = lex->kwtable->typecount;
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
static inline nfa_s *nfa_ (void)
{
    return calloc(1, sizeof(nfa_s));
}

static inline nfa_node_s *nfa_node_s_ (void)
{
    return calloc(1, sizeof(nfa_node_s));
}

token_s *make_epsilon (void)
{
    token_s *new;
    
    new = calloc(1, sizeof(*new));
    if (!new) {
        perror("Memory Allocation Error");
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
        perror("Memory Allocation error");
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
        perror("Memory Allocation error");
        exit(EXIT_FAILURE);
    }
    start->edges[start->nedges] = edge;
    start->nedges++;
}

void reparent (nfa_node_s *parent, nfa_node_s *oldparent)
{
    uint16_t i;
    
    for (i = 0; i < oldparent->nedges; i++)
        addedge (parent, oldparent->edges[i]);
    free(oldparent);
}

void insert_at_branch (nfa_s *unfa, nfa_s *concat, nfa_s *insert)
{
    int i;
    
    for (i = unfa->start->nedges-1; i >= 0; i--) {
        if (unfa->start->edges[i]->state == concat->start) {
            reparent (insert->final, concat->start);
            insert->final = concat->final;
            unfa->start->edges[i]->state = insert->start;
            free(concat);
            return;
        }
    }
}


void prx_texp (lex_s *lex, token_s **curr)
{
    nfa_s *uparent = NULL, *concat = NULL;
    token_s *nterm;
    
    if ((*curr)->type.val == LEXTYPE_NONTERM) {
        addmachine (lex, *curr);
        *curr = (*curr)->next;
        if (!prx_tokenid (lex->machs, curr)) {
            lex->typecount++;
            lex->machs->tokid = lex->typecount;
        }
        if ((*curr)->type.val == LEXTYPE_PRODSYM) {
            *curr = (*curr)->next;
            nterm = malloc(sizeof(*nterm));
            if (!nterm) {
                perror("Memory Allocation Error");
                return;
            }
            nterm->lineno = 0;
            nterm->type.val = LEXTYPE_START;
            nterm->type.attribute = LEXATTR_DEFAULT;
            nterm->next = NULL;
            nterm->prev = NULL;
            strcpy(nterm->lexeme, (*curr)->prev->prev->lexeme);
            lex->machs->nfa = prx_expression(lex , curr, &uparent, &concat);
            if (uparent)
                lex->machs->nfa = uparent;
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
    nfa_s *un_nfa, *clos_nfa, *term;
    
    term = prx_term(lex, curr, unfa, concat);
    switch (prx_closure(lex, curr)) {
        case CLOSTYPE_KLEENE:
            clos_nfa = nfa_();
            clos_nfa->start = nfa_node_s_();
            clos_nfa->final = nfa_node_s_();
            addedge(clos_nfa->start, nfa_edge_s_(make_epsilon(), term->start));
            addedge(clos_nfa->start, nfa_edge_s_(make_epsilon(), clos_nfa->final));
            addedge(term->final, nfa_edge_s_(make_epsilon(), clos_nfa->final));
            addedge(term->final, nfa_edge_s_(make_epsilon(), term->start));
            term = clos_nfa;
            break;
        case CLOSTYPE_POS:
            clos_nfa = nfa_();
            clos_nfa->start = term->start;
            clos_nfa->final = nfa_node_s_();
            addedge(term->final, nfa_edge_s_(make_epsilon(), clos_nfa->final));
            addedge(clos_nfa->final, nfa_edge_s_(make_epsilon(), clos_nfa->final));
            term = clos_nfa;
            break;
        case CLOSTYPE_ORNULL:
            addedge(term->start, nfa_edge_s_(make_epsilon(), term->final));
            break;
        case CLOSTYPE_NONE:
            break;
        default:
            break;
    }
    exp_ = prx_expression_(lex, curr, unfa, concat);
    if (exp_.op == OP_UNION) {
        if (*unfa) {
            addedge((*unfa)->start, nfa_edge_s_(make_epsilon(), term->start));
            addedge(term->final, nfa_edge_s_(make_epsilon(), (*unfa)->final));
        }
        else {
            un_nfa = nfa_();
            un_nfa->start = nfa_node_s_();
            un_nfa->final = nfa_node_s_();
            addedge(un_nfa->start, nfa_edge_s_(make_epsilon(), term->start));
            addedge(un_nfa->start, nfa_edge_s_(make_epsilon(), exp_.nfa->start));
            addedge(term->final, nfa_edge_s_(make_epsilon(), un_nfa->final));
            addedge(exp_.nfa->final, nfa_edge_s_(make_epsilon(), un_nfa->final));
            *unfa = un_nfa;
        }
        *concat = term;
    }
    else if (exp_.op == OP_CONCAT) {
        if (*unfa) {
            insert_at_branch (*unfa, *concat, term);
            *concat = term;
        }
        else {
            reparent(term->final, exp_.nfa->start);
            term->final = exp_.nfa->final;
        }
    }
    return term;
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
    nfa_edge_s *edge;
    nfa_s *nfa, *unfa_ = NULL, *concat_ = NULL, *backup1, *u_nfa;
    
    switch((*curr)->type.val) {
        case LEXTYPE_OPENPAREN:
            *curr = (*curr)->next;
            backup1 = *unfa;
            nfa = prx_expression(lex, curr, &unfa_, &concat_);
            if ((*curr)->type.val != LEXTYPE_CLOSEPAREN)
                printf("Syntax Error at line %u: Expected ')' , but got: %s\n", (*curr)->lineno, (*curr)->lexeme);
            *curr = (*curr)->next;
            *unfa = backup1;
            if (unfa_) {
                *concat = unfa_;
                return unfa_;
            }
            else {
                *concat = nfa;
                return nfa;
            }
        case LEXTYPE_TERM:
        case LEXTYPE_NONTERM:
        case LEXTYPE_EPSILON:
            nfa = nfa_();
            nfa->start = nfa_node_s_();
            nfa->final = nfa_node_s_();
            edge = nfa_edge_s_(*curr, nfa->final);
            addedge(nfa->start, edge);
            *curr = (*curr)->next;
            prx_annotation (edge, curr);
            exp_ = prx_expression_(lex, curr, unfa, concat);
            if (exp_.op == OP_UNION) {
                if (*unfa) {
                    addedge((*unfa)->start, nfa_edge_s_(make_epsilon(), nfa->start));
                    addedge(nfa->final, nfa_edge_s_(make_epsilon(), (*unfa)->final));
                }
                else {
                    u_nfa = nfa_();
                    u_nfa->start = nfa_node_s_();
                    u_nfa->final = nfa_node_s_();
                    addedge(u_nfa->start, nfa_edge_s_(make_epsilon(), nfa->start));
                    addedge(u_nfa->start, nfa_edge_s_(make_epsilon(), exp_.nfa->start));
                    addedge(nfa->final, nfa_edge_s_(make_epsilon(), u_nfa->final));
                    addedge(exp_.nfa->final, nfa_edge_s_(make_epsilon(), u_nfa->final));
                    *unfa = u_nfa;
                }
                *concat = nfa;
            }
            else if (exp_.op == OP_CONCAT) {
                if (*unfa) {
                    insert_at_branch (*unfa, *concat, nfa);
                    *concat = nfa;
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

void prx_annotation (nfa_edge_s *edge, token_s **curr)
{
    if ((*curr)->type.val == LEXTYPE_ANNOTATE) {
        edge->annotation = atoi((*curr)->lexeme);
        *curr = (*curr)->next;
    }
}

int prx_tokenid (mach_s *mach, token_s **curr)
{
    if ((*curr)->type.val == LEXTYPE_ANNOTATE) {
        if ((*curr)->type.attribute == LEXATTR_WORD) {
            if (!strcmp((*curr)->lexeme, "autoinc"))
                mach->attr_auto = true;
            else if (!strcmp((*curr)->lexeme, "composite"))
                mach->composite = true;
            *curr = (*curr)->next;
            return 0;
        }
        else {
            mach->tokid = atoi((*curr)->lexeme);
            *curr = (*curr)->next;
            return 1;
        }
    }
    return 0;
}

lex_s *lex_s_ (void)
{
    lex_s *lex;
    
    lex = calloc(1,sizeof(*lex));
    if (!lex) {
        perror("Memory Allocation Error");
        return NULL;
    }
    lex->kwtable = idtable_s_();
    lex->idtable = idtable_s_();
    lex->tok_hash = hash_(tok_hashf, tok_isequalf);
    return lex;
}

idtable_s *idtable_s_ (void)
{
    idtable_s *table;
    
    table = calloc(1, sizeof(*table));
    if (!table) {
        perror("Memory Allocation Error");
        return NULL;
    }
    table->root = calloc(1, sizeof(*table->root));
    if (!table->root) {
        perror("Memory Allocation Error");
        return NULL;
    }
    return table;
}

void idtable_insert (idtable_s *table, u_char *str, int type, int att)
{
    table->typecount++;
    trie_insert(table, table->root, str, type, att);
}

idtlookup_s idtable_lookup (idtable_s *table, u_char *str)
{
    return trie_lookup(table->root, str);
}

int trie_insert (idtable_s *table, idtnode_s *trie, u_char *str, int type, int att)
{
    int search;
    idtnode_s *nnode;
    
    if (!trie->nchildren)
        search = 0; 
    else {
        search = bsearch_tr(trie, *str);
        if (search & 0x8000)
            search &= ~0x8000;
        else
            return trie_insert(table, trie->children[search], str+1, type, att);
    }
    nnode = calloc (1, sizeof(*nnode));
    if (!nnode) {
        perror("Memory Allocation Error");
        return -1;
    }
    nnode->c = *str;
    if (!*str) {
       if (type < 0)
            nnode->type = table->typecount;
        else
            nnode->type = type;
        nnode->att = att;
        parray_insert (trie, search, nnode);
        return 1;
    }
    parray_insert (trie, search, nnode);
    return trie_insert(table, trie->children[search], str+1, type, att);
}

void parray_insert (idtnode_s *tnode, uint8_t index, idtnode_s *child)
{
    uint8_t i, j, n;
    idtnode_s **children;
    
    if (tnode->nchildren)
        tnode->children = realloc(tnode->children, sizeof(*tnode->children) * (tnode->nchildren + 1));
    else
        tnode->children = malloc(sizeof(*tnode->children));
    children = tnode->children;
    n = tnode->nchildren - index;
    for (i = 0, j = tnode->nchildren; i < n; i++, j--)
        children[j] = children[j-1];
    children[j] = child;
    tnode->nchildren++;
}

idtlookup_s trie_lookup (idtnode_s *trie, u_char *str)
{
    uint16_t search;
    
    search = bsearch_tr(trie, *str);
    if (search & 0x8000)
        return (idtlookup_s){.type = -(search & ~0x8000), .att = 0};
    if (!*str)
        return (idtlookup_s){.type = trie->children[search]->type, .att = trie->children[search]->att};
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
        perror("Memory Allocation Error");
        return; 
    }
    nm->nterm = tok;
    if (lex->machs)
        nm->next = lex->machs;
    lex->machs = nm;
    lex->nmachs++;
}

int tokmatch(u_char *buf, token_s *tok)
{
    uint16_t i, len;
    
    if (*buf == UEOF)
        return EOF;
    len = strlen(tok->lexeme);
    for (i = 0; i < len; i++) {
        if (buf[i] == UEOF)
            return EOF;
        if (buf[i] != tok->lexeme[i])
            return 0;
    }
    return len;
}

match_s nfa_match (lex_s *lex, nfa_s *nfa, nfa_node_s *state, u_char *buf)
{
    int tmatch, tmpmatch;
    uint16_t i, type;
    mach_s *tmp;
    match_s curr, result;

    curr.n = 0;
    curr.attribute = 0;
    curr.success = false;
    if (state == nfa->final)
        curr.success = true;
    for (i = 0; i < state->nedges; i++) {
        switch (state->edges[i]->token->type.val) {
            case LEXTYPE_EPSILON:
                result = nfa_match (lex, nfa, state->edges[i]->state, buf);
                if (result.success) {
                    curr.success = true;
                    curr.attribute = (state->edges[i]->annotation > 0) ? state->edges[i]->annotation : result.attribute;
                    if (result.n > curr.n)
                        curr.n = result.n;
                }
                break;
            case LEXTYPE_NONTERM:
                tmp = getmach(lex, state->edges[i]->token->lexeme);
                result = nfa_match (lex, tmp->nfa, tmp->nfa->start, buf);
                if (result.success) {
                    tmpmatch = result.n;
                    result = nfa_match(lex, nfa, state->edges[i]->state, &buf[result.n]);
                    if (result.success) {
                        curr.success = true;
                        if (result.n > 0 && result.attribute > 0)
                            curr.attribute = result.attribute;
                        else if (tmpmatch > 0 && state->edges[i]->annotation > 0)
                            curr.attribute = state->edges[i]->annotation;
                        if (result.n + tmpmatch > curr.n)
                            curr.n = result.n + tmpmatch;
                    }
                }
                break;
            default: /* case LEXTYPE_TERM */
                tmatch = tokmatch(buf, state->edges[i]->token);
                if (tmatch && tmatch != EOF) {
                    result = nfa_match(lex, nfa, state->edges[i]->state, &buf[tmatch]);
                    if (result.success) {
                        curr.success = true;
                        curr.attribute = (state->edges[i]->annotation > 0) ? state->edges[i]->annotation : result.attribute;
                        if (result.n + tmatch > curr.n)
                            curr.n = result.n + tmatch;
                    }
                }
                break;
        }
    }
    return curr;
}

token_s *lex (lex_s *lex, u_char *buf)
{
    mach_s *mach, *bmach;
    match_s res, best;
    uint16_t lineno = 1;
    u_char c[2];
    idtlookup_s lookup;
    token_s *head = NULL, *tlist = NULL;
    
    c[1] = '\0';
    println(lineno, buf);
    while (*buf != UEOF) {
        best.attribute = 0;
        best.n = 0;
        best.success = false;
        while (*buf <= ' ') {
            if (*buf == '\n') {
                println(lineno, buf+1);
                lineno++;
            }
            buf++;
        }
        if (*buf == UEOF)
            break;
        for (mach = lex->machs; mach; mach = mach->next) {
            if (*buf == UEOF)
                break;
            res = nfa_match(lex, mach->nfa, mach->nfa->start, buf);
            if (res.success && !mach->composite && res.n > best.n) {
                best = res;
                bmach = mach;
            }
        }
        c[0] = buf[best.n];
        buf[best.n] = '\0';
        if (best.success) {
            if (best.n <= MAX_LEXLEN) {
                lookup = idtable_lookup(lex->kwtable, buf);
                if (lookup.type > 0) {
                    addtok(&tlist, buf, lineno, lookup.type, res.attribute);
                    hashname(lex, lookup.type, buf);
                }
                else {
                    lookup = idtable_lookup(lex->idtable, buf);
                    if (lookup.type > 0) {
                        addtok(&tlist, buf, lineno, lookup.type, lookup.att);
                        hashname(lex, lookup.type, buf);
                    }
                    else if (bmach->attr_auto) {
                        bmach->attrcount++;
                        addtok(&tlist, buf, lineno, bmach->tokid, bmach->attrcount);
                        idtable_insert(lex->idtable, buf, bmach->tokid, bmach->attrcount);
                        hashname(lex, bmach->tokid, bmach->nterm->lexeme);
                    }
                    else {
                        addtok(&tlist, buf, lineno, bmach->tokid, best.attribute);
                        hashname(lex, bmach->tokid, bmach->nterm->lexeme);
                    }
                }
                if (!head)
                    head = tlist;
            }
            else
                printf("%6s\tLexical Error: at line: %d: Token too long: %s\n", " ", lineno, buf);
        }
        else {
            lookup = idtable_lookup(lex->kwtable, c);
            if (lookup.type > 0) {
                addtok(&tlist, c, lineno, lookup.type, LEXATTR_DEFAULT);
                hashname(lex, lookup.type, c);
                if (!head)
                    head = tlist;
            }
            else {
                if (best.n)
                    printf("%6s\tLexical Error: at line: %d: Unknown Character: %s\n", " ", lineno, buf);
                else
                    printf("%6s\tLexical Error: at line: %d: Unknown Character: %s\n", " ", lineno, c);
            }
        }
        buf[best.n] = c[0];
        if (best.n)
            buf += best.n;
        else
            buf++;
    }
    addtok(&tlist, "$", lineno, lex->typecount+1, LEXATTR_DEFAULT);
    hashname(lex, lex->typecount+1, "$");
    return head;
}

mach_s *getmach(lex_s *lex, u_char *id)
{
    mach_s *iter;
    
    for (iter = lex->machs; iter && strcmp(iter->nterm->lexeme, id); iter = iter->next);
    if (!iter) {
        printf("Regex Error: Regex %s never defined", id);
        exit(EXIT_FAILURE);
    }
    return iter;
}

inline bool hashname(lex_s *lex, unsigned long token_val, u_char *name)
{
    return hashinsert(lex->tok_hash, (void *)token_val, name);
}

inline u_char *getname(lex_s *lex, unsigned long token_val)
{
    return hashlookup(lex->tok_hash, (void *)token_val);
}

uint16_t tok_hashf (void *key)
{
    return (unsigned long)key % HTABLE_SIZE;
}

bool tok_isequalf(void *key1, void *key2)
{
    return key1 == key2;
}