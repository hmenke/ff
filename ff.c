#ifndef __cplusplus
#define _GNU_SOURCE
#endif

#include "dircolors.h"
#include "flagman.h"
#include "macros.h"
#include "message.h"
#include "options.h"

// C standard library
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// POSIX C library
#include <dirent.h>
#include <fnmatch.h>
#include <pthread.h>
#include <sys/sysinfo.h>
#include <unistd.h>

// PCRE
#include <pcre.h>

// Git
#include <git2.h>

typedef struct {
    int depth;
    size_t len;
    char *str;
    git_repository *repo;
} message_body;

message_body *message_body_new(int depth, size_t len, char *str,
                               git_repository *repo) {
    message_body *msg = (message_body *)malloc(sizeof(message_body));
    msg->depth = depth;
    msg->len = len;
    msg->str = str ? strdup(str) : NULL;
    msg->repo = repo;
    return msg;
}

void message_body_free(void *ptr) {
    message_body *msg = (message_body *)ptr;
    free(msg->str);
    git_repository_free(msg->repo);
    free(msg);
}

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
          const char *glob_pattern, int glob_flags,
          // GIT
          git_repository *repo) {
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

        // Check .gitignore
        if (!opt->no_ignore && repo != NULL) {
            int ignored = 0;
            git_ignore_path_is_ignored(&ignored, repo, entry->d_name);
            if (ignored == 1) {
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
            git_repository *currentrepo = NULL;
            if (git_repository_open(&currentrepo, current) != 0) {
                // If it is not a git repo, duplicate the current handle
                if (repo) {
                    // TODO: This is extremely slow. We should
                    // ref-count repo instead of copy and free each
                    // time.
                    git_repository_open(&currentrepo,
                                        git_repository_workdir(repo));
                }
            }
            message *m = message_new(
                message_body_new(depth + 1, l_current, current, currentrepo),
                message_body_free);
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

    // git_libgit2_init();

    foreach_queue_get(msg, opt->q) {
        message_body *b = message_data(msg);
        int depth = b->depth;
        size_t l_parent = b->len;
        const char *parent = b->str;
        git_repository *repo = b->repo;

        // Walk the directory tree
        walk(parent, l_parent, opt, depth, re, extra, jit_stack, glob_pattern,
             glob_flags, repo);
        flagman_release(opt->flagman_lock);
    }

    // git_libgit2_shutdown();

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

int main(int argc, char *argv[]) {
    options opt;
    opt.mode = NONE;
    opt.only_type = DT_UNKNOWN;
    opt.skip_hidden = true;
    opt.max_depth = -1;
    opt.colorize = isatty(fileno(stdout));
    opt.icase = false;
    opt.no_ignore = false;
    opt.nthreads = get_nprocs();

    switch (parse_options(argc, argv, &opt)) {
    case OPTIONS_SUCCESS:
        break;
    case OPTIONS_FAILURE: // parsing error
        return 1;
    case OPTIONS_HELP: // --help
        return 0;
    }

    // Open a new message queue
    opt.q = queue_new();

    // Acquire the flagman lock
    opt.flagman_lock = flagman_new();
    flagman_acquire(opt.flagman_lock);

    git_libgit2_init();

    // Start threads
    pthread_t *thread = (pthread_t *)malloc(opt.nthreads * sizeof(pthread_t));
    for (int i = 0; i < opt.nthreads; ++i) {
        pthread_create(&thread[i], NULL, &worker, &opt);
    }

    // Send the inital job
    if (opt.optind == argc) {
        git_repository *repo = NULL;
        git_repository_open_ext(&repo, ".", 0, NULL);
        message *msg =
            message_new(message_body_new(0, 1, ".", repo), message_body_free);
        queue_put_head(opt.q, msg);
    }

    for (int arg = opt.optind; arg < argc; ++arg) {
        git_repository *repo = NULL;
        git_repository_open_ext(&repo, argv[arg], 0, NULL);
        message *msg =
            message_new(message_body_new(0, strlen(argv[arg]), argv[arg], repo),
                        message_body_free);
        queue_put_head(opt.q, msg);
    }

    // Send termination signal
    flagman_wait(opt.flagman_lock);
    for (int i = 0; i < opt.nthreads; ++i) {
        queue_put_tail(opt.q, NULL);
    }

    for (int i = 0; i < opt.nthreads; ++i) {
        pthread_join(thread[i], NULL);
    }

    if (opt.mode == REGEX) {
        pcre_free(opt.match.re);
    }

    free(thread);
    git_libgit2_shutdown();
    flagman_free(opt.flagman_lock);
    queue_free(opt.q);

    return 0;
}
