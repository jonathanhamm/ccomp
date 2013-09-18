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
#include <math.h>

enum basic_ops_ {
    OP_NOP = 0,
    OP_CONCAT,
    OP_UNION,
    OP_GETID,
    OP_GETIDATT,
    OP_NUM
};

#define LERR_PREFIX         "      --Lexical Error: at line: %d: "
#define LERR_UNKNOWNSYM     LERR_PREFIX "Unknown Character: %s"
#define LERR_TOOLONG        LERR_PREFIX "Token too long: %s"
#define LERR_SPECTOOLONG    LERR_PREFIX "%s too long: %s"

#define ISANNOTATE(curr)    ((*curr)->type.val == LEXTYPE_ANNOTATE)

#define MAX_IDLEN 		10
#define MAX_INTLEN		10
#define MAX_REALINT     5
#define MAX_REALFRACT	5
#define MAX_REALEXP     2

#define ANNOTYPE        1
#define EDGETYPE        2

#define INT_CHAR_WIDTH      10

typedef struct exp__s exp__s;
typedef struct nodelist_s nodelist_s;
typedef struct lexargs_s lexargs_s;
typedef struct pnonterm_s pnonterm_s;
typedef struct overflow_s overflow_s;
typedef struct match_s match_s;
typedef struct regex_ann_s regex_ann_s;
typedef struct prxa_expression_s prxa_expression_s;

typedef void (*ann_callback_f) (token_s **, void *);
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

struct overflow_s
{
    size_t len;
    char *str;
};

struct match_s
{
    size_t n;
    int attribute;
    char *stype;
    bool success;
    overflow_s overflow;
};

struct regex_ann_s
{
    int *count;
    mach_s *mach;
    llist_s *matchlist;
};

struct prxa_expression_s
{
    bool isint;
    int ival;
    char *strval;
};

static void printlist (token_s *list);
static void parray_insert (idtnode_s *tnode, uint8_t index, idtnode_s *child);
static uint16_t bsearch_tr (idtnode_s *tnode, char key);
static idtnode_s *trie_insert (idtable_s *table, idtnode_s *trie, char *str, tdat_s tdat);
static tlookup_s trie_lookup (idtnode_s *trie, char *str);
static unsigned regex_annotate (token_s **tlist, char *buf, unsigned *lineno, void *data);

static lex_s *lex_s_ (void);
static idtnode_s *patch_search (llist_s *patch, char *lexeme);
static int parseregex (lex_s *lex, token_s **curr);
static void prx_keywords (lex_s *lex, token_s **curr, int *count);
static void prx_tokens (lex_s *lex, token_s **curr, int *count);
static void prx_tokens_ (lex_s *lex, token_s **curr, int *count);
static regex_ann_s *prx_texp (lex_s *lex, token_s **curr, int *count);
static nfa_s *prx_expression (lex_s *lex, token_s **curr, nfa_s **unfa, nfa_s **concat);
static exp__s prx_expression_ (lex_s *lex, token_s **curr, nfa_s **unfa, nfa_s **concat);
static nfa_s *prx_term (lex_s *lex, token_s **curr, nfa_s **unfa, nfa_s **concat);
static int prx_closure (lex_s *lex, token_s **curr);

static void prxa_annotation(token_s **curr, void *ptr, regex_callback_f callback);
static void prxa_regexdef(token_s **curr, regex_ann_s *reg);
static void setann_val(int *location, int value);
static void prxa_edgestart(token_s **curr, nfa_edge_s *edge);
static void prxa_assigment(token_s **curr, nfa_edge_s *edge);
static prxa_expression_s prxa_expression(token_s **curr, nfa_edge_s *edge);
static void prxa_expression_(token_s **curr, nfa_edge_s *edge);

static void regexdef_callback (token_s **curr, mach_s *mach);
static void nfaedge_callback (token_s **curr, nfa_edge_s *edge);
static int prx_annotation (nfa_edge_s *edge, token_s **curr, void *ptr, ann_callback_f callback);
static int prxann_expressionlist (nfa_edge_s *edge, token_s **curr, void *ptr, ann_callback_f callback);
static int prxann_expression (nfa_edge_s *edge, token_s **curr, void *ptr, ann_callback_f callback);
static int prxann_expression_ (nfa_edge_s *edge, token_s **curr, void *ptr, ann_callback_f callback);
static void prxann_equals (nfa_edge_s *edge, token_s **curr, void *ptr, ann_callback_f callback);

static int prx_tokenid (lex_s *lex, token_s **curr);
static void addcycle (nfa_node_s *start, nfa_node_s *dest);
match_s nfa_match (lex_s *lex, nfa_s *nfa, nfa_node_s *state, char *buf);
static char *make_lexerr (const char *errstr, int lineno, char *lexeme);
static void mscan (lexargs_s *args);
static mach_s *getmach(lex_s *lex, char *id);

/*
 rule1:
    rule2 | rule3 | rule4 
 
 rule2:
    nonterm1
 
 rule3:
    nonterm2
 
 rule4: 
    nonterm4
    |
    rule5
 
 rule5:
    nonterm5
 
 */

lex_s *buildlex (const char *file)
{
    token_s *list;
    lex_s *lex;

    lex = lex_s_();
    list = lexspec (file, regex_annotate, NULL);
    lex->typestart = parseregex(lex, &list);
    return lex;
}

token_s *lexspec (const char *file, annotation_f af, void *data)
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
                addtok(&list, "|", lineno, LEXTYPE_UNION, LEXATTR_DEFAULT, NULL);
                break;
            case '(':
                addtok(&list, "(", lineno, LEXTYPE_OPENPAREN, LEXATTR_DEFAULT, NULL);
                break;
            case ')':
                addtok(&list, ")", lineno, LEXTYPE_CLOSEPAREN, LEXATTR_DEFAULT, NULL);
                break;
            case '*':
                addtok(&list, "*", lineno, LEXTYPE_KLEENE, LEXATTR_DEFAULT, NULL);
                break;
            case '+':
                addtok(&list, "+", lineno, LEXTYPE_POSITIVE, LEXATTR_DEFAULT, NULL);
                break;
            case '?':
                addtok(&list, "?", lineno, LEXTYPE_ORNULL, LEXATTR_DEFAULT, NULL);
                break;
            case '\n':
                lineno++;
                addtok (&list, "EOL", lineno, LEXTYPE_EOL, LEXATTR_DEFAULT, NULL);
                break;
            case (char)0xCE:
                if (buf[i+1] == (char)0xB5)
                    addtok(&list, "EPSILON", lineno, LEXTYPE_EPSILON, LEXATTR_DEFAULT, NULL);
                i++;
                break;
            case (char)0xE2:
                if (buf[i+1] == (char)0xA8) {
                    if (buf[i+2] == (char)0xAF) {
                        strcpy(lbuf, "\xE2\xA8\xAF");
                        addtok(&list, lbuf, lineno, LEXTYPE_TERM, LEXATTR_DEFAULT, NULL);
                        i += 2;
                    }
                }
                break;
            case '{':
                tmp = af(&list, &buf[i], &lineno, data);
                if (tmp)
                    i += tmp;
                else {
                    perror("Error Parsing Regex");
                    exit(EXIT_FAILURE);
                }
                break;
            case '-':
                if (buf[i+1] == '>') {
                    addtok (&list, "->", lineno, LEXTYPE_PRODSYM, LEXATTR_DEFAULT, NULL);
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
                        addtok (&list, lbuf, lineno, LEXTYPE_ERROR, LEXATTR_ERRTOOLONG, NULL);
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
                    addtok (&list, lbuf, lineno, LEXTYPE_NONTERM, LEXATTR_DEFAULT, NULL);
                }
                else {
                    lbuf[bpos] = '\0';
                    addtok (&list, &lbuf[0], lineno, LEXTYPE_TERM, LEXATTR_DEFAULT, NULL);
                    i--;
                }
                break;
            case '"':
                bpos = 0;
                while(buf[++i] != '"') {
                    if(buf[i] == EOF) {
                        fprintf(stderr, "Lexical Error: Failed to match \" at line %d", lineno);
                        exit(EXIT_FAILURE);
                    }
                    if (buf[i] == '\n')
                        lineno++;
                    lbuf[bpos++] = buf[i];
                    if (bpos == MAX_LEXLEN) {
                        lbuf[bpos] = '\0';
                        addtok(&list, lbuf, lineno, LEXTYPE_CODE, LEXATTR_DEFAULT, "code");
                        bpos = 0;
                    }
                }
                if (bpos) {
                    buf[i] = '\0';
                    addtok(&list, lbuf, lineno, LEXTYPE_CODE, LEXATTR_DEFAULT, "code");
                }
                break;
default_:
            default:
                if (isspace(buf[i]))
                    break;
                for (bpos = 0, j = LEXATTR_CHARDIG; buf[i] > ' ' && buf[i] != EOF; bpos++, i++)
                {
                    if (bpos == MAX_LEXLEN) {
                        lbuf[bpos] = '\0';
                        addtok (&list, lbuf, lineno, LEXTYPE_ERROR, LEXATTR_ERRTOOLONG, NULL);
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
                        case '/':
                            if (bpos > 0) {
                                lbuf[bpos] = '\0';
                                addtok (&list, lbuf, lineno, LEXTYPE_TERM, j, NULL);
                            }
                            i--;
                            goto doublebreak_;
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
                    addtok (&list, lbuf, lineno, LEXTYPE_TERM, j, NULL);
                }
                else {
                    lbuf[bpos] = '\0';
                    if (isdigit(buf[i]))
                        addtok (&list, lbuf, lineno, LEXTYPE_TERM, j, NULL);
                    else 
                        addtok (&list, lbuf, lineno, LEXTYPE_TERM, LEXATTR_BEGINDIG, NULL);
                    i--;
                }
                break;
        }
doublebreak_:
        ;
    }
    addtok(&list, "$", lineno, LEXTYPE_EOF, LEXATTR_DEFAULT, NULL);
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

unsigned regex_annotate (token_s **tlist, char *buf, unsigned *lineno, void *data)
{
    unsigned i = 0, bpos = 0;
    char tmpbuf[MAX_LEXLEN+1];

    buf++;
    while (buf[i] != '}') {
        while (isspace(buf[i])) {
            if (buf[i] == '\n')
                ++*lineno;
            i++;
        }
        if (buf[i] == '}')
            continue;
        bpos = 0;
        if (isalpha(buf[i]) || buf[i] == '<') {
            tmpbuf[bpos] = buf[i];
            for (bpos++, i++; isalpha(buf[i]) || isdigit(buf[i]) || buf[i] == '>'; bpos++, i++) {
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
            addtok(tlist, tmpbuf, *lineno, LEXTYPE_ANNOTATE, LEXATTR_WORD, NULL);
        }
        else if (isdigit(buf[i])) {
            tmpbuf[bpos] = buf[i];
            for (bpos++, i++; isdigit(buf[i]); bpos++, i++) {
                if (bpos == MAX_LEXLEN)
                    return false;
                tmpbuf[bpos] = buf[i];
            }
            tmpbuf[bpos] = '\0';
            addtok(tlist, tmpbuf, *lineno, LEXTYPE_ANNOTATE, LEXATTR_NUM, NULL);
        }
        else if (buf[i] == '=') {
            i++;
            addtok(tlist, "=", *lineno, LEXTYPE_ANNOTATE, LEXATTR_EQU, NULL);
        }
        else if (buf[i] == ',') {
            i++;
            addtok(tlist, ",", *lineno, LEXTYPE_ANNOTATE, LEXATTR_COMMA, NULL);
        }
        else {
            printf("Illegal character sequence in annotation at line %u: %c\n", *lineno, buf[i]);
            exit(EXIT_FAILURE);
        }
    }
    addtok(tlist, "$", *lineno, LEXTYPE_ANNOTATE, LEXATTR_FAKEEOF, NULL);
    return i+1;
}

int addtok (token_s **tlist, char *lexeme, uint32_t lineno, uint16_t type, uint16_t attribute, char *stype)
{
    token_s *ntok;
    
    ntok = calloc(1, sizeof(*ntok));
    if (!ntok) {
        perror("Memory Allocation Error");
        return -1;
    }
    ntok->type.val = type;
    ntok->type.attribute = attribute;
    ntok->lineno = lineno;
    ntok->stype = stype;
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
    if ((*list)->type.val != LEXTYPE_EOL) {
        printf("Syntax Error at line %u: Expected EOL but got %s\n", (*list)->lineno, (*list)->lexeme);
        exit(EXIT_FAILURE);
    }

    *list = (*list)->next;
    prx_tokens(lex, list, &types);
    if ((*list)->type.val != LEXTYPE_EOF) {
        printf("Syntax Error at line %u: Expected $ but got %s\n", (*list)->lineno, (*list)->lexeme);
        exit(EXIT_FAILURE);
    }
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
        idtable_insert(lex->kwtable, lexeme, (tdat_s){.is_string = false, .itype = *counter, .att = 0});
        *curr = (*curr)->next;
    }
    ++*counter;
}

void prx_tokens (lex_s *lex, token_s **curr, int *count)
{
    prx_texp(lex, curr, count);
    prx_tokens_(lex, curr, count);
}

void prx_tokens_ (lex_s *lex, token_s **curr, int *count)
{
    regex_ann_s *reg;
    mach_s *mach;
    llist_s *matches = NULL, *mcurr, *miter;
    
    if ((*curr)->type.val == LEXTYPE_EOL) {
        *curr = (*curr)->next;
        reg = prx_texp (lex, curr, count);
        if (reg)
            llpush(&matches, reg);
        prx_tokens_(lex, curr, count);
    }
    while (matches) {
        mcurr = llpop(&matches);
        reg = mcurr->ptr;
        for (miter = reg->matchlist; miter; miter = miter->next) {
            for (mach = lex->machs; mach; mach = mach->next) {
                printf("Comparing %s %s\n", mach->nterm->lexeme, miter->ptr);
                if (!ntstrcmp(mach->nterm->lexeme, miter->ptr))
                    break;
            }
            if (!mach) {
                printf("Error: regex %s not found.\n", miter->ptr);
                exit(EXIT_FAILURE);
            }
            mach->nterm->type = reg->mach->nterm->type;
        }
        free_llist(reg->matchlist);
        free(mcurr);
    }
}
/*********************************** EXPERIMENTAL NEW ROUTINES *************************************/
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

regex_ann_s *prx_texp (lex_s *lex, token_s **curr, int *count)
{
    idtnode_s *tnode = NULL;
    nfa_s *uparent = NULL, *concat = NULL;
    regex_ann_s *reg;
    
    if ((*curr)->type.val == LEXTYPE_NONTERM) {
        addmachine(lex, *curr);
        *curr = (*curr)->next;
        ++*count;
        lex->machs->nterm->type.val = *count;
        reg = malloc(sizeof(*reg));
        if (!reg) {
            perror("Memory Allocation Error");
            exit(EXIT_FAILURE);
        }
        reg->count = count;
        reg->mach = lex->machs;
        reg->matchlist = NULL;
        prxa_annotation(curr, reg, (void (*)(token_s **, void *))prxa_regexdef);
        if (!reg->matchlist) {
            free(reg);
            reg = NULL;
        }
        tnode = patch_search(lex->patch, lex->machs->nterm->lexeme);
        if ((*curr)->type.val == LEXTYPE_PRODSYM) {
            *curr = (*curr)->next;
            lex->machs->nfa = prx_expression(lex, curr, &uparent, &concat);
            if (uparent)
                lex->machs->nfa = uparent;
        }
        else {
            printf("Syntax Error at line %u: Expected '->' but got: %s\n", (*curr)->lineno, (*curr)->lexeme);
            exit(EXIT_FAILURE);
        }
    }
    else {
        printf("Syntax Error at line %u: Expected nonterminal: <...>, but got: %s\n", (*curr)->lineno, (*curr)->lexeme);
        exit(EXIT_FAILURE);
    }
    return reg;
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
            addedge(clos_nfa->final, nfa_edge_s_(make_epsilon(), term->start));
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
                exit(EXIT_FAILURE);
                /* make compiler happy */
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
            if ((*curr)->type.val != LEXTYPE_CLOSEPAREN) {
                printf("Syntax Error at line %u: Expected ')' , but got: %s\n", (*curr)->lineno, (*curr)->lexeme);
                exit(EXIT_FAILURE);
            }
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
            exit(EXIT_FAILURE);
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

void prxa_regexdef(token_s **curr, regex_ann_s *reg)
{
    mach_s *mach = reg->mach;

    while (ISANNOTATE(curr) && (*curr)->type.attribute != LEXATTR_FAKEEOF) {
        if (!strcasecmp((*curr)->lexeme, "typecount"))
            mach->typecount = true;
        else if (!strcasecmp((*curr)->lexeme, "idtype")) {
            mach->attr_id = true;
            mach->composite = false;
        }
        else if (!strcasecmp((*curr)->lexeme, "definition")) {
            --*reg->count;
            mach->composite = true;
            mach->attr_id = false;
        }
        else if (!strcasecmp((*curr)->lexeme, "length")) {
            *curr = (*curr)->next;
            if (ISANNOTATE(curr) && (*curr)->type.attribute == LEXATTR_EQU) {
                *curr = (*curr)->next;
                if (ISANNOTATE(curr) && (*curr)->type.attribute == LEXATTR_NUM)
                    mach->lexlen = safe_atol((*curr)->lexeme);
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
        else if (!strcasecmp((*curr)->lexeme, "type")) {
            *curr = (*curr)->next;
            if (ISANNOTATE(curr) && (*curr)->type.attribute == LEXATTR_EQU) {
                *curr = (*curr)->next;
                if (ISANNOTATE(curr)) {
                    switch((*curr)->type.attribute) {
                        case LEXATTR_WORD:
                            llpush(&reg->matchlist, (*curr)->lexeme);
                            break;
                        case LEXATTR_NUM:
                            mach->nterm->type.val = safe_atol((*curr)->lexeme);
                            break;
                        default:
                            printf("Error: Expected word or number but got %s\n", (*curr)->lexeme);
                            exit(EXIT_FAILURE);
                            break;
                    }
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
        if (ISANNOTATE(curr) && (*curr)->type.attribute == LEXATTR_COMMA)
            *curr = (*curr)->next;
    }
}

void prxa_assignment(token_s **curr, nfa_edge_s *edge)
{
    prxa_expression_s val;
    char *str;
    
    if (ISANNOTATE(curr)) {

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
    if (!strcasecmp(str, "attribute")) {
        assert(val.isint);
        setann_val(&edge->annotation.attribute, val.ival);
    }
    else if (!strcasecmp(str, "length")) {
        assert(val.isint);
        setann_val(&edge->annotation.length, val.ival);
    }
    else if (!strcasecmp(str, "type")) {
        assert(!val.isint);
        edge->annotation.type = val.strval;
    }
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
                edge->annotation.attribute = safe_atol((*curr)->lexeme);
                *curr = (*curr)->next;
                break;
        }
    }
    else {
        printf("Error parsing regex annotation at line %d: %s\n", (*curr)->lineno, (*curr)->lexeme);
        exit(EXIT_FAILURE);
    }
}

prxa_expression_s prxa_expression(token_s **curr, nfa_edge_s *edge)
{
    prxa_expression_s ret = {0};
    
    if (ISANNOTATE(curr)) {
        switch ((*curr)->type.attribute) {
            case LEXATTR_EQU:
                *curr = (*curr)->next;
                if (ISANNOTATE(curr)) {
                    if ((*curr)->type.attribute == LEXATTR_NUM) {
                        ret.isint = true;
                        ret.ival = safe_atol((*curr)->lexeme);
                    }
                    else if ((*curr)->type.attribute == LEXATTR_WORD) {
                        ret.isint = false;
                        ret.strval = (*curr)->lexeme;
                    }
                    *curr = (*curr)->next;
                    prxa_expression_(curr, edge);
                }
                else {
                    printf("Error parsing regex annotation at line %d : %s\n", (*curr)->lineno, (*curr)->lexeme);
                    exit(EXIT_FAILURE);
                }
                break;
            case LEXATTR_FAKEEOF:
                return (prxa_expression_s){.isint = true, .ival = -1, .strval = NULL};
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
    lex->tok_hash = hash_(basic_hashf, basic_isequalf);
    lex->listing = linetable_s_();
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

int ntstrcmp (char *nterm, char *str)
{
    int i, result;
    char *ptr;
    
    ptr = &nterm[1];
    for (i = 0; ptr[i] != '>'; i++);
    ptr[i] = '\0';
    result = strcmp(ptr, str);
    ptr[i] = '>';
    return result;
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
    size_t tmatch, tmpmatch;
    uint16_t i;
    mach_s *tmp;
    match_s curr, result;

    curr.n = 0;
    curr.stype = NULL;
    curr.attribute = 0;
    curr.success = false;
    curr.overflow.str = NULL;
    curr.overflow.len = 0;
    if (state == nfa->final)
        curr.success = true;
    for (i = 0; i < state->nedges; i++) {
        switch (state->edges[i]->token->type.val) {
            case LEXTYPE_EPSILON:
                result = nfa_match(lex, nfa, state->edges[i]->state, buf);
                if (result.success) {
                    curr.success = true;
                    curr.stype = (state->edges[i]->annotation.type) ? state->edges[i]->annotation.type : result.stype;
                    curr.attribute = (state->edges[i]->annotation.attribute > 0) ? state->edges[i]->annotation.attribute : result.attribute;
                    if (result.n > curr.n)
                        curr.n = result.n;
                }
                break;
            case LEXTYPE_NONTERM:
                tmp = getmach(lex, state->edges[i]->token->lexeme);
                result = nfa_match(lex, tmp->nfa, tmp->nfa->start, buf);
                if (state->edges[i]->annotation.length > -1 && result.n > state->edges[i]->annotation.length) {
                    result.overflow.str = state->edges[i]->token->lexeme;
                    result.overflow.len = result.n;
                }
                else if (result.success) {
                    tmpmatch = result.n;
                    result = nfa_match(lex, nfa, state->edges[i]->state, &buf[result.n]);
                    if (result.success) {
                        curr.success = true;
                        if (result.n > 0 && result.attribute > 0)
                            curr.attribute = result.attribute;
                        else if (tmpmatch > 0 && state->edges[i]->annotation.attribute > 0)
                            curr.attribute = state->edges[i]->annotation.attribute;
                        if (state->edges[i]->annotation.type)
                            curr.stype = state->edges[i]->annotation.type;
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
                        curr.attribute = (state->edges[i]->annotation.attribute > 0) ? state->edges[i]->annotation.attribute : result.attribute;
                        curr.stype = (state->edges[i]->annotation.type) ? state->edges[i]->annotation.type : result.stype;
                        if (result.n + tmatch > curr.n)
                            curr.n = result.n + tmatch;
                    }
                }
                break;
        }
    }
    return curr;
}

lextok_s lexf (lex_s *lex, char *buf, uint32_t linestart, bool listing)
{
    int idatt = 0;
    mach_s *mach, *bmach;
    match_s res, best;
    uint16_t lineno = linestart;
    char c[2], *backup, *error, tmpbuf[MAX_LEXLEN];
    tlookup_s lookup;
    token_s *head = NULL, *tlist = NULL;
    overflow_s overflow;
    
    c[1] = '\0';
    backup = buf;
    if (listing && *buf != EOF) {
        addline(&lex->listing, buf);
        lineno++;
    }
    while (*buf != EOF) {
        best.attribute = 0;
        best.n = 0;
        best.success = false;
        best.stype = NULL;
        overflow.str = NULL;
        overflow.len = 0;
        
        while (isspace(*buf)) {
            if (*buf == '\n') {
                lineno++;
                if (listing)
                    addline(&lex->listing, buf+1);
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
            if (!res.overflow.str &&  res.n <= mach->lexlen) {
                if (res.success && !mach->composite && res.n > best.n) {
                    best = res;
                    bmach = mach;
                }
            }
            else {
                if (!res.overflow.str) {
                    overflow.str = buf;
                    overflow.len = res.n;
                    best.success = false;
                }
                else
                    overflow = res.overflow;
            }
        }
        c[0] = buf[best.n];
        buf[best.n] = '\0';
        if (best.success) {
            if (best.n <= bmach->lexlen) {
                lookup = idtable_lookup(lex->kwtable, buf);
                if (lookup.is_found) {
                    addtok(&tlist, buf, lineno, lookup.tdat.itype, best.attribute, best.stype);
                    hashname(lex, lookup.tdat.itype, buf);
                }
                else {
                    lookup = idtable_lookup(lex->idtable, buf);
                    if (lookup.is_found) {
                        addtok(&tlist, buf, lineno, lookup.tdat.itype, lookup.tdat.att, best.stype);
                        hashname(lex, lookup.tdat.itype, buf);
                    }
                    else if (bmach->attr_id) {
                        idatt++;
                        addtok(&tlist, buf, lineno, bmach->nterm->type.val, idatt, best.stype);
                        idtable_insert(lex->idtable, buf, (tdat_s){.is_string = false, .itype = bmach->nterm->type.val, .att = idatt});
                        hashname(lex, lex->typestart, bmach->nterm->lexeme);
                    }
                    else {
                        addtok(&tlist, buf, lineno, bmach->nterm->type.val, best.attribute, best.stype);
                        hashname(lex, lex->typestart, bmach->nterm->lexeme);
                    }
                }
                if (!head)
                    head = tlist;
            }
            else {
                addtok(&tlist, c, lineno, LEXTYPE_ERROR, LEXATTR_DEFAULT, best.stype);
                if (listing) {
                    error = make_lexerr(LERR_TOOLONG, lineno, buf);
                    adderror(lex->listing, error, lineno);
                }
            }
        }
        else if (overflow.str) {
            buf[best.n] = c[0];
            c[0] = buf[overflow.len];
            buf[overflow.len] = '\0';
            memset(tmpbuf, 0, sizeof(tmpbuf));
            snprintf(tmpbuf, MAX_LEXLEN, "%s", buf);
            addtok(&tlist, tmpbuf, lineno, LEXTYPE_ERROR, LEXATTR_DEFAULT, best.stype);
            if (listing)
                adderror(lex->listing, make_lexerr (LERR_TOOLONG, lineno, buf), lineno);
            best.n = overflow.len;
        }
        else {
            lookup = idtable_lookup(lex->kwtable, c);
            if (lookup.is_found) {
                addtok(&tlist, c, lineno, lookup.tdat.itype, LEXATTR_DEFAULT, best.stype);
                hashname(lex, lookup.tdat.itype, NULL);
                if (!head)
                    head = tlist;
            }
            else {
                if (best.n) {
                    addtok(&tlist, c, lineno, LEXTYPE_ERROR, LEXATTR_DEFAULT, best.stype);
                    if (listing) {
                        assert(*buf != EOF);
                        error = make_lexerr(LERR_UNKNOWNSYM, lineno, buf);
                        adderror(lex->listing, error, lineno);
                    }
                }
                else {
                    addtok(&tlist, c, lineno, LEXTYPE_ERROR, LEXATTR_DEFAULT, best.stype);
                    if (listing) {
                        assert(c[0] != EOF);
                        error = make_lexerr(LERR_UNKNOWNSYM, lineno, c);
                        adderror(lex->listing, error, lineno);
                    }
                }
            }
        }
        buf[best.n] = c[0];
        if (best.n)
            buf += best.n;
        else 
            buf++;
    }
    addtok(&tlist, "$", lineno, LEXTYPE_EOF, LEXATTR_DEFAULT, best.stype);
    if (!head)
        head = tlist;
    hashname(lex, LEXTYPE_EOF, "$");
    return (lextok_s){.lex = lex, .lines = lineno, .tokens = head};
}

char *make_lexerr (const char *errmsg, int lineno, char *lexeme)
{
    char *error;
    size_t errsize;
    
    errsize = strlen(errmsg) + strlen(lexeme) + INT_CHAR_WIDTH;
    error = calloc(1, errsize);
    if (!error) {
        perror("Memory Allocation Error");
        exit(EXIT_FAILURE);
    }
    sprintf(error, errmsg, lineno, lexeme);
    error[errsize-1] = '\n';
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
    token_s *iter;
    
    len = strlen(buf);
    buf[len] = EOF;
    ltok = lexf(lex, buf, 1, false);
    buf[len] = '\0';
    type.val = LEXTYPE_ERROR;
    type.attribute = LEXATTR_DEFAULT;
    for (iter = ltok.tokens; iter; iter = iter->next) {
        if (iter->type.val == LEXTYPE_ERROR || iter->type.val == LEXTYPE_EOF)
            continue;
        type = iter->type;
    }
    iter = ltok.tokens;
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