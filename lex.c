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

typedef struct exp__s exp__s;
typedef struct nodelist_s nodelist_s;
typedef struct lexargs_s lexargs_s;
typedef struct pnonterm_s pnonterm_s;

struct exp__s
{
    int8_t op;
    machnode_s *node;
};

struct nodelist_s
{
    machnode_s *node;
    nodelist_s *next;
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
static machnode_s *prx_expression (lex_s *lex, token_s **curr, machnode_s **uparent, machnode_s *term, bool uparent_update);
static exp__s prx_expression_ (lex_s *lex, token_s **curr, machnode_s **uparent, machnode_s *term, bool uparent_update);
static machnode_s *prx_term (lex_s *lex, token_s **curr, machnode_s **uparent, machnode_s *term, bool uparent_update);
static int prx_closure (lex_s *lex, token_s **curr);

static inline machnode_s *makenode(token_s *token);
static inline void state_union(machnode_s *left, machnode_s *right);
static void state_concat(machnode_s *left, machnode_s *right);
static void addchild (machnode_s *parent, machnode_s *child);
static nodelist_s *getleaves (machnode_s *start);
static void addnode(nodelist_s **list, machnode_s *node);
static int lookupnode (nodelist_s **list, machnode_s *node);
static void traverse (machnode_s *parent, nodelist_s **list);
static void freenodelist(nodelist_s *list);
static int addcycle (machnode_s *node, machnode_s *cycle);
static int addclosure (machnode_s *term, int type);

static bool transparent (machnode_s *node);
static void mscan (lexargs_s *args);
static mach_s *getmach(lex_s *lex, u_char *id);
static pnonterm_s callnonterm (lex_s *lex, u_char *buf, mach_s *machine, machnode_s *start);
static int lmatch (u_char *lexeme, u_char *buf);
static int rlmatch (lex_s *lex, mach_s *mach, machnode_s *machnode, u_char *buf);
static bool resolve_finalstates(lex_s *lex, machnode_s *start);

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
    machnode_s *mcurr;
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

void prx_texp (lex_s *lex, token_s **curr)
{
    uint16_t i;
    machnode_s *node, *expression;
    
    if ((*curr)->type.val == LEXTYPE_NONTERM) {
        addmachine (lex, *curr);
        *curr = (*curr)->next;
        if ((*curr)->type.val == LEXTYPE_PRODSYM) {
            *curr = (*curr)->next;
            node = calloc(1, sizeof(*node));
            if (!node) {
                perror("Heap Allocation Error");
                return;
            }
            node->token = calloc (1, sizeof(*node->token));
            if (!node->token) {
                perror("Heap Allocation Error");
                return;
            }
            node->isfinal = false;
            node->token->type.val = LEXTYPE_START;
            node->token->type.attribute = LEXATTR_DEFAULT;
            snprintf(node->token->lexeme, MAX_LEXLEN, "%s", (*curr)->prev->prev->lexeme);
            expression = prx_expression(lex , curr, &node, NULL, false);
            if (expression != node)
                state_union(node, expression);
            if (resolve_finalstates(lex, node))
                node->isfinal = true;
            lex->machs->start = node;
            printf("\n\n%s   %d\n\n\n", node->token->lexeme, node->isfinal);
        }
        else
            printf("Syntax Error at line %u: Expected '=>' but got: %s\n", (*curr)->lineno, (*curr)->lexeme);
    }
    else
        printf("Syntax Error at line %u: Expected nonterminal: <...>, but got: %s\n", (*curr)->lineno, (*curr)->lexeme);
}

machnode_s *prx_expression (lex_s *lex, token_s **curr, machnode_s **uparent, machnode_s *term, bool uparent_update)
{
    exp__s exp_;
    machnode_s *locterm;
    
    locterm = prx_term(lex, curr, uparent, term, uparent_update);
    switch (prx_closure(lex, curr)) {
        case CLOSTYPE_KLEENE:
            addclosure (locterm, CLOSTYPE_KLEENE);
            break;
        case CLOSTYPE_POS:
            addclosure (locterm, CLOSTYPE_POS);
            break;
        case CLOSTYPE_ORNULL:
            addclosure (locterm, CLOSTYPE_ORNULL);
            break;
        case CLOSTYPE_NONE:
            break;
        default:
            break;
    }
    exp_ = prx_expression_(lex, curr, uparent, term, uparent_update);
    if (term != exp_.node) {
        if (exp_.node) {
            if (exp_.op)
                state_concat (term, exp_.node);
            else
                state_union (*uparent, exp_.node);
        }
    }
    else if (term) {
        printf("EXCEPTIONAL: %s\n", term->token->lexeme);
    }
    return locterm;
}

exp__s prx_expression_ (lex_s *lex, token_s **curr, machnode_s **uparent, machnode_s *term, bool uparent_update)
{
    if ((*curr)->type.val == LEXTYPE_UNION) {
        *curr = (*curr)->next;
        switch ((*curr)->type.val) {
            case LEXTYPE_OPENPAREN:
            case LEXTYPE_TERM:
            case LEXTYPE_NONTERM:
            case LEXTYPE_EPSILON:   
                return (exp__s){0, prx_expression(lex, curr, uparent, term, uparent_update)};
            default:
                *curr = (*curr)->next;
                printf("Syntax Error line %u: Expected '(' , terminal, or nonterminal, but got: %s\n", (*curr)->lineno, (*curr)->lexeme);
                return (exp__s){-1, NULL};
        }
    }
    switch ((*curr)->type.val) {
        case LEXTYPE_OPENPAREN:
        case LEXTYPE_TERM:
        case LEXTYPE_NONTERM:
        case LEXTYPE_EPSILON:
            return (exp__s){1, prx_expression(lex, curr, uparent, term, uparent_update)};
            break;
        default:
            return (exp__s){1, NULL};
    }
}

machnode_s *prx_term (lex_s *lex, token_s **curr, machnode_s **uparent, machnode_s *term, bool uparent_update)
{
    exp__s exp_;
    machnode_s *head, *uparentbackup;
    
    switch((*curr)->type.val) {
        case LEXTYPE_OPENPAREN:
            *curr = (*curr)->next;
            uparentbackup = *uparent;
            if (term)
                *uparent = term;
            head = prx_expression(lex, curr, uparent, term, true);
            if ((*curr)->type.val != LEXTYPE_CLOSEPAREN)
                printf("Syntax Error at line %u: Expected ')' , but got: %s\n", (*curr)->lineno, (*curr)->lexeme);
            *curr = (*curr)->next;
            *uparent = uparentbackup;
            if (!term) {
                printf("Special: %s\n", head->token->lexeme);
                return uparentbackup;
            }
            return term;
        case LEXTYPE_TERM:
        case LEXTYPE_NONTERM:
        case LEXTYPE_EPSILON:
            head = makenode(*curr);
            if (uparent_update) {
                if (term)
                    state_concat(term, head);
                uparent_update = false;
            }
            switch ((*curr)->type.val) {
                case LEXTYPE_TERM:
                    *curr = (*curr)->next;
                    exp_ = prx_expression_(lex, curr, uparent, head, uparent_update);
                    if (head != exp_.node) {
                        if (exp_.node) {
                            if (exp_.op)
                                state_concat (head, exp_.node);
                            else
                                state_union (*uparent, exp_.node);
                        }
                    }
                    else {
                        if (!term)
                            state_concat(*uparent, head);
                    }
                    break;
                case LEXTYPE_NONTERM:
                    *curr = (*curr)->next;
                    exp_ = prx_expression_(lex, curr, uparent, head, uparent_update);
                    if (head != exp_.node) {
                        if (exp_.node) {
                            if (exp_.op)
                                state_concat (head, exp_.node);
                            else
                                state_union (*uparent, exp_.node);
                        }
                    }
                    else {
                        if (!term)
                            state_concat(*uparent, head);
                    }
                    break;
                case LEXTYPE_EPSILON:
                    *curr = (*curr)->next;
                    exp_ = prx_expression_(lex, curr, uparent, head, uparent_update);
                    if (head != exp_.node) {
                        if (exp_.node) {
                            if (exp_.op)
                                state_concat (head, exp_.node);
                            else
                                state_union (*uparent, exp_.node);
                        }
                    }
                    else {
                        if (!term)
                            state_concat(*uparent, head);
                    }
                    break;
            }
            break;
        default:
            printf("Syntax Error at line %u: Expected '(' , terminal , or nonterminal, but got: %s\n", (*curr)->lineno, (*curr)->lexeme);
            return NULL;
    }
    return head;
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

inline machnode_s *makenode(token_s *token)
{
    machnode_s *nnode;
    
    nnode = malloc(sizeof(*nnode));
    if (!nnode) {
        perror("Heap Allocation Error");
        return NULL;
    }
    nnode->nbranches = 0;
    nnode->ncyles = 0;
    nnode->branches = NULL;
    nnode->token = token;
    nnode->isfinal = false;
    return nnode;
}

inline void state_union(machnode_s *left, machnode_s *right)
{
    printf("union: attaching %s to parent: %s\n", right->token->lexeme, left->token->lexeme);
    addchild(left, right);
}

void state_concat(machnode_s *left, machnode_s *right)
{
    nodelist_s *llist, *curr;
    
    llist = getleaves(left);
    for (curr = llist; curr; curr = curr->next) {
        printf("concat: attaching %s to parent %s\n", right->token->lexeme, curr->node->token->lexeme);
        addchild (curr->node, right);
    }
    freenodelist(llist);
}

void addchild (machnode_s *parent, machnode_s *child)
{
    if (!parent->nbranches)
        parent->branches = malloc(sizeof(*parent->branches));
    else
        parent->branches = realloc(parent->branches, (parent->nbranches+1) * sizeof(*parent->branches));
    if (!parent->branches) {
        perror("Heap Allocation Error");
        return;
    }
    if(child == NULL)
        asm("hlt");
    parent->branches[parent->nbranches] = child;
    parent->nbranches++;
}


nodelist_s *getleaves (machnode_s *start)
{
    nodelist_s *list;
    
    list = NULL;
    traverse (start, &list);
    if (!list) {
        list = malloc(sizeof(*list));
        if (!list) {
            perror("Heap Allocation Error");
            return NULL;
        }
        list->node = start;
    }
    return list;
}

void addnode(nodelist_s **list, machnode_s *node)
{
    nodelist_s *n;
    
    n = malloc(sizeof(*n));
    if (!n) {
        perror("Heap Allocation Error");
        return;
    }
    n->node = node;
    n->next = *list;
    *list = n;
}

int lookupnode (nodelist_s **list, machnode_s *node)
{
    nodelist_s *curr;
    
    for (curr = *list; curr; curr = curr->next) {
        if (curr->node == node)
            return 1;
    }
    return 0;
}

void traverse (machnode_s *parent, nodelist_s **list)
{
    uint16_t i;
    
    if (!parent->nbranches)
        addnode(list, parent);
    for (i = 0; i < parent->nbranches; i++)
        traverse(parent->branches[i], list);
} 

void freenodelist (nodelist_s *list)
{
    nodelist_s *backup;
    
    while (list) {
        backup = list;
        list = list->next;
        free(backup);
    }
}

int addcycle (machnode_s *node, machnode_s *cycle)
{
    if (node->ncyles)
        node->loopback = realloc(node->loopback, sizeof(*node->loopback) * (node->ncyles + 1));
    else
        node->loopback = malloc(sizeof(*node->loopback));
    if (!node->loopback) {
        perror("Heap Allocation Error");
        return 0;
    }
    node->loopback[node->ncyles] = cycle;
    node->ncyles++;
    return 1;
}

int addclosure (machnode_s *term, int type)
{
    uint16_t i;
    token_s *epsilon_token;
    machnode_s *epsilon_node;
    nodelist_s *leaves, *iter;
    
    epsilon_token = malloc(sizeof(*epsilon_token));
    if (!epsilon_token) {
        perror("Heap Allocation Error");
        return -1;
    }
    strcpy(epsilon_token->lexeme, "EPSILON");
    epsilon_token->lineno = 0;
    epsilon_token->next = NULL;
    epsilon_token->prev = NULL;
    epsilon_token->type.val = LEXTYPE_EPSILON;
    epsilon_token->type.attribute = LEXATTR_DEFAULT;
    epsilon_node = makenode(epsilon_token);
    printf("Getting leaves for %s, nchildren: %d\n", term->token->lexeme, term->nbranches);
    leaves = getleaves (term);
    for (iter = leaves; iter; iter = iter->next) {
        switch (type) {
            case CLOSTYPE_KLEENE:
                printf("Adding Kleen left associative to: %s from %s\n", term->token->lexeme, iter->node->token->lexeme);
                addcycle(iter->node, term);
                break;
            case CLOSTYPE_POS:
                break;
            case CLOSTYPE_ORNULL:
                break;
        }
    }
    printf("Adding Epsilon to %s\n", term->token->lexeme);
    for (i = 0; i < term->nbranches; i++) {
        if (term->branches[i]->token->type.val == LEXTYPE_EPSILON)
            break;
    }
    if (i < term->nbranches)
        addchild(term, epsilon_node);
    freenodelist(leaves);
}

bool resolve_finalstates(lex_s *lex, machnode_s *start)
{
    uint16_t i;
    bool nterm;
    
    nterm = false;
    if (!start->nbranches) {
        start->isfinal = true;
        if (start->token->type.val == LEXTYPE_NONTERM)
            nterm = resolve_finalstates(lex, getmach(lex, start->token->lexeme)->start);
        if (start->token->type.val == LEXTYPE_EPSILON || nterm)
            return true;
        return false;
    }
    for (i = 0; i < start->nbranches; i++) {
        if (resolve_finalstates(lex, start->branches[i])) {
            start->isfinal = true;
            if (start->token->type.val == LEXTYPE_NONTERM)
                nterm = resolve_finalstates(lex, getmach(lex, start->token->lexeme)->start);
            if (start->token->type.val == LEXTYPE_EPSILON || nterm)
                return true;
        }
    }
    return false;
}

token_s *lex (lex_s *lex, u_char *buf)
{
    int32_t tmp;
    uint16_t i, j, threadsmade;
    lexargs_s *largs, *chosen;
    pthread_t *threads;
    mach_s *machine;
        
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
    free(threads);
}

mach_s *getmach(lex_s *lex, u_char *id)
{
    mach_s *iter;

    for (iter = lex->machs; iter && strcmp(iter->nterm->lexeme, id); iter = iter->next);
    return iter;
}

typedef struct threadlist_s threadlist_s;

struct threadlist_s
{
    threadlist_s *next;
    pthread_t thread;
    lex_s *lex;
    u_char *buf;
    mach_s *machine;
    machnode_s *start;
    pnonterm_s result;
};

static void *callnonterm_wrapper (threadlist_s *node)
{
    node->result = callnonterm (node->lex, node->buf, node->machine, node->start);
    return NULL;
}

static threadlist_s *addthread (threadlist_s **list, lex_s *lex, u_char *buf, mach_s *machine, machnode_s *start)
{
    threadlist_s *nthread;
    
    nthread = malloc(sizeof(*nthread));
    if (!nthread) {
        perror("Heap Allocation Error\n");
        return NULL;
    }
    nthread->lex = lex;
    nthread->buf = buf;
    nthread->machine = machine;
    nthread->start = start;
    if (!*list)
        nthread->next = NULL;
    else {
        nthread->next = *list;
        *list = (*list)->next;
    }
    *list = nthread;
    return nthread;
}

static void free_threadlist (threadlist_s *list)
{
    threadlist_s *backup;
    
    while (list) {
        backup = list;
        list = list->next;
        free(backup);
    }
}

pnonterm_s callnonterm (lex_s *lex, u_char *buf, mach_s *machine, machnode_s *start)
{
    uint16_t i, tmp,
            max, imax, ep,
            localoffset, lastfinal;
    mach_s *mach;
    machnode_s *curr, *gotepsilon;
    pnonterm_s result;
    bool gotfinal;
    threadlist_s *tlist, *tmpnode;
    
    tlist = NULL;
    localoffset = 0;
    lastfinal = 0;
    curr = start;
    gotfinal = false;
    while (true) {
        max = 0;
        gotepsilon = NULL;
        for (i = 0; i < curr->nbranches; i++) {
            tmp = rlmatch (lex, machine, curr->branches[i], &buf[localoffset]);
            if (tmp) {
                if (curr->branches[i]->token->type.val == LEXTYPE_NONTERM) {
                    mach = getmach(lex, curr->branches[i]->token->lexeme);
                    result = callnonterm (lex, &buf[localoffset], mach, mach->start);
                    if (result.success && result.offset > max) {
                        imax = i;
                        max = result.offset;
                        if (curr->branches[i]->isfinal) {
                            gotfinal = true;
                            lastfinal = result.offset;
                        }
                    }
                }
                else {
                    if (tmp > max) {
                        char backup = buf[tmp];
                        buf[tmp] = '\0';
                        printf("tmp: %d\n", tmp);
                        printf("parsed: %s\n", buf);
                        buf[tmp] = backup;
                        imax = i;
                        max = tmp;
                        if (curr->branches[i]->isfinal) {
                            gotfinal = true;
                            lastfinal = localoffset;
                        }
                    }
                }
            }
            else if (curr->branches[i]->token->type.val == LEXTYPE_EPSILON) {
                gotepsilon = curr->branches[i];
                tmpnode = addthread (&tlist, lex, &buf[localoffset], machine, curr->branches[i]);
                pthread_create(&tmpnode->thread, NULL, (void *(*)(void *))callnonterm_wrapper, (void *)tmpnode);
            }
        }
        if (!max) {
            for (i = 0; i < curr->ncyles; i++) {
                tmp = rlmatch (lex, machine, curr->loopback[i], &buf[localoffset]);
                if (tmp) {
                    if (curr->loopback[i]->token->type.val == LEXTYPE_NONTERM) {
                        mach = getmach(lex, curr->loopback[i]->token->lexeme);
                        result = callnonterm (lex, &buf[localoffset], mach, mach->start);
                        if (result.success && result.offset > max) {
                            imax = i;
                            max = result.offset;
                            if (curr->loopback[i]->isfinal) {
                                gotfinal = true;
                                lastfinal = localoffset;
                            }
                        }
                    }
                    else if (tmp > max) {
                        max = tmp;
                        imax = i;
                        if (curr->loopback[i]->isfinal) {
                            gotfinal = true;
                            lastfinal = localoffset;
                        }
                    }
                }
            }
            if (!max) {
                if (gotepsilon) {
                    for (tmpnode = tlist; tmpnode; tmpnode = tmpnode->next)
                        pthread_join(tmpnode->thread, NULL);
                    for (tmpnode = tlist; tmpnode; tmpnode = tmpnode->next) {
                        if (tmpnode->result.success && tmpnode->result.offset > max) {
                            gotfinal = true;
                            max = tmpnode->result.offset;
                            gotepsilon = tmpnode->start;
                        }
                    }
                    free_llist((void *)tlist);
                    tlist = NULL;
                    if (gotfinal)
                        localoffset += max;
                    else
                        return (pnonterm_s) {.success = gotfinal, .offset = localoffset};
                    curr = gotepsilon;
                }
                else
                    return (pnonterm_s) {.success = gotfinal, .offset = localoffset};
            }
            else {
                if (gotepsilon) {
                    ep = 0;
                    for (tmpnode = tlist; tmpnode; tmpnode = tmpnode->next)
                        pthread_join(tmpnode->thread, NULL);
                    for (tmpnode = tlist; tmpnode; tmpnode = tmpnode->next) {
                        if (tmpnode->result.success && tmpnode->result.offset > ep) {
                            gotfinal = true;
                            ep = tmpnode->result.offset;
                            gotepsilon = tmpnode->start;
                        }
                    }
                    free_llist((void *)tlist);
                    tlist = NULL;
                }
                if (ep > max) {
                    localoffset += ep;
                    curr = gotepsilon;
                }
                else {
                    localoffset += max;
                    curr = curr->loopback[imax];
                }
            }
        }
        else {
            if (gotepsilon) {
                ep = 0;
                for (tmpnode = tlist; tmpnode; tmpnode = tmpnode->next)
                    pthread_join(tmpnode->thread, NULL);
                for (tmpnode = tlist; tmpnode; tmpnode = tmpnode->next) {
                    if (tmpnode->result.success && tmpnode->result.offset > ep) {
                        gotfinal = true;
                        ep = tmpnode->result.offset;
                        gotepsilon = tmpnode->start;
                    }
                }
                for(;;)printf("%d %d\n", ep, max);
                free_llist((void *)tlist);
                tlist = NULL;
            }
            if (ep > max) {
                localoffset += ep;
                curr = gotepsilon;
            }
            else {
                localoffset += max;
                curr = curr->branches[imax];
            }
        }
    }
}

bool transparent (machnode_s *node)
{
    uint16_t i;
    
    if (!node->nbranches)
        return true;
    for (i = 0 ; i < node->nbranches; i++) {
        if (node->branches[i]->token->type.val == LEXTYPE_EPSILON)
            return transparent(node->branches[i]);
    }
    return false;
}

void mscan (lexargs_s *args)
{
    pnonterm_s result;
    
    result = callnonterm (args->lex, args->buf, args->machine, args->machine->start);
    args->bread = result.offset;
    args->accepted = result.success;
}

int lmatch (u_char *lexeme, u_char *buf)
{
    uint16_t i;
    
    
    printf("trying to match at lexeme: %s against %s\n", lexeme, buf);
    for (i = 0; lexeme[i]; i++) {
        if (lexeme[i] != buf[i])
            return 0;
    }
    if (lexeme[i])
        return 0;
    printf("matched: %s , %s\n", lexeme, buf);
    return i;
}

int rlmatch (lex_s *lex, mach_s *mach, machnode_s *machnode, u_char *buf)
{
    int tmp;
    uint16_t i;
    mach_s *iter;
    
    if (machnode->token->type.val == LEXTYPE_NONTERM) {
        iter = getmach(lex, machnode->token->lexeme);
        for (i = 0; i < iter->start->nbranches; i++) {
            tmp = rlmatch(lex, iter, iter->start->branches[i], buf);
            if (tmp > 0)
                return tmp;
        }
    }
    return lmatch (machnode->token->lexeme, buf);
}