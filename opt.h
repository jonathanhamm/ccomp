#ifndef OPT_H_
#define OPT_H_

#include <stdint.h>

#define TACKLEX_LEN 128


typedef struct tactok_s tactok_s;

struct tactok_s
{
    
    uint8_t type;
    uint8_t att;
    char lexeme[TACKLEX_LEN];
    tactok_s *next;
    tactok_s *prev;
};

#endif