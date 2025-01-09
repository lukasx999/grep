
/* get linecount, and max line-length of the file, so we can allocate the correct size */
static void parse_file(Grep *grep, FILE *file) {
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
    grep->line_bufsize = bufsize_ref + 1;
    grep->lines_count  = nlcount;
}


static void read_file(Grep *grep) {
    FILE *file = fopen(grep->filename, "r");
    if (file == NULL)
        error_and_exit(strerror(errno));

    parse_file(grep, file);
    rewind(file);

    grep->lines = malloc(grep->lines_count * sizeof(char*));
    for (size_t i=0; i < grep->lines_count; ++i)
        grep->lines[i] = calloc(grep->line_bufsize, sizeof(char));

    size_t i = 0;
    while (fgets(grep->lines[i], grep->line_bufsize, file) != NULL) {
        /* remove newline */
        char *l = grep->lines[i];
        l[strcspn(l, "\n")] = '\0';
        ++i;
    }
    assert(i == grep->lines_count);

    fclose(file);
}



static void read_stdin(Grep *grep) {
    grep->line_bufsize = BUFSIZ;

    /* dynarray */
    size_t capacity = 5, size = 0;
    grep->lines = malloc(capacity * sizeof(char*));

    for (size_t i=0; i < capacity; ++i)
        grep->lines[i] = calloc(grep->line_bufsize, sizeof(char));


    while (fgets(grep->lines[size], grep->line_bufsize, stdin) != NULL) {

        /* remove newline */
        char *l = grep->lines[size];
        l[strcspn(l, "\n")] = '\0';
        ++size;

        /* realloc */
        if (size == capacity) {
            capacity *= 2;
            grep->lines = realloc(grep->lines, capacity * sizeof(char*));

            for (size_t i=capacity/2; i < capacity; ++i)
                grep->lines[i] = calloc(grep->line_bufsize, sizeof(char));
        }

    }

    /* free excess memory used by dynarray */
    for (size_t i=size; i < capacity; ++i)
        free(grep->lines[i]);

    grep->lines_count = size;

}


static void free_lines(Grep *grep) {
    for (size_t i=0; i < grep->lines_count; ++i)
        free(grep->lines[i]);
    free(grep->lines);
    grep->lines = NULL;
}
