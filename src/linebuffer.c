#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "./linebuffer.h"



/* get linecount, and max line-length of the file, so we can allocate the correct size */
static void parse_file(LineBuffer *lb, FILE *file) {
    size_t bufsize     = 0;
    size_t bufsize_ref = 0;
    size_t nlcount     = 0;

    char c;
    while ((c = fgetc(file)) != EOF) {
        ++bufsize;
        if (c == '\n') {
            ++nlcount;
            if (bufsize > bufsize_ref)
                bufsize_ref = bufsize;
            bufsize = 0;
        }
    }

    /* `-1` to reserve space for a newline on the longest line in the file */
    /* otherwise, fgets() would read another line, which we dont want */
    lb->bufsize = bufsize_ref + 1;
    lb->linecount  = nlcount;
}

int linebuffer_read_file(LineBuffer *lb, const char *filename) {
    memset(lb, 0, sizeof(LineBuffer));

    FILE *file = fopen(filename, "r");
    if (file == NULL)
        return 1;

    parse_file(lb, file);
    rewind(file);

    lb->lines = malloc(lb->linecount * sizeof(char*));
    for (size_t i=0; i < lb->linecount; ++i)
        lb->lines[i] = calloc(lb->bufsize, sizeof(char));

    // TODO: fread
    size_t i = 0;
    while (fgets(lb->lines[i], lb->bufsize, file) != NULL) {
        /* remove newline */
        char *l = lb->lines[i];
        l[strcspn(l, "\n")] = '\0';
        ++i;
    }
    assert(i == lb->linecount);

    fclose(file);
    return 0;
}

void linebuffer_read_stdin(LineBuffer *lb) {
    memset(lb, 0, sizeof(LineBuffer));
    lb-> bufsize = BUFSIZ;

    /* dynarray */
    size_t capacity = 5, size = 0;
    lb->lines = malloc(capacity * sizeof(char*));

    for (size_t i=0; i < capacity; ++i)
        lb->lines[i] = calloc(lb->bufsize, sizeof(char));


    while (fgets(lb->lines[size], lb->bufsize, stdin) != NULL) {

        /* remove newline */
        char *l = lb->lines[size];
        l[strcspn(l, "\n")] = '\0';
        ++size;

        /* realloc */
        if (size == capacity) {
            capacity *= 2;
            lb->lines = realloc(lb->lines, capacity * sizeof(char*));

            for (size_t i=capacity/2; i < capacity; ++i)
                lb->lines[i] = calloc(lb->bufsize, sizeof(char));
        }

    }

    /* free excess memory used by dynarray */
    for (size_t i=size; i < capacity; ++i)
        free(lb->lines[i]);

    lb->linecount = size;
}

void linebuffer_destroy(LineBuffer *lb) {
    for (size_t i=0; i < lb->linecount; ++i)
        free(lb->lines[i]);
    free(lb->lines);
    lb->lines = NULL;
}
