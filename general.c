#include "general.h"
#include <stdio.h>
#include <stdlib.h>

#define INITFBUF_SIZE 128

u_char *readfile (const char *file)
{
    FILE *f;
    size_t bsize, nbytes;
    u_char *buf;
    
    bsize = INITFBUF_SIZE;
    f = fopen (file, "r");
    if (!f) {
        perror("File IO Error");
        return NULL;
    }
    buf = malloc(INITFBUF_SIZE);
    if (!buf) {
        perror("Heap Allocation Error");
        fclose(f);
        return NULL;
    }
    for (nbytes = 0; (buf[nbytes] = fgetc(f)) != UEOF; nbytes++) {
        if (nbytes+1 == bsize) {
            bsize *= 2;
            buf = realloc(buf, bsize);
            if (!buf) {
                perror("Memory Allocation Error");
                fclose(f);
                return NULL;
            }
        }
    }
    fclose(f);
    if (nbytes < bsize)
        buf = realloc(buf, nbytes+1);
    return buf;
}

void free_llist (void *list)
{
    void *backup;
    
    while (list) {
        backup = list;
        list = *(void **)list;
        free(backup);
    }
}

void println (u_char *buf)
{
    do {
        if (*buf == UEOF)
            return;
        putchar(*buf);
    }
    while (*buf++ != '\n');
}