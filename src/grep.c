#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <assert.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <strings.h>
#include <unistd.h>
#include <getopt.h>
#include <regex.h>

#include "./linebuffer.h"


#define COLOR_RED  "\33[31m"
#define COLOR_BOLD "\33[1m"
#define COLOR_END  "\33[0m"



static void error_and_exit(const char *fmt, ...) {
    va_list va;
    va_start(va, fmt);
    fprintf(stderr, "Error: ");
    vfprintf(stderr, fmt, va);
    fprintf(stderr, "\n");
    va_end(va);
    exit(EXIT_FAILURE);
}



typedef struct {
    bool case_sensitive;
    bool inverse_match;
    bool print_count;
    bool use_regex;
    bool extended_regex;
} GrepOptions;

typedef struct {
    const char *filename; // NULL if stdin
    const char *query; // raw query
    LineBuffer file;
    GrepOptions opts;
    regex_t re; // only when using regex
} Grep;








// static void print_line(
//     const char *line,
//     const char *query,
//     size_t *matches,
//     int matchcount
// ) {
//
//     int match_index = 0;
//     for (size_t i=0; i < strlen(line); ++i) {
//
//         /* if current index is within matchlist and matchlist is not empty, */
//         /* print the query, and move to the end of the word                 */
//         bool dont_print = line[i] == ' '; // HACK: inconsistent highlighting
//         if (i == matches[match_index] && matchcount && !dont_print) {
//             printf(
//                 "%s%s%.*s%s",
//                 COLOR_RED, COLOR_BOLD,
//                 (int) strlen(query),
//                 line + i,
//                 COLOR_END
//             );
//             match_index++;
//             i += strlen(query) - 1;
//
//         } else {
//             printf("%c", line[i]);
//         }
//
//     }
//     printf("\n");
//
// }



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
        bool dont_print = line[i] == ' '; // HACK: inconsistent highlighting

        if (i == matches[match_index] && matchcount && !dont_print) {

            size_t end = matches[i+1];
            printf(
                "%s%s%.*s%s",
                COLOR_RED, COLOR_BOLD,
                (int) end,
                line + i,
                COLOR_END
            );
            match_index += 2;
            i += end - 1;

        } else {
            printf("%c", line[i]);
        }

    }
    printf("\n");

}


/* fills the array matches with indices of the occurances of `query` within `str` */
/* the size of matches is assumed to be `strlen(query)`, as thats its size in the worst-case */
static int search_string(
    const char *str,
    const char *query,
    size_t *matches,
    bool case_sensitive
) {
    size_t matchcount = 0;

    /* edge-case: empty query will loop forever */
    if (!strcmp(query, ""))
        return 0;

    const char *s = str;
    while ((s = case_sensitive
        ? strstr(s, query)
        : strcasestr(s, query)) != NULL) {

        matches[matchcount++] = s - str;
        s += strlen(query);
    }

    return matchcount;
}


static int search_string_regex(
    const char *str,
    regex_t *re,
    size_t *matches
) {

    size_t matchcount = 0;

    regmatch_t pmatch = { 0 };
    while (regexec(re, str, 1, &pmatch, 0) != REG_NOMATCH) {
        matches[matchcount++] = pmatch.rm_so;
        matches[matchcount++] = pmatch.rm_eo;
        str += pmatch.rm_eo;
    }

    return matchcount;
}


static void do_grep(Grep *grep) {

    /* "global" in the sense that it only counts each query occurance once in a line */
    int global_matchcount = 0;

    for (size_t i=0; i < grep->file.linecount; ++i) {
        const char *line = grep->file.lines[i];

        size_t matches[strlen(line)]; /* contains start and end index of the matches */
        memset(matches, 0, strlen(line));
        int matchcount = 0;

        if (grep->opts.use_regex)
            matchcount = search_string_regex(line, &grep->re, matches);
        else
            matchcount = search_string(
                line, grep->query,
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
        .filename = filename,
        .query    = query,
        .file     = { 0 },
        .opts     = opts,
        .re       = { 0 },
    };


    if (grep.opts.use_regex) {
        int flags = REG_NEWLINE;

        if (grep.opts.extended_regex)
            flags |= REG_EXTENDED;

        if (!grep.opts.case_sensitive)
            flags |= REG_ICASE;

        regcomp(&grep.re, grep.query, flags);
    }


    if (filename == NULL) {
        linebuffer_read_stdin(&grep.file);
    }
    else {
        int err = linebuffer_read_file(&grep.file, grep.filename);
        if (err != 0)
            error_and_exit("Failed to read file");
    }




    do_grep(&grep);



    if (grep.opts.use_regex)
        regfree(&grep.re);

    linebuffer_destroy(&grep.file);

}


static void print_usage(char **argv) {
    fprintf(stderr, "Usage: %s <file> <query>\n", argv[0]);
}


int main(int argc, char **argv) {

    if (argc < 2) {
        print_usage(argv);
        return EXIT_FAILURE;
    }

    const char *query    = argv[1];
    const char *filename = NULL;

    /* read from stdin */
    if (argc == 2) {
        if (isatty(STDIN_FILENO)) {
            print_usage(argv);
            return EXIT_FAILURE;
        }
    } else {
        filename = !strcmp(argv[2], "-") ? NULL : argv[2];
    }


    GrepOptions opts = {
        .case_sensitive = true,
        .inverse_match  = false,
        .print_count    = false,
        .use_regex      = true,
        .extended_regex = false,
    };

    int opt = 0;
    while ((opt = getopt(argc, argv, "ivcEF")) != -1) {
        switch (opt) {
            case 'i':
                opts.case_sensitive = false;
                break;
            case 'F':
                opts.use_regex = false;
                break;
            case 'E':
                opts.extended_regex = true;
                break;
            case 'v':
                opts.inverse_match = true;
                break;
            case 'c':
                opts.print_count = true;
                break;
            default: {} break;
        }
    }

    grep(opts, filename, query);


    return EXIT_SUCCESS;
}
