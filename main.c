//
//  main.c
//  pcomp
//
//  Created by Jonathan Hamm on 5/6/13.
//  Copyright (c) 2013 Jonathan Hamm. All rights reserved.
//

#include "lex.h"
#include <stdio.h>

int main(int argc, const char * argv[])
{
    lex (buildlex ("regex_pascal"), readfile ("samples/lex_sample1"));
    return 0;
}

