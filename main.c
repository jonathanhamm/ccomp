#include "lex.h"
#include <stdio.h>

int main(int argc, const char * argv[])
{
    token_s *tokens;
        
    tokens = lex (buildlex ("regex_pascal"), readfile ("samples/lex_sample1"));
    
    return 0;
}

