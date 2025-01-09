#ifndef _LINEBUFFER_H
#define _LINEBUFFER_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>



typedef struct {
    char **lines;
    size_t linecount;
    size_t bufsize;
} LineBuffer; /* In-memory representation of a file/stdin */

/* returns nonzero on failure */
extern int linebuffer_read_file(LineBuffer *lb, const char *filename);
extern void linebuffer_read_stdin(LineBuffer *lb);
extern void linebuffer_destroy(LineBuffer *lb);



#endif // _LINEBUFFER_H
