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
#define LEXTYPE_ESCAPE      7
#define LEXTYPE_RANDCHAR    8
#define LEXTYPE_NID         10
#define LEXTYPE_EPSILON     11
#define LEXTYPE_PRODSYM     12
#define LEXTYPE_NONTERM     13

#define LEXATTR_DEFAULT     0
#define LEXATTR_WSPACEEOL   1
#define LEXATTR_ERRTOOLONG  0

#define INITFBUF_SIZE 128

static void printlist (token_s *list);
static u_char *readfile (const char *file);

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
        if (head == NULL)
            head = curr;
        switch (buf[i]) {
            case '|':
                addtok (&curr, "|", LEXTYPE_UNION, LEXATTR_DEFAULT);
                break;
            case '\\':
                addtok (&curr, "\\", LEXTYPE_ESCAPE, LEXATTR_DEFAULT);
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
                j = i;
                lbuf[0] = '<';
                for (bpos = 1, i++; ((buf[i] >= 'a' && buf[i] <= 'z') || (buf[i] >= 'A' && buf[i] <= 'Z')
                                || (buf[i] >= '0' && buf[i] <= '9')); bpos++, i++)
                {
                    if (bpos == MAX_LEXLEN) {
                        lbuf[bpos] = '\0';
                        addtok (&curr, lbuf, LEXTYPE_ERROR, LEXATTR_ERRTOOLONG);
                        break;
                    }
                    lbuf[bpos] = buf[i];
                }
                if (i == j)
                    addtok (&curr, "<", LEXTYPE_NONTERM, LEXATTR_DEFAULT);
                else if (buf[i] == '>' && bpos != MAX_LEXLEN) {
                    lbuf[bpos] = '>';
                    lbuf[bpos + 1] = '\0';
                    addtok (&curr, lbuf, LEXTYPE_NONTERM, LEXATTR_DEFAULT);
                }
                else {
                    addtok (&curr, "<", LEXTYPE_TERM, LEXATTR_DEFAULT);
                    addtok (&curr, &lbuf[1], LEXTYPE_TERM, LEXATTR_DEFAULT);
                    i--;
                }
                break;
            case EPSILON:
                addtok (&curr, "\219", LEXTYPE_EPSILON, LEXATTR_DEFAULT);
                break;
            default:
                if (buf[i] <= ' ')
                    continue;
                for (bpos = 0; ((buf[i] >= 'a' && buf[i] <= 'z') || (buf[i] >= 'A' && buf[i] <= 'Z')
                    || (buf[i] >= '0' && buf[i] <= '9')); bpos++, i++)
                {
                    if (bpos == MAX_LEXLEN) {
                        lbuf[bpos] = '\0';
                        addtok (&curr, lbuf, LEXTYPE_ERROR, LEXATTR_ERRTOOLONG);
                        break;
                    }
                    lbuf[bpos] = buf[i];
                }
                if (!bpos) {
                    lbuf[0] = buf[i];
                    lbuf[1] = '\0';
                    addtok (&curr, lbuf, LEXTYPE_TERM, LEXATTR_DEFAULT);
                }
                else {
                    lbuf[bpos] = '\0';
                    if (buf[0] >= '0' && buf[0] <= '9')
                        addtok (&curr, lbuf, LEXTYPE_NID, LEXATTR_DEFAULT);
                    else
                        addtok (&curr, lbuf, LEXTYPE_TERM, LEXATTR_DEFAULT);
                    i--;
                }
                break;
        }
    }
    printlist (head);
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
        if (nbytes == bsize) {
            nbytes *= 2;
            buf = realloc(buf, nbytes);
            if (!buf) {
                perror("Heap Allocation Error");
                fclose(f);
                return NULL;
            }
        }
    }
    fclose(f);
    if (nbytes < bsize)
        buf = realloc(buf, nbytes);
    return buf;
}
