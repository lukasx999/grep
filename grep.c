#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <assert.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <getopt.h>



#define COLOR_RED  "\33[31m"
#define COLOR_BOLD "\33[1m"
#define COLOR_END  "\33[0m"



typedef struct {
    bool case_sensitive;
    bool inverse_match;
    bool print_count;
} GrepOptions;

typedef struct {
    const char *filename;
    const char *query; // TODO: regex query
    char **lines;
    size_t lines_count;
    size_t line_bufsize; /* size of the longest line in the file */
    GrepOptions opts;
} Grep;



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
    // TODO: dynarray
    (void) grep;
    assert(false);
}


static void free_lines(Grep *grep) {
    for (size_t i=0; i < grep->lines_count; ++i)
        free(grep->lines[i]);
    free(grep->lines);
    grep->lines = NULL;
}




/* fills the array matches with indices of the occurances of `query` within `str` */
/* the size of matches is assumed to be `strlen(query)`, as thats its size in the worst-case */
static int search_string(
    const char *str,
    const char *query,
    size_t *matches,
    bool case_sensitive
) {
    int matchcount = 0;

    const char *s = str;
    while ((s = case_sensitive
        ? strstr(s, query)
        : strcasestr(s, query)) != NULL) {

        matches[matchcount++] = s - str;
        s += strlen(query);
    }

    return matchcount;
}


static void print_line(
    const char *line,
    const char *query,
    size_t *matches,
    int matchcount
) {

    int match_index = 0;
    for (size_t i=0; i < strlen(line); ++i) {

        /* if current index is within matchlist and matchlist is not empty, */
        /* print the query, and move to the end of the word                 */
        if (i == matches[match_index] && matchcount) {
            printf(
                "%s%s%.*s%s",
                COLOR_RED,
                COLOR_BOLD,
                (int) strlen(query),
                line + i,
                COLOR_END
            );
            match_index++;
            i += strlen(query) - 1;

        } else {
            printf("%c", line[i]);
        }

    }
    printf("\n");

}


static void do_grep(Grep *grep) {

    /* "global" in the sense that it only counts each query occurance */
    /* in a line once */
    int global_matchcount = 0;

    for (size_t i=0; i < grep->lines_count; ++i) {
        const char *line = grep->lines[i];

        size_t matches[strlen(line)];
        memset(matches, 0, strlen(line));
        int matchcount = search_string(
            line,
            grep->query,
            matches,
            grep->opts.case_sensitive
        );

        if (!matchcount != grep->opts.inverse_match)
            continue;

        if (grep->opts.print_count) {
            global_matchcount++;
            continue;
        }

        print_line(line, grep->query, matches, matchcount);

    }

    if (grep->opts.print_count)
        printf("%d\n", global_matchcount);

}


static void grep(GrepOptions opts, const char *filename, const char *query) {
    Grep grep = {
        .filename           = filename,
        .query              = query,
        .lines              = NULL,
        .line_bufsize       = 0,
        .lines_count        = 0,
        .opts               = opts,
    };

    if (filename == NULL)
        read_stdin(&grep);
    else
        read_file(&grep);

    do_grep(&grep);
    free_lines(&grep);
}



int main(int argc, char **argv) {

    if (argc < 3) {
        fprintf(stderr, "Usage: %s <file> <query>\n", argv[0]);
        return EXIT_FAILURE;
    }

    const char *query    = argv[1];
    const char *filename = argv[2];

    // TODO: make filename optional if !isatty(0)
    if (!strcmp(filename, "-"))
        filename = NULL;


    GrepOptions opts = {
        .case_sensitive = true,
        .inverse_match  = false,
        .print_count    = false,
    };

    int opt = 0;
    while ((opt = getopt(argc, argv, "ivc")) != -1) {
        switch (opt) {
            case 'i': {
                opts.case_sensitive = false;
            } break;
            case 'v': {
                opts.inverse_match = true;
            } break;
            case 'c': {
                opts.print_count = true;
            } break;
            default: {} break;
        }
    }

    grep(opts, filename, query);


    return EXIT_SUCCESS;
}
