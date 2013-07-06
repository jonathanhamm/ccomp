#ifndef GENERAL_H_
#define GENERAL_H_

#include <stdint.h>

#ifndef u_char
typedef unsigned char u_char;
#endif

#define UEOF (u_char)EOF

typedef enum bool {false = 0, true = 1} bool;

extern u_char *readfile (const char *file);

extern void free_llist (void *list);

#endif
