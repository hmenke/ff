#include "options.h"

// C standard library
#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

// POSIX C library
#include <dirent.h>

// GNU C library
#include <getopt.h>

// PCRE
#include <pcre.h>

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

int parse_options(int argc, char *argv[], options *opt) {
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
            opt->max_depth = (long)strtoul(optarg, NULL, 0);
            if (opt->max_depth == 0 || errno == ERANGE) {
                print_usage("Invalid argument for --depth");
                return 1;
            }
            break;
        case 't':
            assert(optarg && strlen(optarg) > 0);
            switch (optarg[0]) {
            case 'b':
                opt->only_type = DT_BLK;
                break;
            case 'c':
                opt->only_type = DT_CHR;
                break;
            case 'd':
                opt->only_type = DT_DIR;
                break;
            case 'n':
                opt->only_type = DT_FIFO;
                break;
            case 'l':
                opt->only_type = DT_LNK;
                break;
            case 'f':
                opt->only_type = DT_REG;
                break;
            case 's':
                opt->only_type = DT_SOCK;
                break;
            default:
                print_usage("Invalid argument for --type");
                return 1;
            }
            break;
        case 'g': // glob
            opt->mode = GLOB;
            break;
        case 'H':
            opt->skip_hidden = false;
            break;
        case 'I':
            opt->icase = true;
            break;
        case 'N':
            assert(optarg);
            opt->nthreads = (long)strtoul(optarg, NULL, 0);
            if (opt->nthreads == 0 || errno == ERANGE) {
                print_usage("Invalid argument for --nthreads");
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
    opt->directory = ".";
    switch (argc - optind) {
    case 2:
        opt->directory = argv[optind + 1];
        goto pattern;
    case 1: {
        DIR *d = opendir(argv[optind]);
        if (d != NULL) {
            opt->directory = argv[optind];
            puts("It's a directory!");
            closedir(d);
            goto none;
        }
    }
    pattern:
        pattern = argv[optind];
        if (strlen(pattern) > 0 && opt->mode == NONE) {
            opt->mode = REGEX;
        }
        break;
    case 0:
    none:
        opt->mode = NONE;
        break;
    default:
        print_usage("You need to provide a pattern");
        return 1;
    }

    // Check if the requested directory even exists
    DIR *d = opendir(opt->directory);
    if (d == NULL) {
        perror(opt->directory);
        print_usage(NULL);
        return 1;
    }
    closedir(d);

    // Set up the pattern matcher
    switch (opt->mode) {
    case REGEX: {
        int flags = opt->icase ? PCRE_CASELESS : 0;

        // Compile pattern
        const char *error;
        int erroffset;
        opt->match.re = pcre_compile(pattern, flags, &error, &erroffset, NULL);
        if (opt->match.re == NULL) {
            fprintf(stderr, "Invalid regex: %s at %d\n", error, erroffset);
            return 1;
        }
    } break;
    case GLOB:
        opt->match.pattern = pattern;
        break;
    case NONE:
        break;
    }

    return 0;
}
