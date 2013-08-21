/*
 lex.c
 Author: Jonathan Hamm
 
 Description:
    Implementation for a lexical analyzer generator. This reads in a regular
    expression and source file specified by the user. It then tokenizes the source 
    file in conformance to the specified regular expression.
 */

#include "lex.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <pthread.h>

enum basic_ops_ {
    OP_NOP = 0,
    OP_CONCAT,
    OP_UNION,
    OP_GETID,
    OP_GETIDATT,
    OP_NUM
};

#define LERR_PREFIX         "      \tLexical Error: at line: %d: "
#define LERR_UNKNOWNSYM     LERR_PREFIX "Unknown Character: %s\n"
#define LERR_TOOLONG        LERR_PREFIX "Token too long: %s\n"

#define ISANNOTATE(curr)    ((*curr)->type.val == LEXTYPE_ANNOTATE)

#define MAX_IDLEN 		10
#define MAX_INTLEN		10
#define MAX_REALINT     5
#define MAX_REALFRACT	5
#define MAX_REALEXP     2

#define INT_CHAR_WIDTH      10

typedef struct exp__s exp__s;
typedef struct nodelist_s nodelist_s;
typedef struct lexargs_s lexargs_s;
typedef struct pnonterm_s pnonterm_s;
typedef struct match_s match_s;

typedef void (*regex_callback_f) (token_s **, void *);

struct exp__s
{
    int8_t op;
    nfa_s *nfa;
};

struct lexargs_s
{
    lex_s *lex;
    mach_s *machine;
    char *buf;
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
    bool overflow;
};

static void printlist (token_s *list);
static void parray_insert (idtnode_s *tnode, uint8_t index, idtnode_s *child);
static uint16_t bsearch_tr (idtnode_s *tnode, char key);
static idtnode_s *trie_insert (idtable_s *table, idtnode_s *trie, char *str, tdat_s tdat);
static tlookup_s trie_lookup (idtnode_s *trie, char *str);
static unsigned regex_annotate (token_s **tlist, char *buf, unsigned *lineno);

static lex_s *lex_s_ (void);
static idtnode_s *patch_search (llist_s *patch, char *lexeme);
static int parseregex (lex_s *lex, token_s **curr);
static void prx_keywords (lex_s *lex, token_s **curr, int *count);
static void prx_tokens (lex_s *lex, token_s **curr, int *count);
static void prx_tokens_ (lex_s *lex, token_s **curr, int *count);
static void prx_texp (lex_s *lex, token_s **curr, int *count);
static nfa_s *prx_expression (lex_s *lex, token_s **curr, nfa_s **unfa, nfa_s **concat);
static exp__s prx_expression_ (lex_s *lex, token_s **curr, nfa_s **unfa, nfa_s **concat);
static nfa_s *prx_term (lex_s *lex, token_s **curr, nfa_s **unfa, nfa_s **concat);
static int prx_closure (lex_s *lex, token_s **curr);

static void prxa_annotation(token_s **curr, void *ptr, regex_callback_f callback);
static void prxa_regexdef(token_s **curr, mach_s *mach);
static void setann_val(int *location, int value);
static void prxa_edgestart(token_s **curr, nfa_edge_s *edge);
static void prxa_assigment(token_s **curr, nfa_edge_s *edge);
static int prxa_expression(token_s **curr, nfa_edge_s *edge);
static void prxa_expression_(token_s **curr, nfa_edge_s *edge);

static void addcycle (nfa_node_s *start, nfa_node_s *dest);
static char *make_lexerr (const char *errstr, int lineno, char *lexeme);
static void mscan (lexargs_s *args);
static mach_s *getmach(lex_s *lex, char *id);

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
    lex->typestart = parseregex(lex, &list);
    return lex;
}

token_s *lexspec (const char *file, annotation_f af)
{
    unsigned i, j, lineno, tmp, bpos;
    char *buf;
    char lbuf[2*MAX_LEXLEN + 1];
    token_s *list = NULL, *backup,
            *p, *pp;
        
    buf = readfile(file);
    if (!buf)
        return NULL;
    for (i = 0, j = 0, lineno = 1, bpos = 0; buf[i] != EOF; i++) {
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
            case (char)0xCE:
                i++;
                if (buf[i] == (char)0xB5)
                    addtok(&list, "EPSILON", lineno, LEXTYPE_EPSILON, LEXATTR_DEFAULT);
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
                    for (p = list->prev; p && p->type.val != LEXTYPE_EOL; p = p->prev) {
                        if (!p->prev)
                            pp = p;
                    }
                    if (p)
                        p->type.attribute = LEXATTR_EOLNEWPROD;
                    else {
                        p = calloc(1, sizeof(*p));
                        if (!p) {
                            perror("Memory Allocation Error");
                            exit(EXIT_FAILURE);
                        }
                        strcmp(p->lexeme, "EOL");
                        p->next = pp;
                        p->type.val = LEXTYPE_EOL;
                        p->type.attribute = LEXATTR_EOLNEWPROD;
                        pp->prev = p;                        
                    }
                    i++;
                }
                else
                    goto default_;
                    /* jump to default */
                break;
            case '<':
                lbuf[0] = '<';
                for (bpos = 1, i++, j = i; isalnum(buf[i]) || buf[i] == '_' || buf[i] == '\''; bpos++, i++)
                {
                    if (bpos == MAX_LEXLEN) {
                        addtok (&list, lbuf, lineno, LEXTYPE_ERROR, LEXATTR_ERRTOOLONG);
                        goto doublebreak_;
                    }
                    lbuf[bpos] = buf[i];
                }
                if (i == j) {
                    i--;
                    goto default_;
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
default_:
            default:
                if (buf[i] <= ' ')
                    break;
                for (bpos = 0, j = LEXATTR_CHARDIG; buf[i] > ' ' && buf[i] != EOF; bpos++, i++)
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
                            /* DOUBLEBREAK */
                        default:
                            break;
                    }
                    if (buf[i] == '\\')
                        i++;
                    lbuf[bpos] = buf[i];
                    if (!isalnum(buf[i]))
                        j = LEXATTR_NCHARDIG;
                }
                if (!bpos) {
                    lbuf[0] = buf[i];
                    lbuf[1] = '\0';
                    addtok (&list, lbuf, lineno, LEXTYPE_TERM, j);
                }
                else {
                    lbuf[bpos] = '\0';
                    if (isdigit(buf[i]))
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
    addtok(&list, "$", lineno, LEXTYPE_EOF, LEXATTR_DEFAULT);
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
  /*  token_s *bob = list;
    for (; bob; bob = bob->next)
        printf("%s %u %u\n", bob->lexeme, bob->type.val, bob->type.attribute);
    asm("hlt");*/
    free(buf);
    return list;
}

unsigned regex_annotate (token_s **tlist, char *buf, unsigned *lineno)
{
    unsigned i = 0, bpos = 0;
    char tmpbuf[MAX_LEXLEN+1];

    buf++;
    while (buf[i] != '}') {
        while (buf[i] <= ' ') {
            if (buf[i] == '\n')
                ++*lineno;
            i++;
        }
        bpos = 0;
        if (isalpha(buf[i]) || buf[i] == '<') {
            tmpbuf[bpos] = buf[i];
            for (bpos++, i++; isalpha(buf[i]) || buf[i] == '>'; bpos++, i++) {
                if (bpos == MAX_LEXLEN)
                    return false;
                tmpbuf[bpos] = buf[i];
                if (buf[i] == '>') {
                    bpos++;
                    i++;
                    break;
                }
            }
            tmpbuf[bpos] = '\0';
            addtok(tlist, tmpbuf, *lineno, LEXTYPE_ANNOTATE, LEXATTR_WORD);
        }
        else if (isdigit(buf[i])) {
            tmpbuf[bpos] = buf[i];
            for (bpos++, i++; isdigit(buf[i]); bpos++, i++) {
                if (bpos == MAX_LEXLEN)
                    return false;
                tmpbuf[bpos] = buf[i];
            }
            tmpbuf[bpos] = '\0';
            addtok(tlist, tmpbuf, *lineno, LEXTYPE_ANNOTATE, LEXATTR_NUM);
        }
        else if (buf[i] == '=') {
            i++;
            addtok(tlist, "=", *lineno, LEXTYPE_ANNOTATE, LEXATTR_EQU);
        }
        else if (buf[i] == ',') {
            i++;
            addtok(tlist, ",", *lineno, LEXTYPE_ANNOTATE, LEXATTR_COMMA);
        }
        else {
            printf("Illegal character sequence in annotation at line %u\n", *lineno);
            exit(EXIT_FAILURE);
        }
    }
    addtok(tlist, "$", *lineno, LEXTYPE_ANNOTATE, LEXATTR_FAKEEOF);
    return i+1;
}

int addtok (token_s **tlist, char *lexeme, uint32_t lineno, uint16_t type, uint16_t attribute)
{
    token_s *ntok;
    
    printf("adding %s\n", lexeme);
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

int parseregex (lex_s *lex, token_s **list)
{
    int types = LEXID_START;
    
    prx_keywords(lex, list, &types);
    if ((*list)->type.val != LEXTYPE_EOL)
        printf("Syntax Error at line %u: Expected EOL but got %s\n", (*list)->lineno, (*list)->lexeme);

    *list = (*list)->next;
    prx_tokens(lex, list, &types);
    if ((*list)->type.val != LEXTYPE_EOF)
        printf("Syntax Error at line %u: Expected $ but got %s\n", (*list)->lineno, (*list)->lexeme);
    lex->idtable = idtable_s_();
    return types;
}

void prx_keywords (lex_s *lex, token_s **curr, int *counter)
{
    char *lexeme;
    idtnode_s *node;
        
    while ((*curr)->type.val == LEXTYPE_TERM) {
        node = NULL;
        lexeme = (*curr)->lexeme;
        ++*counter;
        printf("adding %s\n", lexeme);
        idtable_insert(lex->kwtable, lexeme, (tdat_s){.is_string = false, .itype = *counter, .att = 0});
        *curr = (*curr)->next;
    }
    ++*counter;
}

void prx_tokens (lex_s *lex, token_s **curr, int *count)
{
    prx_texp (lex, curr, count);
    prx_tokens_(lex, curr, count);
}

void prx_tokens_ (lex_s *lex, token_s **curr, int *count)
{
    if ((*curr)->type.val == LEXTYPE_EOL) {
        *curr = (*curr)->next;
        prx_texp (lex, curr, count);
        prx_tokens_ (lex, curr, count);
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
    edge->annotation.attcount = false;
    edge->annotation.attribute = -1;
    edge->annotation.length = -1;
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

void prx_texp (lex_s *lex, token_s **curr, int *count)
{
    idtnode_s *tnode = NULL;
    nfa_s *uparent = NULL, *concat = NULL;

    if ((*curr)->type.val == LEXTYPE_NONTERM) {
        addmachine (lex, *curr);
        *curr = (*curr)->next;
        ++*count;
        lex->machs->nterm->type.val = *count;
        prxa_annotation(curr, lex->machs, (void (*)(token_s **, void *))prxa_regexdef);
        tnode = patch_search (lex->patch, lex->machs->nterm->lexeme);

        if ((*curr)->type.val == LEXTYPE_PRODSYM) {
            *curr = (*curr)->next;
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
            prxa_annotation(curr, edge, (void (*)(token_s **curr, void *))prxa_edgestart);
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

void prxa_annotation(token_s **curr, void *ptr, regex_callback_f callback)
{
    if ((*curr)->type.val == LEXTYPE_ANNOTATE) {
        callback(curr, ptr);
        if (ISANNOTATE(curr) && (*curr)->type.attribute == LEXATTR_FAKEEOF)
            *curr = (*curr)->next;
        else {
            printf("Syntax Error at line %d: Expected } but got %s\n", (*curr)->lineno, (*curr)->lexeme);
            exit(EXIT_FAILURE);
        }
    }
}

void prxa_regexdef(token_s **curr, mach_s *mach)
{
    if (!strcasecmp((*curr)->lexeme, "typecount"))
        mach->typecount = true;
    else if (!strcasecmp((*curr)->lexeme, "idtype")) {
        mach->attr_id = true;
        mach->composite = false;
    }
    else if (!strcasecmp((*curr)->lexeme, "composite")) {
        mach->composite = true;
        mach->attr_id = false;
    }
    else if (!strcasecmp((*curr)->lexeme, "length")) {
        *curr = (*curr)->next;
        if (ISANNOTATE(curr) && (*curr)->type.attribute == LEXATTR_EQU) {
            *curr = (*curr)->next;
            if (ISANNOTATE(curr) && (*curr)->type.attribute == LEXATTR_NUM) {
                mach->lexlen = safe_atoui((*curr)->lexeme);
                *curr = (*curr)->next;
            }
            else {
                printf("Syntax Error at line %d: Expected number but got %s\n", (*curr)->lineno, (*curr)->lexeme);
                exit(EXIT_FAILURE);
            }
        }
        else {
            printf("Syntax Error at line %d: Expected = but got %s\n", (*curr)->lineno, (*curr)->lexeme);
            exit(EXIT_FAILURE);
        }
    }
    else {
        printf("Error at line %d: Unknown regex definition mode %s\n", (*curr)->lineno, (*curr)->lexeme);
        exit(EXIT_FAILURE);
    }
    *curr = (*curr)->next;
}

void prxa_assignment(token_s **curr, nfa_edge_s *edge)
{
    int val;
    char *str;
    
    if (ISANNOTATE(curr)) {
        printf("derpadfadfa %s\n", (*curr)->lexeme);

        if ((*curr)->type.attribute == LEXATTR_FAKEEOF) {
            return;
        }
    }
    else {
        printf("Error at line %d: Unexpected %s\n", (*curr)->lineno, (*curr)->lexeme);
        exit(EXIT_FAILURE);
    }
    
    str = (*curr)->lexeme;
    if (!strcasecmp(str, "attcount")) {
        if (edge->annotation.attribute != -1) {
            printf("Error at line %d at %s: Incompatible attribute type combination.\n", (*curr)->lineno, (*curr)->lexeme);
            exit(EXIT_FAILURE);
        }
        edge->annotation.attcount = true;
    }
    *curr = (*curr)->next;
    val = prxa_expression(curr, edge);
    if (!strcasecmp(str, "attribute"))
        setann_val(&edge->annotation.attribute, val);
    else if (!strcasecmp(str, "length"))
        setann_val(&edge->annotation.length, val);
    else {
        printf("Error at line %d: Unknown attribute type: %s\n", (*curr)->lineno, str);
        exit(EXIT_FAILURE);
    }

}

void setann_val(int *location, int value)
{
    if (*location == -1)
        *location = value;
    else {
        puts("Error: Regex Attribute used more than once.\n");
        exit(EXIT_FAILURE);
    }
}

void prxa_edgestart(token_s **curr, nfa_edge_s *edge)
{    
    if (ISANNOTATE(curr)) {
        switch ((*curr)->type.attribute) {
            case LEXATTR_WORD:
                prxa_assignment(curr, edge);
                break;
            case LEXATTR_NUM:
                if (edge->annotation.attribute != -1) {
                    printf("Error at line %d: Edge attribute value %s assigned more than once.", (*curr)->lineno, (*curr)->lexeme);
                    exit(EXIT_FAILURE);
                }
                edge->annotation.attribute = safe_atoui((*curr)->lexeme);
                *curr = (*curr)->next;
                break;
        }
    }
    else {
        printf("Error parsing regex annotation at line %d: %s\n", (*curr)->lineno, (*curr)->lexeme);
        exit(EXIT_FAILURE);
    }
}

int prxa_expression(token_s **curr, nfa_edge_s *edge)
{
    int ret = 0;
    
    if (ISANNOTATE(curr)) {
        switch ((*curr)->type.attribute) {
            case LEXATTR_EQU:
                *curr = (*curr)->next;
                if (ISANNOTATE(curr)) {
                    if ((*curr)->type.attribute == LEXATTR_NUM)
                        ret = safe_atoui((*curr)->lexeme);
                    *curr = (*curr)->next;
                    prxa_expression_(curr, edge);
                }
                else {
                    printf("Error parsing regex annotation at line %d : %s\n", (*curr)->lineno, (*curr)->lexeme);
                    exit(EXIT_FAILURE);
                }
                break;
            case LEXATTR_FAKEEOF:
                return -1;
            default:
                printf("Syntax error at line %d: Expected = or } but got: %s\n", (*curr)->lineno, (*curr)->lexeme);
                exit(EXIT_FAILURE);
        }
    }
    return ret;
}

void prxa_expression_(token_s **curr, nfa_edge_s *edge)
{
    if (ISANNOTATE(curr)) {
        switch ((*curr)->type.attribute) {
            case LEXATTR_COMMA:
                *curr = (*curr)->next;
                prxa_assignment(curr, edge);
                break;
            case LEXATTR_FAKEEOF:
                break;
            default:
                printf("Syntax error at line %d: Expected , or } but got: %s\n", (*curr)->lineno, (*curr)->lexeme);
                exit(EXIT_FAILURE);
        }
    }
    else {
        printf("Error parsing regex annotation at line %d: %s\n", (*curr)->lineno, (*curr)->lexeme);
        exit(EXIT_FAILURE);
    }
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
    lex->tok_hash = hash_(tok_hashf, tok_isequalf);
    return lex;
}

idtnode_s *patch_search (llist_s *patch, char *lexeme)
{
    while (patch) {
        if (((idtnode_s *)patch->ptr)->tdat.is_string) {
            if (!strcmp(((idtnode_s *)patch->ptr)->tdat.stype, lexeme))
                return patch->ptr;
        }
        patch = patch->next;
    }
    return NULL;
}

idtable_s *idtable_s_ (void)
{
    idtable_s *table;
    
    table = malloc(sizeof(*table));
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

void *idtable_insert (idtable_s *table, char *str, tdat_s tdat)
{
    return trie_insert(table, table->root, str, tdat);
}

tlookup_s idtable_lookup (idtable_s *table, char *str)
{
    return trie_lookup(table->root, str);
}

idtnode_s *trie_insert (idtable_s *table, idtnode_s *trie, char *str, tdat_s tdat)
{
    int search;
    idtnode_s *nnode;

    if (!trie->nchildren)
        search = 0; 
    else {
        search = bsearch_tr(trie, *str);
        if (search & 0x8000)
            search &= ~0x8000;
        else if (*str)
            return trie_insert(table, trie->children[search], str+1, tdat);
        else
            return NULL;
    }
    nnode = calloc(1, sizeof(*nnode));
    if (!nnode) {
        perror("Memory Allocation Error");
        exit(EXIT_FAILURE);
    }
    nnode->c = *str;
    if (!*str) {
       /* if (!tdat.is_string && tdat.itype < 0) {
            table->counter++;
            switch (table->mode) {
                case IDT_AUTOINC_TYPE:
                    tdat.itype = table->counter;
                    break;
                case IDT_AUTOINC_ATT:
                    tdat.att = table->counter;
                    break;
                case IDT_AUTOINC_TYPE | IDT_AUTOINC_ATT:
                    tdat.itype = table->counter;
                    tdat.att = table->counter;
                    break;
                case IDT_MANUAL:
                default:
                    break;
            }
            nnode->tdat = tdat;
            nnode->tdat.att = table->counter;
        }*/
        nnode->tdat = tdat;
        parray_insert (trie, search, nnode);
        return nnode;
    }
    parray_insert(trie, search, nnode);
    return trie_insert(table, trie->children[search], str+1, tdat);
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

tlookup_s trie_lookup (idtnode_s *trie, char *str)
{
    uint16_t search;
    
    search = bsearch_tr(trie, *str);
    if (search & 0x8000)
        return (tlookup_s){.is_found = false, .tdat = trie->tdat};
    if (!*str)
        return (tlookup_s){.is_found = true, .tdat = trie->children[search]->tdat};
    return trie_lookup(trie->children[search], str+1);
}

uint16_t bsearch_tr (idtnode_s *tnode, char key)
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
    nm->lexlen = MAX_LEXLEN;
    if (lex->machs)
        nm->next = lex->machs;
    lex->machs = nm;
    lex->nmachs++;
}

int tokmatch(char *buf, token_s *tok)
{
    uint16_t i, len;
    
    if (*buf == EOF)
        return EOF;
    len = strlen(tok->lexeme);
    for (i = 0; i < len; i++) {
        if (buf[i] == EOF)
            return EOF;
        if (buf[i] != tok->lexeme[i])
            return 0;
    }
    return len;
}

match_s nfa_match (lex_s *lex, nfa_s *nfa, nfa_node_s *state, char *buf)
{
    int tmatch, tmpmatch;
    uint16_t i;
    mach_s *tmp;
    match_s curr, result;

    curr.n = 0;
    curr.attribute = 0;
    curr.success = false;
    curr.overflow = false;
    if (state == nfa->final)
        curr.success = true;
    for (i = 0; i < state->nedges; i++) {
        switch (state->edges[i]->token->type.val) {
            case LEXTYPE_EPSILON:
                result = nfa_match(lex, nfa, state->edges[i]->state, buf);
                if (state->edges[i]->annotation.length > -1 && result.n > state->edges[i]->annotation.length) {
                    result.success = false;
                    result.overflow = true;
                }
                if (result.success) {
                    curr.success = true;
                    curr.attribute = (state->edges[i]->annotation.attribute > 0) ? state->edges[i]->annotation.attribute : result.attribute;
                    
                    if (result.n > curr.n)
                        curr.n = result.n;
                }
                break;
            case LEXTYPE_NONTERM:
                tmp = getmach(lex, state->edges[i]->token->lexeme);
                result = nfa_match(lex, tmp->nfa, tmp->nfa->start, buf);
                if (state->edges[i]->annotation.length > -1 && result.n > state->edges[i]->annotation.length) {
                    result.success = false;
                    result.overflow = true;
                }
                if (result.success) {
                    tmpmatch = result.n;
                    result = nfa_match(lex, nfa, state->edges[i]->state, &buf[result.n]);
                    if (result.success) {
                        curr.success = true;
                        if (result.n > 0 && result.attribute > 0)
                            curr.attribute = result.attribute;
                        else if (tmpmatch > 0 && state->edges[i]->annotation.attribute > 0)
                            curr.attribute = state->edges[i]->annotation.attribute;
                        if (result.n + tmpmatch > curr.n)
                            curr.n = result.n + tmpmatch;
                    }
                }
                break;
            default: /* case LEXTYPE_TERM */
                tmatch = tokmatch(buf, state->edges[i]->token);
                if (tmatch && tmatch != EOF) {
                    result = nfa_match(lex, nfa, state->edges[i]->state, &buf[tmatch]);
                    if (state->edges[i]->annotation.length > -1 && result.n > state->edges[i]->annotation.length) {
                        result.success = false;
                        result.overflow = true;
                    }
                    if (result.success) {
                        curr.success = true;
                        curr.attribute = (state->edges[i]->annotation.attribute > 0) ? state->edges[i]->annotation.attribute : result.attribute;
                        if (result.n + tmatch > curr.n)
                            curr.n = result.n + tmatch;
                    }
                }
                break;
        }
    }
    return curr;
}

lextok_s lexf (lex_s *lex, char *buf)
{
    int idatt = 0;
    mach_s *mach, *bmach;
    match_s res, best;
    uint16_t lineno = 1;
    char c[2], *backup, *error;
    tlookup_s lookup;
    token_s *head = NULL, *tlist = NULL;
    
    c[1] = '\0';
    backup = buf;
    println(lineno, buf);
    while (*buf != EOF) {
        best.attribute = 0;
        best.n = 0;
        best.success = false;
        while (*buf <= ' ') {
            if (*buf == '\n') {
                println(lineno, buf+1);
                lineno++;
            }
            else if (*buf == EOF)
                break;
            buf++;
        }
        if (*buf == EOF)
            break;
        for (mach = lex->machs; mach; mach = mach->next) {
            if (*buf == EOF)
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
                if (lookup.is_found) {
                    addtok(&tlist, buf, lineno, lookup.tdat.itype, res.attribute);
                    hashname(lex, lookup.tdat.itype, buf);
                }
                else {
                    lookup = idtable_lookup(lex->idtable, buf);
                    if (lookup.is_found) {
                        addtok(&tlist, buf, lineno, lookup.tdat.itype, lookup.tdat.att);
                        hashname(lex, lookup.tdat.itype, buf);
                    }
                    else if (bmach->attr_id) {
                        idatt++;
                        addtok(&tlist, buf, lineno, bmach->nterm->type.val, idatt);
                        idtable_insert(lex->idtable, buf, (tdat_s){.is_string = false, .itype = bmach->nterm->type.val, .att = idatt});
                        hashname(lex, lex->typestart, bmach->nterm->lexeme);
                    }
                    else {
                        addtok(&tlist, buf, lineno, bmach->nterm->type.val, best.attribute);
                        hashname(lex, lex->typestart, bmach->nterm->lexeme);
                    }
                }
                if (!head)
                    head = tlist;
            }
            else {
                error = make_lexerr(LERR_TOOLONG, lineno, buf);
                addtok(&tlist, c, lineno, LEXTYPE_ERROR, LEXATTR_DEFAULT);
                tlist->error = error;
            }
        }
        else {
            lookup = idtable_lookup(lex->kwtable, c);
            if (lookup.is_found) {
                addtok(&tlist, c, lineno, lookup.tdat.itype, LEXATTR_DEFAULT);
                hashname(lex, lookup.tdat.itype, NULL);
                if (!head)
                    head = tlist;
            }
            else {
                if (best.n) {
                    error = make_lexerr (LERR_UNKNOWNSYM, lineno, buf);
                    addtok(&tlist, c, lineno, LEXTYPE_ERROR, LEXATTR_DEFAULT);
                    tlist->error = error;
                }
                else {
                    error = make_lexerr (LERR_UNKNOWNSYM, lineno, c);
                    addtok(&tlist, c, lineno, LEXTYPE_ERROR, LEXATTR_DEFAULT);
                    tlist->error = error;
                }
            }
        }
        buf[best.n] = c[0];
        if (best.n)
            buf += best.n;
        else
            buf++;
    }
    addtok(&tlist, "$", lineno, LEXTYPE_EOF, LEXATTR_DEFAULT);
    hashname(lex, LEXTYPE_EOF, "$");
    return (lextok_s){.lex = lex, .tokens = head};
}

char *make_lexerr (const char *errmsg, int lineno, char *lexeme)
{
    char *error;
    
    error = calloc(1, strlen(errmsg) + MAX_LEXLEN + INT_CHAR_WIDTH);
    if (!error) {
        perror("Memory Allocation Error");
        exit(EXIT_FAILURE);
    }
    sprintf(error, errmsg, lineno, lexeme);
    return error;
}

mach_s *getmach(lex_s *lex, char *id)
{
    mach_s *iter;
    
    for (iter = lex->machs; iter && strcmp(iter->nterm->lexeme, id); iter = iter->next);
    if (!iter) {
        printf("Regex Error: Regex %s never defined", id);
        exit(EXIT_FAILURE);
    }
    return iter;
}

type_s gettype (lex_s *lex, char *buf)
{
    type_s type;
    size_t len;
    lextok_s ltok;
    token_s *iter, *backup;
    
    len = strlen(buf);
    buf[len] = EOF;
    ltok = lexf(lex, buf);
    buf[len] = '\0';
    type.val = LEXTYPE_ERROR;
    type.attribute = LEXATTR_DEFAULT;
    for (iter = ltok.tokens; iter; iter = iter->next) {
        if (iter->type.val == LEXTYPE_ERROR || iter->type.val == LEXTYPE_EOF)
            continue;
        type = iter->type;
    }
    iter = ltok.tokens;
    while(iter) {
        backup = iter;
        iter = iter->next;
        if (backup->error)
            free(backup->error);
        free(backup);
    }
    return type;
}

inline bool hashname(lex_s *lex, unsigned long token_val, char *name)
{
    return hashinsert(lex->tok_hash, (void *)token_val, name);
}

inline char *getname(lex_s *lex, unsigned long token_val)
{
    return hashlookup(lex->tok_hash, (void *)token_val);
}

unsigned short tok_hashf (void *key)
{
    return (unsigned long)key % HTABLE_SIZE;
}

bool tok_isequalf(void *key1, void *key2)
{
    return key1 == key2;
}