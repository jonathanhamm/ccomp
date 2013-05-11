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

#define LEXATTR_DEFAULT     0
#define LEXATTR_WSPACEEOL   1
#define LEXATTR_ERRTOOLONG  0
#define LEXATTR_CHARDIG     0
#define LEXATTR_NCHARDIG    1
#define LEXATTR_BEGINDIG    2

#define INITFBUF_SIZE 128

static void printlist (token_s *list);
static u_char *readfile (const char *file);

static void parseregex (token_s *curr);
static void prx_keywords (token_s *curr);
static void prx_tokens (token_s *curr);
static void prx_tokens_ (token_s *curr);
static void prx_texp (token_s *curr);
static void prx_expression (token_s *curr);
static void prx_expression_ (token_s *curr);
static void prx_term (token_s *curr);
static void prx_term_ (token_s *curr);
static void prx_closure (token_s *curr);

token_s *buildlex (const char *file)
{
    uint32_t i, j;
    u_char *buf;
    uint8_t bpos;
    u_char lbuf[MAX_LEXLEN + 1];
    token_s *head, *curr;
    
    head = curr = NULL;
    buf = readfile(file);
    if (!buf)
        return NULL;
    for (i = 0, bpos = 0; buf[i] != UEOF; i++) {
        if (!head)
            head = curr;
        switch (buf[i]) {
            case '|':
                addtok (&curr, "|", LEXTYPE_UNION, LEXATTR_DEFAULT);
                break;
            case '(':
                addtok (&curr, "(", LEXTYPE_OPENPAREN, LEXATTR_DEFAULT);
                break;
            case ')':
                addtok (&curr, ")", LEXTYPE_CLOSEPAREN, LEXATTR_DEFAULT);
                break;
            case '\\':
                if (buf[i + 1] != UEOF) {
                    i++;
                    if (buf[i] == 'E')
                        addtok (&curr, "E", LEXTYPE_EPSILON, LEXATTR_DEFAULT);
                    else {
                        lbuf[0] = buf[i];
                        lbuf[1] = '\0';
                        addtok (&curr, lbuf, LEXTYPE_TERM, LEXATTR_DEFAULT);
                    }
                }
                break;
            case '*':
                addtok (&curr, "*", LEXTYPE_KLEENE, LEXATTR_DEFAULT);
                break;
            case '+':
                addtok (&curr, "+", LEXTYPE_POSITIVE, LEXATTR_DEFAULT);
                break;
            case '?':
                addtok (&curr, "?", LEXTYPE_ORNULL, LEXATTR_DEFAULT);
                break;
            case '\n':
                addtok (&curr, "EOL", LEXTYPE_EOL, LEXATTR_DEFAULT);
                break;
            case '=':
                if (buf[i+1] == '>') {
                    addtok (&curr, "=>", LEXTYPE_PRODSYM, LEXATTR_DEFAULT);
                    i++;
                }
                else
                    addtok (&curr, "=", LEXTYPE_PRODSYM, LEXATTR_DEFAULT);
                break;
            case '<':
                lbuf[0] = '<';
                for (bpos = 1, i++, j = i; ((buf[i] >= 'a' && buf[i] <= 'z') || (buf[i] >= 'A' && buf[i] <= 'Z')
                                || (buf[i] >= '0' && buf[i] <= '9') || buf[i] == '_'); bpos++, i++)
                {
                    if (bpos == MAX_LEXLEN) {
                        addtok (&curr, lbuf, LEXTYPE_ERROR, LEXATTR_ERRTOOLONG);
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
                    addtok (&curr, lbuf, LEXTYPE_NONTERM, LEXATTR_DEFAULT);
                }
                else {
                    lbuf[bpos] = '\0';
                    addtok (&curr, &lbuf[0], LEXTYPE_TERM, LEXATTR_DEFAULT);
                    i--;
                }
doublebreak_:
                break;
fallthrough_:
            default:
                if (buf[i] <= ' ')
                    break;
                for (bpos = 0, j = LEXATTR_CHARDIG; buf[i] > ' ' && buf[i] != UEOF; bpos++, i++)
                {
                    if (bpos == MAX_LEXLEN) {
                        lbuf[bpos] = '\0';
                        addtok (&curr, lbuf, LEXTYPE_ERROR, LEXATTR_ERRTOOLONG);
                        break;
                    }
                    lbuf[bpos] = buf[i];
                    if (!((buf[i] >= 'A' && buf[i] <= 'Z') || (buf[i] >= 'a' && buf[i] <= 'z') || (buf[i] >= '0' && buf[i] <= '9')))
                        j = LEXATTR_NCHARDIG;
                }
                if (!bpos) {
                    lbuf[0] = buf[i];
                    lbuf[1] = '\0';
                    addtok (&curr, lbuf, LEXTYPE_TERM, j);
                }
                else {
                    lbuf[bpos] = '\0';
                    if (buf[0] >= '0' && buf[0] <= '9')
                        addtok (&curr, lbuf, LEXTYPE_TERM, j);
                    else 
                        addtok (&curr, lbuf, LEXTYPE_TERM, LEXATTR_BEGINDIG);
                    i--;
                }
                break;
        }
    }
}

int addtok (token_s **tlist, u_char *lexeme, uint16_t type, uint16_t attribute)
{
    token_s *ntok;
    
    ntok = calloc(1, sizeof(*ntok));
    if (!ntok) {
        perror("Heap Allocation Error");
        return -1;
    }
    ntok->type.val = type;
    ntok->type.attribute = attribute;
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

void parseregex (token_s *list)
{
    prx_keywords(list);
    prx_tokens(list);
}

void prx_keywords (token_s *curr)
{
    
}

void prx_tokens (token_s *curr)
{
    
}

void prx_tokens_ (token_s *curr)
{
    
}

void prx_texp (token_s *curr)
{
    
}

void prx_expression (token_s *curr)
{
    
}

void prx_expression_ (token_s *curr)
{
    
}

void prx_term (token_s *curr)
{
    
}

void prx_term_ (token_s *curr)
{
    
}

void prx_closure (token_s *curr)
{
}
