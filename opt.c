#include "opt.h"
#include "general.h"
#include <stdlib.h>
#include <stdio.h>

static tactok_s *tac_toks, *tac_tail;
static hash_s *tacsym;

#define TAC_ALPHA       0x8000U
#define TAC_NUM         0x4000U
#define TAC_UNDER       0x2000U
#define TAC_DOT         0x1000U
#define TAC_EQ          0x0800U
#define TAC_LESS        0x0400U
#define TAC_MORE        0x0200U
#define TAC_SUB         0x0100U
#define TAC_MUL         0x0080U
#define TAC_DIV         0x0040U
#define TAC_COL         0x0020U
#define TAC_ALPHAFIRST  0x0010U
#define TAC_NUMFIRST    0x0008U
#define TAC_WHITESPACE  0x0004U
#define TAC_UNDERFIRST  0x0002U
#define TAC_CLEAR       0x0000U


enum {
    TACTOK_LABEL,
    TACTOK_BEGINPROGRAM,
    
}
toktypes;

static void taclex(const char *filename);
static void addtok(uint8_t type, uint8_t att, char *bptr, char *fptr);

void taclex(const char *filename)
{
    int c;
    FILE *f;
    uint16_t state = TAC_CLEAR;
    char *buffer, *bptr, *fptr;
    
    f = fopen(filename, "r");
    if(!f){
        perror("File IO Error");
        exit(EXIT_FAILURE);
    }
    
    buffer = readfile(filename);
    tacsym = hash_(pjw_hashf, str_isequalf);
    
    for(fptr = bptr = buffer; *fptr != EOF; fptr++) {
        switch(*fptr) {
            case ' ':
            case '\t':
            case '\n':
            case '\r':
            case '\v':
                state |= TAC_WHITESPACE;
                break;
            case ':':
                state |= TAC_COL;
                break;
            case '_':
                state |= TAC_UNDER;
                break;
            case '.':
                state |= TAC_DOT;
                break;
            case '=':
                state |= TAC_EQ;
                break;
            case '<':
                state |= TAC_LESS;
                break;
            case '>':
                state |= TAC_MORE;
                break;
            case '-':
                state |= TAC_SUB;
                break;
            case '*':
                state |= TAC_MUL;
                break;
            case '/':
                state |= TAC_DIV;
                break;
            default:
                if(
                   (*fptr >= 'a' && *fptr <= 'z') ||
                   (*fptr >= 'A' && *fptr <= 'Z')
                   ) {
                        if(state | TAC_CLEAR)
                            state |= TAC_ALPHAFIRST;
                        state |= TAC_ALPHA;
                   }
                else if(*fptr >= '1' && *fptr <= '9'){
                    state |= TAC_NUM;
                }
                else {
                    fprintf(stderr, "Lexical Error: Unknown Character: %c\n", *fptr);
                }
                break;
        }
    }
}

void addtok(uint8_t type, uint8_t att, char *bptr, char *fptr)
{
    tactok_s *t;
    char *lex, backup;
    
    t = malloc(sizeof(*t));
    if(!t){
        perror("Memory Allocation Error");
        exit(EXIT_FAILURE);
    }
    t->type = type;
    t->att = att;
    lex = t->lexeme;
    if(fptr - bptr >= TACKLEX_LEN) {
        backup = *fptr;
        *fptr = '\0';
        fprintf(stderr, "Error: Lexeme too long: %s\n", bptr);
        *fptr = backup;
        free(t);
    }
    else {
        while(bptr < fptr)
            *lex++ = *bptr++;
        if(!tac_toks){
            t->next = NULL;
            t->prev = NULL;
            tac_toks = tac_tail = t;
        }
        else{
            t->prev = tac_tail;
            t->next = NULL;
            tac_tail->next = t;
            tac_tail = t;
        }
    }
}
