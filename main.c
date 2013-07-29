#include "lex.h"
#include <stdio.h>

int main(int argc, const char *argv[])
{
    lextok_s lextok;
    
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

