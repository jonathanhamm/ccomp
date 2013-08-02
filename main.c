#include "lex.h"
#include "general.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAXARG_LEN  1024

#define ARG_CHAR    1
#define ARG_WORD    2
#define ARG_DASH    3
#define ARG_ASSIGN  4

typedef struct argtok_s argtok_s;

struct argtok_s
{
    argtok_s *next;
    int id;
    char lexeme[MAXARG_LEN+1];
};

static void add_argtoken (argtok_s **tlist, const char *lexeme, int id);
static argtok_s *arg_tokenize (int argc, const char *argv[]);

int main(int argc, const char *argv[])
{
    argtok_s *list, *iter;
    lextok_s lextok;
    
    list = arg_tokenize(argc, argv);
    for (iter = list; iter; iter = iter->next)
        printf("%s\n\n", iter->lexeme);
    
    return 0;
    
    if (argc == 1)
        lextok = lex (buildlex ("regex_pascal"), readfile ("samples/lex_sample2"));
    else if (argc == 2)
        lextok = lex (buildlex ("regex_pascal"), readfile (argv[1]));
    else if (argc == 3)
        lextok = lex (buildlex (argv[2]), readfile (argv[1]));
    else
        perror("Argument Error");
    build_parse ("cfg_pascal", lextok);
    return 0;
}

void add_argtoken (argtok_s **tlist, const char *lexeme, int id)
{
    argtok_s *tok;
    
    tok = malloc(sizeof(*tok));
    if (!tok) {
        perror("Memory Allocation Error");
        exit(EXIT_FAILURE);
    }
    tok->id = id;
    tok->next = NULL;
    strcpy(tok->lexeme, lexeme);
    if (*tlist)
        (*tlist)->next = tok;
    *tlist = tok;
}

argtok_s *arg_tokenize (int argc, const char *argv[])
{
    int i, bpos;
    char c;
    char buf[MAXARG_LEN+1];
    argtok_s *head = NULL, *tlist = NULL;
    
    if (argc == 1)
        return NULL;
    for (i = 1; i < argc; i++) {
        while ((c = *argv[i])) {
            bpos = 0;
            switch (c) {
                case '-':
                    add_argtoken (&tlist, "-", ARG_DASH);
                    if (!head)
                        head = tlist;
                    ++argv[i];
                    break;
                case '=':
                    add_argtoken (&tlist, "=", ARG_ASSIGN);
                    if (!head)
                        head = tlist;
                    ++argv[i];
                    break;
                case '\\':
                    c = *++argv[i];
                    if (!c)
                        break;
                default:
                    buf[bpos] = c;
                    ++bpos;
                    if (*++argv[i]) {
                        while ((c = *argv[i])) {
                            if (c == '\\') {
                                c = *++argv[i];
                                if (!c)
                                    break;
                            }
                            if (c == '-' || c == '=')
                                break;
                            if (bpos == MAXARG_LEN) {
                                buf[bpos] = '\0';
                                perror("Error: Too long parameter");
                                exit(EXIT_FAILURE);
                            }
                            buf[bpos] = c;
                            ++bpos;
                            ++argv[i];
                        }
                        buf[bpos] = '\0';
                        add_argtoken (&tlist, buf, ARG_WORD);
                        if (!head)
                            head = tlist;
                    }
                    else {
                        buf[bpos] = '\0';
                        add_argtoken (&tlist, buf, ARG_CHAR);
                        if (!head)
                            head = tlist;
                    }
                    break;
            }
        }
    }
    return head;
}