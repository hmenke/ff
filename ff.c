#ifndef __cplusplus
#define _GNU_SOURCE
#endif

#include "dircolors.h"
#include "flagman.h"
#include "macros.h"
#include "message.h"

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
#include <pthread.h>
#include <sys/sysinfo.h>
#include <unistd.h>

// GNU C library
#include <getopt.h>

// PCRE
#include <pcre.h>

typedef enum { NONE, GLOB, REGEX } match_mode;

typedef struct {
    queue *q;
    flagman *flagman_lock;

    // tagged union
    union {
        pcre *re;
        const char *pattern;
    } match;
    match_mode mode;

    // program parameters
    const char *directory;
    unsigned char only_type;
    bool skip_hidden;
    long max_depth;
    bool colorize;
    bool icase;
} options;

void process_match(const char *realpath, const char *dirname,
                   const char *basename, const options *const opt) {
    if (opt->colorize) {
        printf(DIRCOLOR_DIR "%s/" DIRCOLOR_RESET "%s%s" DIRCOLOR_RESET "\n",
               dirname, dircolor(realpath), basename);
    } else {
        puts(realpath);
    }
}

void walk(const char *parent, const size_t l_parent, const options *const opt,
          const int depth,
          // PCRE
          pcre *re, pcre_extra *extra, pcre_jit_stack *jit_stack,
          // GLOB
          const char *glob_pattern, int glob_flags) {
    if (opt->max_depth > 0 && depth >= opt->max_depth) {
        return;
    }

    foreach_opendir(entry, parent) {
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
        char *current = (char *)malloc((l_current + 1) * sizeof(char));
        strncpy(current, parent, l_parent);
        strncpy(current + l_parent, "/", 1);
        strncpy(current + l_parent + 1, d_name, d_namlen);
        current[l_current] = '\0';

        // Perform the match
        switch (opt->mode) {
        case REGEX: {
            int ovector[3];
            if (pcre_jit_exec(re, extra, d_name, d_namlen, 0, 0, ovector, 3,
                              jit_stack) > 0) {
                goto success;
            }
            break;
        }
        case GLOB:
            if (fnmatch(glob_pattern, d_name, glob_flags) == 0) {
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
        }

        if (entry->d_type == DT_DIR) {
            flagman_acquire(opt->flagman_lock);
            message *m = message_new(depth + 1, l_current, current);
            queue_put(opt->q, m, depth + 1);
        }

        free(current);
    }
}

static void *worker(void *arg) {
    const options *const opt = (options *)arg;

    pcre *re = NULL;
    pcre_extra *extra = NULL;
    pcre_jit_stack *jit_stack = NULL;

    const char *glob_pattern = NULL;
    int glob_flags = 0;

    switch (opt->mode) {
    case REGEX: {
        // Compile pattern
        const char *error;
        extra = pcre_study(opt->match.re, PCRE_STUDY_JIT_COMPILE, &error);
        assert(extra != NULL);
        jit_stack = pcre_jit_stack_alloc(32 * 1024, 512 * 1024);
        assert(jit_stack != NULL);
        pcre_assign_jit_stack(extra, NULL, jit_stack);
    } break;
    case GLOB:
        glob_pattern = opt->match.pattern;
        glob_flags = opt->icase ? FNM_CASEFOLD : 0;
        break;
    case NONE:
        break;
    }

    foreach_queue_get(msg, opt->q) {
        int depth = message_depth(msg);
        size_t l_parent = message_len(msg);
        const char *parent = message_path(msg);

        // Walk the directory tree
        walk(parent, l_parent, opt, depth, re, extra, jit_stack, glob_pattern,
             glob_flags);
        flagman_release(opt->flagman_lock);
    }

    // Cleanup
    switch (opt->mode) {
    case REGEX:
        pcre_free_study(extra);
        pcre_jit_stack_free(jit_stack);
        break;
    case GLOB:
        break;
    case NONE:
        break;
    }

    return NULL;
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
        "  -d, --depth <n>      Maximum directory traversal depth\n"
        "  -t, --type <x>       Restrict output to type with <x> one of\n"
        "                           b   block device.\n"
        "                           c   character device.\n"
        "                           d   directory.\n"
        "                           n   named pipe (FIFO).\n"
        "                           l   symbolic link.\n"
        "                           f   regular file.\n"
        "                           s   UNIX domain socket.\n"
        "  -n, --nthreads <n>   Use <n> threads for parallel directory "
        "traversal\n"
        "  -g, --glob           Match glob instead of regex\n"
        "  -H, --hidden         Traverse hidden directories and files as well\n"
        "  -I, --icase          Ignore case when applying the regex\n"
        "  -h, --help           Display this help and quit\n",
        stderr);
}

int main(int argc, char *argv[]) {
    options opt;
    opt.mode = NONE;
    opt.only_type = DT_UNKNOWN;
    opt.skip_hidden = true;
    opt.max_depth = -1;
    opt.colorize = isatty(fileno(stdout));
    opt.icase = false;

    long nthreads = get_nprocs();

    // Parse options
    int option_index = 0;
    static struct option long_options[] = {
        {"depth", required_argument, NULL, 'd'},
        {"type", required_argument, NULL, 't'},
        {"nthreads", required_argument, NULL, 'N'},
        {"glob", no_argument, NULL, 'g'},
        {"hidden", no_argument, NULL, 'H'},
        {"icase", no_argument, NULL, 'I'},
        {"help", no_argument, NULL, 'h'},
        {NULL, 0, NULL, 0}};

    int c = -1;
    while ((c = getopt_long(argc, argv, "d:t:N:gHIh", long_options,
                            &option_index)) != -1) {
        switch (c) {
        case 'd':
            assert(optarg);
            opt.max_depth = (long)strtoul(optarg, NULL, 0);
            if (opt.max_depth == 0 || errno == ERANGE) {
                print_usage("Invalid argument for --depth");
                return 1;
            }
            break;
        case 't':
            assert(optarg && strlen(optarg) > 0);
            switch (optarg[0]) {
            case 'b':
                opt.only_type = DT_BLK;
                break;
            case 'c':
                opt.only_type = DT_CHR;
                break;
            case 'd':
                opt.only_type = DT_DIR;
                break;
            case 'n':
                opt.only_type = DT_FIFO;
                break;
            case 'l':
                opt.only_type = DT_LNK;
                break;
            case 'f':
                opt.only_type = DT_REG;
                break;
            case 's':
                opt.only_type = DT_SOCK;
                break;
            default:
                print_usage("Invalid argument for --type");
                return 1;
            }
            break;
        case 'g': // glob
            opt.mode = GLOB;
            break;
        case 'H':
            opt.skip_hidden = false;
            break;
        case 'I':
            opt.icase = true;
            break;
        case 'N':
            assert(optarg);
            nthreads = (long)strtoul(optarg, NULL, 0);
            if (nthreads == 0 || errno == ERANGE) {
                print_usage("Invalid argument for --depth");
                return 1;
            }
            break;
        case 'h':
            print_usage(NULL);
            return 0;
        default:
            print_usage(NULL);
            return 1;
        }
    }

    // Scan pattern and directory
    const char *pattern = "";
    opt.directory = ".";
    switch (argc - optind) {
    case 2:
        opt.directory = argv[optind + 1];
        goto pattern;
    case 1: {
        DIR *d = opendir(argv[optind]);
        if (d != NULL) {
            opt.directory = argv[optind];puts("It's a directory!");
            closedir(d);
            goto none;
        }
    pattern:
        pattern = argv[optind];
        if (strlen(pattern) > 0 && opt.mode == NONE) {
            opt.mode = REGEX;
        }
    } break;
    case 0:
    none:
        opt.mode = NONE;
        break;
    default:
        print_usage("You need to provide a pattern");
        return 1;
    }

    // Check if the requested directory even exists
    DIR *d = opendir(opt.directory);
    if (d == NULL) {
        perror(opt.directory);
        print_usage(NULL);
        return 1;
    }
    closedir(d);

    // Set up the pattern matcher
    switch (opt.mode) {
    case REGEX: {
        int flags = opt.icase ? PCRE_CASELESS : 0;

        // Compile pattern
        const char *error;
        int erroffset;
        opt.match.re = pcre_compile(pattern, flags, &error, &erroffset, NULL);
        if (opt.match.re == NULL) {
            fprintf(stderr, "Invalid regex: %s at %d\n", error, erroffset);
            return 1;
        }
    } break;
    case GLOB:
        opt.match.pattern = pattern;
        break;
    case NONE:
        break;
    }

    // Open a new message queue
    opt.q = queue_new();

    // Acquire the flagman lock
    opt.flagman_lock = flagman_new();
    flagman_acquire(opt.flagman_lock);

    // Start threads
    pthread_t *thread = (pthread_t *)malloc(nthreads * sizeof(pthread_t));
    for (int i = 0; i < nthreads; ++i) {
        pthread_create(&thread[i], NULL, &worker, &opt);
    }

    // Send the inital job
    message *msg = message_new(0, strlen(opt.directory), opt.directory);
    queue_put_head(opt.q, msg);

    // Send termination signal
    flagman_wait(opt.flagman_lock);
    for (int i = 0; i < nthreads; ++i) {
        queue_put_tail(opt.q, NULL);
    }

    for (int i = 0; i < nthreads; ++i) {
        pthread_join(thread[i], NULL);
    }

    if (opt.mode == REGEX) {
        pcre_free(opt.match.re);
    }

    free(thread);
    flagman_free(opt.flagman_lock);
    queue_free(opt.q);

    return 0;
}
