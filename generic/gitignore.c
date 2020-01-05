#include "gitignore.h"

// git
#include "dir.h"
#include "git-compat-util.h"

// POSIX C library
#include <sys/stat.h>

// C standard library
#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct pattern_list *pattern_list_new(const char *path) {
    struct pattern_list *pl = malloc(sizeof(*pl));
    pl->nr = 0;
    pl->alloc = 0;
    pl->filebuf = NULL;
    pl->src = (path != NULL) ? strdup(path) : NULL;
    pl->patterns = NULL;
    return pl;
}

void pattern_list_free(struct pattern_list *pl) {
    if (pl == NULL) {
        return;
    }
    for (int i = 0; i < pl->nr; ++i) {
        free(pl->patterns[i]);
    }
    free(pl->patterns);
    free(pl->filebuf);
    free((void *)pl->src);
    free(pl);
    pl = NULL;
}

bool isfile(const char *path) {
    struct stat statbuf;
    if (lstat(path, &statbuf) != 0) {
        return false;
    }
    return S_ISREG(statbuf.st_mode);
}

bool isdir(const char *path) {
    struct stat statbuf;
    if (lstat(path, &statbuf) != 0) {
        return false;
    }
    return S_ISDIR(statbuf.st_mode);
}

static gitignore *gitignore_global = NULL;

void gitignore_init_global() {
    // Check for and parse the global .gitignore file
    //
    // TODO: Check ~/.gitconfig for core.excludesfile
    const char *home = NULL;
    char *ignorehome = NULL;
    if ((home = getenv("XDG_CONFIG_HOME")) != NULL) {
        size_t homelen = strlen(home);
        ignorehome = (char *)malloc((strlen(home) + sizeof("/git/ignore"))
                                    * sizeof(char));
        memcpy(ignorehome, home, homelen);
        memcpy(ignorehome + homelen, "/git/ignore", sizeof("/git/ignore"));
    } else if ((home = getenv("HOME")) != NULL) {
        size_t homelen = strlen(home);
        ignorehome = (char *)malloc(
            (strlen(home) + sizeof("/.config/git/ignore")) * sizeof(char));
        memcpy(ignorehome, home, homelen);
        memcpy(ignorehome + homelen, "/.config/git/ignore",
               sizeof("/.config/git/ignore"));
    }

    if (ignorehome == NULL || !isfile(ignorehome)) {
        free(ignorehome);
        gitignore_global = NULL;
        return;
    }

    struct pattern_list *pl = pattern_list_new(NULL);
    if (add_patterns_from_file_to_list(ignorehome, "", 0, pl, NULL) != 0) {
        pattern_list_free(pl);
        free(ignorehome);
        return;
    }

    free(ignorehome);
    gitignore_global = pl;
}

void gitignore_free_global() { pattern_list_free(gitignore_global); }

gitignore *gitignore_new(const char *path) {
    if (!isdir(path)) {
        return NULL;
    }

    size_t pathlen = strlen(path);
    size_t len = pathlen + sizeof("/.gitignore");
    char *git = (char *)malloc(len * sizeof(char));
    memcpy(git, path, pathlen);

    // Check if this directory is managed by git
    memcpy(git + pathlen, "/.git", sizeof("/.git"));
    if (!isdir(git)) {
        free(git);
        return NULL;
    }

    // Check if there is a .gitignore file
    memcpy(git + pathlen, "/.gitignore", sizeof("/.gitignore"));
    if (!isfile(git)) {
        free(git);
        return NULL;
    }

    struct pattern_list *pl = pattern_list_new(path);
    if (add_patterns_from_file_to_list(git, "", 0, pl, NULL) != 0) {
        pattern_list_free(pl);
        free(git);
        return NULL;
    }

    free(git);
    return pl;
}

void gitignore_free(gitignore *g) { pattern_list_free(g); }

const char *relpath(const char *path, const char *start) {
    assert(path != NULL);
    if (start == NULL) {
        start = ".";
    }

    while (*path == *start) {
        ++path;
        ++start;
    }

    while (*path == '/') {
        ++path;
    }

    return path;
}

bool gitignore_is_ignored(gitignore *g, char *path, size_t pathlen, int dtype) {
    assert(g != NULL);

    char *base = basename(path);

    enum pattern_match_result res = UNDECIDED;

    if (gitignore_global != NULL) {
        res = path_matches_pattern_list(path, pathlen, base, &dtype,
                                        gitignore_global, NULL);
    }

    if (res == UNDECIDED) {
        const char *rel = relpath(path, g->src);
        size_t rellen = pathlen - (rel - path);
        res = path_matches_pattern_list(rel, rellen, base, &dtype, g, NULL);
    }

    return res != UNDECIDED;
}
