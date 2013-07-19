#include "lex.h"
#include <stdio.h>

int main(int argc, const char *argv[])
{
    token_s *tokens;
    
    printf("%s\n%s\n", argv[1], argv[2]);
    if (argc == 1)
        tokens = lex (buildlex ("regex_pascal"), readfile ("samples/lex_sample1"));
    else if (argc == 2)
        tokens = lex (buildlex ("regex_pascal"), readfile (argv[1]));
    else if (argc == 3)
        tokens = lex (buildlex (argv[2]), readfile (argv[1]));
    else
        perror("Argument Error");
    return 0;
}

