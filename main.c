#include "lex.h"
#include <stdio.h>

int main(int argc, const char *argv[])
{
    token_s *tokens;
    
    if (argc == 1)
        tokens = lex (buildlex ("regex_pascal", regex_annotate), readfile ("samples/lex_sample2"));
    else if (argc == 2)
        tokens = lex (buildlex ("regex_pascal", regex_annotate), readfile (argv[1]));
    else if (argc == 3)
        tokens = lex (buildlex (argv[2], regex_annotate), readfile (argv[1]));
    else
        perror("Argument Error");
    return 0;
}

