#ifndef __cplusplus
#define _GNU_SOURCE
#endif

#include "dircolors.h"
#include "flagman.h"
#include "gitignore.h"
#include "message.h"
#include "options.h"
#include "regex.h"

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

typedef struct _shared_ptr shared_ptr;
struct _shared_ptr {
    gitignore *ptr;
    int *refcnt;
};

shared_ptr make_shared(gitignore *repo) {
    int *refcnt = (int *)malloc(sizeof(int));
    *refcnt = 1;

    shared_ptr s;
    s.ptr = repo;
    s.refcnt = refcnt;

    return s;
}

void free_shared(shared_ptr s) {
    assert(s.refcnt != NULL);
    if (__atomic_sub_fetch(s.refcnt, 1, __ATOMIC_SEQ_CST) == 0) {
        gitignore_free(s.ptr);
        free(s.refcnt);
        s.refcnt = NULL;
    }
}

shared_ptr make_shared_copy(shared_ptr s) {
    __atomic_add_fetch(s.refcnt, 1, __ATOMIC_SEQ_CST);
    return s;
}

typedef struct {
    int depth;
    size_t len;
    char *str;
    shared_ptr repo;
} message_body;

message_body *message_body_new(int depth, size_t len, const char *str,
                               shared_ptr repo) {
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
    free_shared(msg->repo);
    free(msg);
}

void process_match(const char *real_path, const char *dir_name,
                   const char *base_name, const options *const opt) {
    if (opt->colorize) {
        printf(DIRCOLOR_DIR "%s/" DIRCOLOR_RESET "%s%s" DIRCOLOR_RESET "%c",
               dir_name, dircolor(real_path), base_name, opt->delimiter);
    } else {
        printf("%s%c", real_path, opt->delimiter);
    }
}

int cmp(const void *a, const void *b) {
    return strcoll(*(char *const *)a, *(char *const *)b);
}

void walk(const char *parent, const size_t l_parent, const options *const opt,
          const int depth,
          // PCRE
          regex *re, regex_storage *mem,
          // GLOB
          const char *glob_pattern, int glob_flags,
          // GIT
          shared_ptr repo) {
    // If maximum depth is exceeded we stop
    if (opt->max_depth > 0 && depth >= opt->max_depth) {
        return;
    }

    // Traverse the directory
    size_t cnt = 0, len_names = 16;
    char **names = (char **)malloc(len_names * sizeof(char *));
    for (struct dirent *_d = (struct dirent *)opendir(parent), *entry = NULL;
         (DIR *)_d != NULL
         && ((entry = readdir((DIR *)_d)) != NULL || closedir((DIR *)_d));) {
        const char *d_name = entry->d_name;
        size_t d_namlen = strlen(d_name);

        // Skip current and parent
        if (strcmp(d_name, ".") == 0 || strcmp(d_name, "..") == 0) {
            continue;
        }

        // Skip hidden
        if (opt->skip_hidden
            && (d_name[0] == '.')) { // || d_name[d_namlen - 1] == '~')) {
            continue;
        }

        // Assemble full filename
        size_t l_current = l_parent + d_namlen + 1;
        char *current = (char *)malloc((l_current + 1) * sizeof(char));
        strncpy(current, parent, l_parent);
        strncpy(current + l_parent, "/", 1);
        strncpy(current + l_parent + 1, d_name, d_namlen);
        current[l_current] = '\0';

        // Filter by file extension (only files)
        if (opt->ext && entry->d_type == DT_REG) {
            const char *ext = strrchr(d_name, '.');
            if (ext == NULL || strcmp(ext + 1, opt->ext) != 0) {
                free(current);
                continue;
            }
        }

        // Check .gitignore
        if (!opt->no_ignore && repo.ptr != NULL) {
            if (gitignore_is_ignored(repo.ptr, current, l_current,
                                     entry->d_type)) {
                free(current);
                continue;
            }
        }

        // Perform the match
        switch (opt->mode) {
        case REGEX: {
            if (regex_match(re, mem, d_name, d_namlen)) {
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
            if (!(opt->ext && entry->d_type == DT_DIR)
                && (opt->only_type == DT_UNKNOWN
                    || opt->only_type == entry->d_type)) {
                if (__builtin_expect(cnt == len_names, 0)) {
                    len_names *= 2;
                    names = (char **)realloc(names, len_names * sizeof(char *));
                }
                names[cnt++] = current;
            }
            break;
        }

        // If the current item is a directory itself, queue it for
        // traversal
        if (entry->d_type == DT_DIR) {
            // Increment the flagman count
            flagman_acquire(opt->flagman_lock);

            // If this directory is a git repo, open it so we can scan
            // .gitignore
            shared_ptr currentrepo = make_shared(NULL);
            if (!opt->no_ignore) {
                if ((currentrepo.ptr = gitignore_new(current)) == NULL) {
                    // We failed, so we have to free the refcnt of NULL
                    free_shared(currentrepo);
                    // If it is not a git repo, duplicate the current handle
                    currentrepo = make_shared_copy(repo);
                }
            }

            // Queue the new item
            message *m = message_new(
                message_body_new(depth + 1, l_current, current, currentrepo),
                message_body_free);
            queue_put(opt->q, m, depth + 1);
        }
    }

    qsort(names, cnt, sizeof(char *), cmp);
    for (size_t i = 0; i < cnt; ++i) {
        const char *d_name = strrchr(names[i], '/') + 1;
        process_match(names[i], parent, d_name, opt);
        free(names[i]);
    }
    free(names);
}

static void *worker(void *arg) {
    const options *const opt = (options *)arg;

    // Assemble some thread-local storage, such as JIT stack for PCRE
    // or options for globbing
    regex *re = NULL;
    regex_storage *mem = NULL;

    const char *glob_pattern = NULL;
    int glob_flags = 0;

    switch (opt->mode) {
    case REGEX:
        re = opt->match.re;
        mem = regex_storage_new(re);
        break;
    case GLOB:
        glob_pattern = opt->match.pattern;
        glob_flags = opt->icase ? FNM_CASEFOLD : 0;
        break;
    case NONE:
        break;
    }

    // This is the main loop.
    //
    // Each message in the queue is a directory, so we fetch one
    // message from the queue and walk the directory tree.
    for (message *msg = NULL; (msg = queue_get(opt->q)) != NULL;
         message_free(msg)) {
        // Dissect the message
        message_body *b = (message_body *)message_data(msg);
        int depth = b->depth;
        size_t l_parent = b->len;
        const char *parent = b->str;
        shared_ptr repo = b->repo;

        // Walk the directory tree
        walk(parent, l_parent, opt, depth, re, mem, glob_pattern, glob_flags,
             repo);

        // We are finished, so we can decrement the flagman count
        flagman_release(opt->flagman_lock);
    }

    // Cleanup the thread-local state
    switch (opt->mode) {
    case REGEX:
        regex_storage_free(mem);
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

    // Defaults
    opt.mode = NONE;
    opt.only_type = DT_UNKNOWN;
    opt.skip_hidden = true;
    opt.max_depth = -1;
    opt.colorize = isatty(fileno(stdout));
    opt.icase = false;
    opt.no_ignore = false;
    opt.nthreads = get_nprocs();
    opt.ext = NULL;
    opt.delimiter = '\n';
    opt.absolute = false;

    // Parse the command line
    switch (ff_parse_options(argc, argv, &opt)) {
    case OPTIONS_SUCCESS:
        break;
    case OPTIONS_FAILURE: // parsing error
        return 1;
    case OPTIONS_HELP: // --help
        return 0;
    }

    gitignore_init_global();

    // Open a new message queue
    opt.q = queue_new();

    // Acquire the flagman lock
    opt.flagman_lock = flagman_new();
    flagman_acquire(opt.flagman_lock);

    // Start threads
    pthread_t *thread = (pthread_t *)malloc(opt.nthreads * sizeof(pthread_t));
    for (int i = 0; i < opt.nthreads; ++i) {
        pthread_create(&thread[i], NULL, &worker, &opt);
    }

    // Send the inital job
    if (opt.optind == argc) {
        char *path = strdup(".");
        if (opt.absolute) {
            path = realpath(path, NULL);
        }
        shared_ptr repo = make_shared(NULL);
        if (!opt.no_ignore) {
            repo.ptr = gitignore_new(path);
        }
        message *msg = message_new(
            message_body_new(0, strlen(path), path, repo), message_body_free);
        queue_put_head(opt.q, msg);
        free(path);
    }

    for (int arg = opt.optind; arg < argc; ++arg) {
        char *path = argv[arg];
        if (opt.absolute) {
            path = realpath(path, NULL);
        }
        shared_ptr repo = make_shared(NULL);
        if (!opt.no_ignore) {
            repo.ptr = gitignore_new(path);
        }
        message *msg = message_new(
            message_body_new(0, strlen(path), path, repo), message_body_free);
        queue_put_head(opt.q, msg);
        if (opt.absolute) {
            free(path);
        }
    }

    // Send termination signal
    flagman_wait(opt.flagman_lock);
    for (int i = 0; i < opt.nthreads; ++i) {
        queue_put_tail(opt.q, NULL);
    }

    for (int i = 0; i < opt.nthreads; ++i) {
        pthread_join(thread[i], NULL);
    }

    // Cleanup memory
    if (opt.mode == REGEX) {
        regex_free(opt.match.re);
    }

    free(thread);
    flagman_free(opt.flagman_lock);
    queue_free(opt.q);
    gitignore_free_global();

    return 0;
}
