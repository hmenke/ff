#ifndef __cplusplus
#define _GNU_SOURCE
#endif

// C standard library
#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// POSIX C library
#include <dirent.h>
#include <fnmatch.h>

// GNU C library
#include <getopt.h>

// PCRE
#include <pcre.h>

typedef enum {
    NONE,
    GLOB,
    REGEX
} match_mode;

typedef struct {
    pcre *re;
    pcre_extra *extra;
    pcre_jit_stack *jit_stack;
} regex_options;

typedef struct {
    const char *pattern;
    int flags;
} glob_options;

typedef union {
    regex_options regex;
    glob_options glob;
} match_options;

typedef struct {
    match_options opts;
    match_mode mode;
    unsigned char only_type;
    bool skip_hidden;
    long max_depth;
} options;

void process_match(const char *realpath, const char *dirname,
                   const char *basename, options *opt) {
    (void)dirname;
    (void)basename;
    (void)opt;
    puts(realpath);
}

void walk(const char *parent, size_t l_parent, options *opt, int depth) {
    if (opt->max_depth > 0 && depth >= opt->max_depth) {
        return;
    }

    DIR *d = opendir(parent);
    if (d == NULL) {
        return;
    }

    struct dirent *entry;
    while ((entry = readdir(d)) != NULL) {
        const char *d_name = entry->d_name;
        size_t d_namlen = strlen(d_name);

        // Skip current and parent
        if (strcmp(d_name, ".") == 0 || strcmp(d_name, "..") == 0) {
            continue;
        }

        // Skip hidden
        if (opt->skip_hidden) {
            if (d_name[0] == '.' || d_name[d_namlen - 1] == '~') {
                continue;
            }
        }

        // Assemble filename
        size_t l_current = l_parent + d_namlen + 1;
        char *current = (char*)malloc((l_current + 1) * sizeof(char));
        strncpy(current, parent, l_parent);
        strncpy(current + l_parent, "/", 1);
        strncpy(current + l_parent + 1, d_name, d_namlen);
        current[l_current] = '\0';

        // Perform the match
        switch (opt->mode) {
        case REGEX: {
            int ovector[3];
            if (pcre_jit_exec(opt->opts.regex.re, opt->opts.regex.extra, d_name,
                              d_namlen, 0, 0, ovector, 3,
                              opt->opts.regex.jit_stack) > 0) {
                goto success;
            }
            break;
        }
        case GLOB:
            if (fnmatch(opt->opts.glob.pattern, d_name, opt->opts.glob.flags) ==
                0) {
                goto success;
            }
            break;
        case NONE:
        success:
            if (opt->only_type == DT_UNKNOWN ||
                opt->only_type == entry->d_type) {
                process_match(current, parent, d_name, opt);
            }
            break;
        default:
            abort(); // Can't happen
        }

        if (entry->d_type == DT_DIR) {
            walk(current, l_current, opt, depth + 1);
        }

        free(current);
    }
    closedir(d);
}

void print_usage(const char *msg) {
    if (msg) {
        fputs(msg, stderr);
        fputs("\n", stderr);
    }
    fputs(
        "Usage: ff [options] [pattern] [directory]\n"
        "Simplified version of GNU find using the PCRE library for regex.\n"
        "\n"
        "Valid options:\n"
        "  --depth <n>     Maximum directory traversal depth\n"
        "  --type (f|d)    Restrict output to type (f = file, d = directory)\n"
        "  --glob          Match glob instead of regex\n"
        "  -H, --hidden    Traverse hidden directories and files as well\n"
        "  -I, --icase     Ignore case when applying the regex\n",
        stderr);
}

int main(int argc, char *argv[]) {
    options opt;
    opt.mode = NONE;
    opt.only_type = DT_UNKNOWN;
    opt.skip_hidden = true;
    opt.max_depth = -1;

    bool icase = false;

    // Parse options
    int option_index = 0;
    static struct option long_options[] = {
        {"depth", required_argument, NULL, 0},
        {"type", required_argument, NULL, 0},
        {"glob", no_argument, NULL, 0},
        {"hidden", no_argument, NULL, 'H'},
        {"icase", no_argument, NULL, 'I'},
        {0, 0, NULL, 0}};

    int c = -1;
    while ((c = getopt_long(argc, argv, "HI", long_options, &option_index)) !=
           -1) {
        switch (c) {
        case 0:
            switch (option_index) {
            case 0: // depth
                assert(optarg);
                opt.max_depth = (long)strtoul(optarg, NULL, 0);
                if (opt.max_depth == 0 || errno == ERANGE) {
                    print_usage("Invalid argument for --depth");
                    return 1;
                }
                break;
            case 1: // type
                assert(optarg && strlen(optarg) > 0);
                switch (optarg[0]) {
                case 'f':
                    opt.only_type = DT_REG;
                    break;
                case 'd':
                    opt.only_type = DT_DIR;
                    break;
                default:
                    print_usage("Invalid argument for --type");
                    return 1;
                }
                break;
            case 2: // glob
                opt.mode = GLOB;
                break;
            default:
                print_usage(NULL); // Can this even happen?
                return 1;
            }
            break;
        case 'H':
            opt.skip_hidden = false;
            break;
        case 'I':
            icase = true;
            break;
        default:
            print_usage(NULL);
            return 1;
        }
    }

    const char *pattern = "";
    const char *directory = ".";
    switch (argc - optind) {
    case 2:
        directory = argv[optind + 1];
        goto fallthrough;
    case 1:
    fallthrough:
        pattern = argv[optind];
        if (strlen(pattern) > 0 && opt.mode == NONE) {
            opt.mode = REGEX;
        }
        break;
    case 0:
        opt.mode = NONE;
        break;
    default:
        print_usage("You need to provide a pattern");
        return 1;
    }

    switch (opt.mode) {
    case REGEX: {
        opt.opts.regex.re = NULL;
        opt.opts.regex.extra = NULL;
        opt.opts.regex.jit_stack = NULL;

        int flags = icase ? PCRE_CASELESS : 0;

        // Compile pattern
        const char *error;
        int erroffset;
        opt.opts.regex.re =
            pcre_compile(pattern, flags, &error, &erroffset, NULL);
        if (opt.opts.regex.re == NULL) {
            fprintf(stderr, "Invalid regex: %s at %d\n", error, erroffset);
            return 1;
        }
        opt.opts.regex.extra =
            pcre_study(opt.opts.regex.re, PCRE_STUDY_JIT_COMPILE, &error);
        assert(opt.opts.regex.extra != NULL);
        opt.opts.regex.jit_stack = pcre_jit_stack_alloc(32 * 1024, 512 * 1024);
        assert(opt.opts.regex.jit_stack != NULL);
        pcre_assign_jit_stack(opt.opts.regex.extra, NULL,
                              opt.opts.regex.jit_stack);
    } break;
    case GLOB:
        opt.opts.glob.pattern = pattern;
        opt.opts.glob.flags = icase ? FNM_CASEFOLD : 0;
        break;
    case NONE:
        break;
    default:
        abort(); // Can't happen
    }

    // Walk the directory tree
    walk(directory, strlen(directory), &opt, 0);

    // Cleanup
    switch (opt.mode) {
    case REGEX:
        pcre_free(opt.opts.regex.re);
        pcre_free_study(opt.opts.regex.extra);
        pcre_jit_stack_free(opt.opts.regex.jit_stack);
        break;
    case GLOB:
        break;
    case NONE:
        break;
    default:
        abort(); // Can't happen
    }
    return 0;
}
