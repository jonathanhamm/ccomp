/*
 Author: Jonathan Hamm
 
 lex.c
 */

#include "lex.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

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

#define LEXATTR_DEFAULT     0
#define LEXATTR_WSPACEEOL   1
#define LEXATTR_ERRTOOLONG  0
#define LEXATTR_EOLNEWPROD  1
#define LEXATTR_CHARDIG     0
#define LEXATTR_NCHARDIG    1
#define LEXATTR_BEGINDIG    2

#define INITFBUF_SIZE 128

static void printlist (token_s *list);
static u_char *readfile (const char *file);

static void parseregex (token_s **curr);
static void prx_keywords (token_s **curr);
static void prx_tokens (token_s **curr);
static void prx_tokens_ (token_s **curr);
static void prx_texp (token_s **curr);
static void prx_expression (token_s **curr);
static void prx_expression_ (token_s **curr);
static void prx_term (token_s **curr);
static void prx_union (token_s **curr);
static void prx_closure (token_s **curr);

token_s *buildlex (const char *file)
{
    uint32_t i, j, lineno;
    u_char *buf;
    uint8_t bpos;
    u_char lbuf[2*MAX_LEXLEN + 1];
    token_s *list, *backup;
    
    list = NULL;
    buf = readfile(file);
    if (!buf)
        return NULL;
    for (i = 0, lineno = 1, bpos = 0; buf[i] != UEOF; i++) {
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
            case EPSILON:
                addtok(&list, EPSILONSTR, lineno, LEXTYPE_EPSILON, LEXATTR_DEFAULT);
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
                        case EPSILON:
                        case NULLSET:
                            if (bpos > 0) {
                                lbuf[bpos] = '\0';
                                addtok (&list, lbuf, lineno, LEXTYPE_TERM, j);
                            }
                            i--;
                            goto doublebreak_;
                        default:
                            break;
                    }
                    if (buf[i] == '\\')
                        i++;
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
    printlist(list);
    parseregex(&list);
}

int addtok (token_s **tlist, u_char *lexeme, uint32_t lineno, uint16_t type, uint16_t attribute)
{
    token_s *ntok;
    
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
    return 0;
}

void printlist (token_s *list)
{
    while (list) {
        printf("%s\n", list->lexeme);
        list = list->next;
    }
}

static u_char *readfile (const char *file)
{
    FILE *f;
    size_t bsize, nbytes;
    u_char *buf;
    
    bsize = INITFBUF_SIZE;
    f = fopen (file, "r");
    if (!f) {
        perror("File IO Error");
        return NULL;
    }
    buf = malloc(INITFBUF_SIZE);
    if (!buf) {
        perror("Heap Allocation Error");
        fclose(f);
        return NULL;
    }
    for (nbytes = 0; (buf[nbytes] = fgetc(f)) != UEOF; nbytes++) {
        if (nbytes+1 == bsize) {
            bsize *= 2;
            buf = realloc(buf, bsize);
            if (!buf) {
                perror("Heap Allocation Error");
                fclose(f);
                return NULL;
            }
        }
    }
    fclose(f);
    if (nbytes < bsize)
        buf = realloc(buf, nbytes+1);
    return buf;
}

void parseregex (token_s **list)
{
    prx_keywords(list);
    if ((*list)->type.val != LEXTYPE_EOL)
        printf("Syntax Error at line %u: Expected EOL but got %s\n", (*list)->lineno, (*list)->lexeme);
    *list = (*list)->next;
    prx_tokens(list);
    if ((*list)->type.val != LEXTYPE_EOF)
        printf("Syntax Error at line %u: Expected $ but got %s\n", (*list)->lineno, (*list)->lexeme);
        
}

void prx_keywords (token_s **curr)
{
    while ((*curr)->type.val == LEXTYPE_TERM) {
        
        *curr = (*curr)->next;
    }
}

void prx_tokens (token_s **curr)
{
    prx_texp (curr);
    prx_tokens_(curr);
}

void prx_tokens_ (token_s **curr)
{
    if ((*curr)->type.val == LEXTYPE_EOL) {
        *curr = (*curr)->next;
        prx_texp (curr);
        prx_tokens_ (curr);
    }
}

void prx_texp (token_s **curr)
{
    if ((*curr)->type.val == LEXTYPE_NONTERM) {
        *curr = (*curr)->next;
        if ((*curr)->type.val == LEXTYPE_PRODSYM) {
            *curr = (*curr)->next;
            prx_expression(curr);
        }
        else
            printf("Syntax Error at line %u: Expected '=>' but got: %s\n", (*curr)->lineno, (*curr)->lexeme);
    }
    else
        printf("Syntax Error at line %u: Expected nonterminal: <...>, but got: %s\n", (*curr)->lineno, (*curr)->lexeme);
}

void prx_expression (token_s **curr)
{
    prx_term(curr);
    prx_closure(curr);
    prx_expression_(curr);
}

void prx_expression_ (token_s **curr)
{
    prx_union(curr);
    switch ((*curr)->type.val) {
        case LEXTYPE_OPENPAREN:
        case LEXTYPE_TERM:
        case LEXTYPE_NONTERM:
            prx_expression(curr);
            break;
        default:
            break;
    }
}

void prx_term (token_s **curr)
{
    switch((*curr)->type.val) {
        case LEXTYPE_OPENPAREN:
            *curr = (*curr)->next;
            prx_expression(curr);
            if ((*curr)->type.val != LEXTYPE_CLOSEPAREN)
                printf("Syntax Error at line %u: Expected ')' , but got: %s\n", (*curr)->lineno, (*curr)->lexeme);
            *curr = (*curr)->next;
            break;
        case LEXTYPE_TERM:
        case LEXTYPE_NONTERM:
            *curr = (*curr)->next;
            prx_union(curr);
            prx_expression_(curr);
            break;
        default:
            printf("Syntax Error at line %u: Expected '(' , terminal , or nonterminal, but got: %s\n", (*curr)->lineno, (*curr)->lexeme);
            break;
    }
    
}

void prx_union (token_s **curr)
{
    if ((*curr)->type.val == LEXTYPE_UNION) {
        *curr = (*curr)->next;
        
    }
}

void prx_closure (token_s **curr)
{
    switch((*curr)->type.val) {
        case LEXTYPE_KLEENE:
        case LEXTYPE_POSITIVE:
        case LEXTYPE_ORNULL:
            *curr = (*curr)->next;
            prx_closure (curr);
            break;
        default:
            break;
    }
}