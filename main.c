#include "lex.h"
#include "general.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ARG_CHAR    1
#define ARG_WORD    2
#define ARG_DASH    3
#define ARG_ASSIGN  4
#define ARG_EOF     5

#define DEFAULT_REGEX   "regex_pascal"
#define DEFAULT_CFG     "cfg_pascal"
#define DEFAULT_SOURCE  "samples/lex_sample2"

typedef struct argtok_s argtok_s;
typedef struct files_s files_s;

struct argtok_s
{
    int id;
    char *lexeme;
    argtok_s *next;
};

struct files_s
{
    const char *regex;
    const char *cfg;
    const char *source;
};

static void add_argtoken (argtok_s **tlist, const char *lexeme, int id);
static argtok_s *arg_tokenize (int argc, const char *argv[]);
static void free_tokens (argtok_s *list);

static int check_mode_c (char c);
static int check_mode_w (char *w);

static files_s argsparse_start (argtok_s **curr);
static void argparse_actionlist (argtok_s **curr, files_s *parent);
static void argparse_type (argtok_s **curr, files_s *parent);
static const char **argparse_char (argtok_s **curr, files_s *parent);
static const char **argparse_word (argtok_s **curr, files_s *parent);

int main (int argc, const char *argv[])
{
    files_s files;
    argtok_s *list, *iter;
    lextok_s lextok;
    
    list = arg_tokenize(argc, argv);
    for (iter = list; iter; iter = iter->next)
        printf("%s %d\n\n", iter->lexeme, iter->id);
    iter = list;
    files = argsparse_start(&iter);
    
    free_tokens (list);
    
    lextok = lex(buildlex(files.regex), readfile(files.source));
    
    build_parse (files.cfg, lextok);
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
    tok->lexeme = (char *)lexeme;
    if (*tlist)
        (*tlist)->next = tok;
    *tlist = tok;
}

argtok_s *arg_tokenize (int argc, const char *argv[])
{
    int i;
    char c;
    char *allocated;
    const char *startptr;
    argtok_s *head = NULL, *tlist = NULL;
    
    if (argc == 1)
        return NULL;
    for (i = 1; i < argc; i++) {
        while ((c = *argv[i])) {
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
                    ++argv[i];
                default:
                    startptr = argv[i];
                    if (*++argv[i]) {
                        while ((c = *argv[i])) {
                            if (c == '\\') {
                                c = *++argv[i];
                                if (!c)
                                    break;
                            }
                            if (c == '-' || c == '=') {
                                allocated = malloc(argv[i] - startptr + 1);
                                if (!allocated) {
                                    perror("Memory Allocation Error");
                                    exit(EXIT_FAILURE);
                                }
                                strncpy(allocated, startptr, argv[i] - startptr);
                                allocated[argv[i] - startptr] = '\0';
                                startptr = allocated;
                                break;
                            }
                            ++argv[i];
                        }
                        add_argtoken (&tlist, startptr, ARG_WORD);
                        if (!head)
                            head = tlist;
                    }
                    else {
                        add_argtoken (&tlist, startptr, ARG_CHAR);
                        if (!head)
                            head = tlist;
                    }
                    break;
            }
        }
    }
    add_argtoken(&tlist, "$", ARG_EOF);
    return head;
}

void free_tokens (argtok_s *list)
{
    argtok_s *backup;
    
    while (list) {
        backup = list;
        if (is_allocated(list->lexeme))
            free(list->lexeme);
        list = list->next;
        free(backup);
    }
}

files_s argsparse_start (argtok_s **curr)
{
    files_s files = (files_s){NULL, NULL, NULL};

    if (!*curr)
        return (files_s){.regex = DEFAULT_REGEX, .cfg = DEFAULT_CFG, .source = DEFAULT_SOURCE};
    argparse_actionlist(curr, &files);
    if ((*curr)->id == ARG_EOF) {
        if (!files.regex)
            files.regex = DEFAULT_REGEX;
        if (!files.cfg)
            files.cfg = DEFAULT_CFG;
        if (!files.source)
            files.source = DEFAULT_SOURCE;
        return files;
    }
    else {
        printf("Syntax Error: Extra Token %s\n", (*curr)->lexeme);
        exit(EXIT_FAILURE);
    }
}

void argparse_actionlist (argtok_s **curr, files_s *parent)
{
    switch ((*curr)->id) {
        case ARG_DASH:
            *curr = (*curr)->next;
            argparse_type(curr, parent);
            argparse_actionlist(curr, parent);
            break;
        case ARG_CHAR:
        case ARG_WORD:
            if (!parent->source)
                parent->source = (*curr)->lexeme;
            else {
                printf("Error: Source File Already Specified\n");
                exit(EXIT_FAILURE);
            }
            *curr = (*curr)->next;
            argparse_actionlist(curr, parent);
            break;
        case ARG_EOF:
            break;
        default:
            printf("Syntax Error: Expected '-' or word but got %s\n", (*curr)->lexeme);
            exit(EXIT_FAILURE);
            break;
    }
}

void argparse_type (argtok_s **curr, files_s *parent)
{
    const char **assign;
    
    switch ((*curr)->id) {
        case ARG_CHAR:
            assign = argparse_char(curr, parent);
            break;
        case ARG_DASH:
            *curr = (*curr)->next;
            assign = argparse_word(curr, parent);
            *curr = (*curr)->next;
            if ((*curr)->id == ARG_ASSIGN)
                break;
            else {
                printf("Syntax Error: Expected '=' but got %s\n", (*curr)->lexeme);
                exit(EXIT_FAILURE);
            }
            break;
        default:
            printf("Syntax Error: Expected '-' or character but got %s\n", (*curr)->lexeme);
            exit(EXIT_FAILURE);
            break;
    }
    *curr = (*curr)->next;
    if (*assign) {
        printf("Error: Property Already Assigned\n");
        exit(EXIT_FAILURE);
    }
    if ((*curr)->id == ARG_WORD || (*curr)->id == ARG_CHAR) {
        *assign = (*curr)->lexeme;
        *curr = (*curr)->next;
    }
    else {
        printf("Error: Invalid File Name: %s\n", (*curr)->lexeme);
        exit(EXIT_FAILURE);
    }
}

const char **argparse_char (argtok_s **curr, files_s *parent)
{
    switch (*(*curr)->lexeme) {
        case 'p':
        case 'P':
            return &parent->cfg;
        case 'r':
        case 'R':
            return &parent->regex;
        case 's':
        case 'S':
            return &parent->source;
            break;
        default:
            printf("Error: Undefined Program Option: %s\n", (*curr)->lexeme);
            exit(EXIT_FAILURE);
            break;
    }
}

const char **argparse_word (argtok_s **curr, files_s *parent)
{
    if (strlen((*curr)->lexeme) == 1)
        return argparse_char (curr, parent);
    if (!strcasecmp("regex", (*curr)->lexeme))
        return &parent->regex;
    if (!strcasecmp("cfg", (*curr)->lexeme))
        return &parent->cfg;
    if (!strcasecmp("source", (*curr)->lexeme))
        return &parent->source;
    printf("Error: Undefined Program Option: %s\n", (*curr)->lexeme);
    exit(EXIT_FAILURE);
    return NULL;
}