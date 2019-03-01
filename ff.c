#include <assert.h>
#include <dirent.h>
#include <errno.h>
#include <getopt.h>
#include <pcre.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    pcre *re;
    pcre_extra *extra;
    pcre_jit_stack *jit_stack;

    unsigned char only_type;
    int hidden_flag;
    int pcre_options;
    long max_depth;
} options;

void process_match(const char *match, options * opt) {
    (void) opt;
    puts(match);
}

void walk(const char *parent, size_t l_parent, options * opt, int depth) {
    if (opt->max_depth > 0 && depth >= opt->max_depth) {
        return;
    }

    DIR *d = opendir(parent);
    if (d == NULL) {
        return;
    }

    struct dirent *entry;
    while ((entry = readdir(d)) != NULL) {
        // Skip current and parent
        if (strcmp(entry->d_name, ".") == 0 ||
            strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        // Skip hidden
        size_t d_namlen = strlen(entry->d_name);
        if (opt->hidden_flag) {
            if (entry->d_name[0] == '.' || entry->d_name[d_namlen - 1] == '~') {
                continue;
            }
        }

        // Assemble filename
        size_t l_current = l_parent + d_namlen + 1;
        char *current = malloc((l_current + 1) * sizeof(char));
        strncpy(current, parent, l_parent);
        strncpy(current + l_parent, "/", 1);
        strncpy(current + l_parent + 1, entry->d_name, d_namlen);
        current[l_current] = '\0';

        int ovector[3];
        int rc = -1;
        if (opt->re != NULL) {
            // Check match
            rc = pcre_jit_exec(opt->re, opt->extra, entry->d_name, d_namlen,
                               0, 0, ovector, 3, opt->jit_stack);
        }
        if (opt->re == NULL || rc > 0) {
            if (opt->only_type == DT_UNKNOWN || opt->only_type == entry->d_type) {
                process_match(current, opt);
            }
        }

        if (entry->d_type == DT_DIR) {
            walk(current, l_current, opt, depth + 1);
        }

        free(current);
    }
    closedir(d);
}

void print_usage(char const * msg) {
    if (msg) {
        fputs(msg, stderr);
        fputs("\n", stderr);
    }
    fputs("Usage: ff [options] [pattern] [directory]\n"
          "Simplified version of GNU find using the PCRE library for regex.\n"
          "\n"
          "Valid options:\n"
          "  --depth <n>     Maximum directory traversal depth\n"
          "  --type (f|d)    Restrict output to type (f = file, d = directory)\n"
          "  -H, --hidden    Traverse hidden directories and files as well\n"
          "  -I, --iregex    Ignore case when applying the regex\n"
          , stderr);
}

int main(int argc, char *argv[]) {
    options opt = {
        .re = NULL,
        .extra = NULL,
        .jit_stack = NULL,

        .only_type = DT_UNKNOWN,
        .hidden_flag = 1,
        .pcre_options = 0,
        .max_depth = -1,
    };

    // Parse options
    int option_index = 0;
    static struct option long_options[] = {
        {"depth", required_argument, NULL, 0},
        {"type", required_argument, NULL, 0},
        {"hidden", no_argument, NULL, 'H'},
        {"iregex", no_argument, NULL, 'I'},
        {0, 0, NULL, 0}};

    int c = -1;
    while ((c = getopt_long(argc, argv, "HI", long_options, &option_index)) !=
           -1) {
        switch (c) {
        case 0:
            if (strcmp(long_options[option_index].name, "type") == 0) {
                assert(optarg);
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
            } else if (strcmp(long_options[option_index].name, "depth") == 0) {
                assert(optarg);
                char *end = optarg + strlen(optarg);
                opt.max_depth = (long)strtoul(optarg, &end, 10);
                if (opt.max_depth == 0 || errno == ERANGE) {
                    print_usage("Invalid argument for --depth");
                    return 1;
                }
            }
            break;
        case 'H':
            opt.hidden_flag = 0;
            break;
        case 'I':
            opt.pcre_options |= PCRE_CASELESS;
            break;
        default:
            print_usage(NULL);
            return 1;
        }
    }

    char *pattern;
    char *directory = ".";
    switch (argc - optind) {
    case 2:
        directory = argv[optind + 1];
        __attribute__((fallthrough));
    case 1:
        pattern = argv[optind];
        break;
    case 0:
         walk(directory, strlen(directory), &opt, 0);
         return 0;
    default:
        print_usage("You need to provide a pattern");
        return 1;
    }

    // Compile pattern
    const char *error;
    int erroffset;
    opt.re = pcre_compile(pattern, opt.pcre_options, &error, &erroffset, NULL);
    if (opt.re == NULL) {
        fprintf(stderr, "Invalid regex: %s at %d\n", error, erroffset);
        return 1;
    }
    opt.extra = pcre_study(opt.re, PCRE_STUDY_JIT_COMPILE, &error);
    assert(opt.extra != NULL);
    opt.jit_stack = pcre_jit_stack_alloc(32 * 1024, 512 * 1024);
    assert(opt.jit_stack != NULL);
    pcre_assign_jit_stack(opt.extra, NULL, opt.jit_stack);

    // Walk the directory tree
    walk(directory, strlen(directory), &opt, 0);

    pcre_free(opt.re);
    pcre_free_study(opt.extra);
    pcre_jit_stack_free(opt.jit_stack);
    return 0;
}
