#ifndef __cplusplus
#define _GNU_SOURCE
#endif

#include "gitignore.h"
#include "macros.h"

// C standard library
#include <assert.h>
#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// POSIX C library
#include <fnmatch.h>
#include <libgen.h>
#include <sys/stat.h>

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

const char *relpath(const char *path, const char *start) {
    assert(path != NULL);
    if (start == NULL) {
        start = ".";
    }

    while (*path == *start) {
        ++path;
        ++start;
    }

    return path;
}

bool isempty(const char *str) {
    char c;
    while ((c = *str++) != '\0') {
        if (!isspace(c)) {
            return false;
        }
    }
    return true;
}

// glob

typedef struct _glob glob;

enum glob_flags { NONE = 1 << 0, WHITELISTED = 1 << 1, ONLY_DIR = 1 << 2 };

struct _glob {
    char *pattern;
    enum glob_flags flags;
};

glob *glob_new(const char *str) {
    // Skip all leading whitespace characters
    for (char c = *str; isspace(c); c = *++str)
        ;

    // A blank line matches no files, so it can serve as a separator for
    // readability.
    if (*str == '\0') {
        return NULL;
    }

    // A line starting with # serves as a comment. Put a backslash ("\")
    // in front of the first hash for patterns that begin with a hash.
    if (*str == '#') {
        return NULL;
    }
    if (*str == '\\' && *(str + 1) == '#') {
        ++str;
    }

    // All failures have been treated so we can allocate now,
    glob *g = (glob *)malloc(sizeof(glob));
    g->pattern = NULL;
    g->flags = NONE;

    // An optional prefix "!" which negates the pattern; any matching file
    // excluded by a previous pattern will become included again. It is
    // not possible to re-include a file if a parent directory of that
    // file is excluded. Git doesn’t list excluded directories for
    // performance reasons, so any patterns on contained files have no
    // effect, no matter where they are defined. Put a backslash ("\") in
    // front of the first "!" for patterns that begin with a literal "!",
    // for example, "\!important!.txt".
    if (*str == '!') {
        g->flags |= WHITELISTED;
        ++str;
    }
    if (*str == '\\' && *(str + 1) == '!') {
        ++str;
    }

    size_t len = strlen(str);
    const char *end = str + len - 1;

    // Trailing spaces are ignored unless they are quoted with backslash
    // ("\").
    while (end > str && *end == ' ') {
        --len;
        --end;
    }

    // If the pattern ends with a slash, it is removed for the purpose of
    // the following description, but it would only find a match with a
    // directory. In other words, foo/ will match a directory foo and
    // paths underneath it, but will not match a regular file or a
    // symbolic link foo (this is consistent with the way how pathspec
    // works in general in Git).
    while (end > str && *end == '/') {
        --len;
        --end;
        g->flags |= ONLY_DIR;
    }

    // Manually work around that strncpy bullshit
    g->pattern = (char *)malloc((len + 1) * sizeof(char));
    strncpy(g->pattern, str, len);
    g->pattern[len] = '\0';

    return g;
}

bool glob_match(glob *g, const char *path, bool isdir) {
    if ((g->flags & ONLY_DIR) && !isdir) {
        return false;
    }

    // If the pattern does not contain a slash /, Git treats it as a shell
    // glob pattern and checks for a match against the pathname relative
    // to the location of the .gitignore file (relative to the toplevel of
    // the work tree if not from a .gitignore file).

    // Otherwise, Git treats the pattern as a shell glob: "*" matches
    // anything except "/", "?" matches any one character except "/" and
    // "[]" matches one character in a selected range. See fnmatch(3) and
    // the FNM_PATHNAME flag for a more detailed description.
    // A leading slash matches the beginning of the pathname. For example,
    // "/*.c" matches "cat-file.c" but not "mozilla-sha1/sha1.c".

    int flags = FNM_EXTMATCH;
    if (strchr(g->pattern, '/') != NULL) {
        flags |= FNM_PATHNAME;
    }

    bool match = false;
    if (fnmatch(g->pattern, path, flags) == 0) {
        match = true;
    }

    return match;
}

void glob_free(glob *g) {
    if (g == NULL) {
        return;
    }
    free(g->pattern);
    free(g);
    g = NULL;
}

// Linked list of globs

#define foreach_glob(gl, globlist)                                             \
    for (glob *gl = NULL, *_gn = globlist ? (glob *)(globlist->head) : NULL;   \
         ((globnode *)_gn) != NULL && ((globnode *)_gn) != globlist->tail &&   \
         (gl = ((globnode *)_gn)->g);                                          \
         _gn = (glob *)((globnode *)_gn)->next)

typedef struct _globnode globnode;
struct _globnode {
    glob *g;
    globnode *next;
};

typedef struct _globlist globlist;
struct _globlist {
    globnode *head;
    globnode *tail;
    size_t n;
};

globlist *globlist_new() {
    globlist *gl = (globlist *)malloc(sizeof(globlist));
    gl->head = NULL;
    gl->tail = NULL;
    return gl;
}

void globlist_append(globlist *gl, glob *g) {
    globnode *gn = (globnode *)malloc(sizeof(globnode));
    gn->g = g;
    gn->next = NULL;

    if (gl->head == NULL && gl->tail == NULL) {
        gl->head = gn;
        gl->tail = gn;
        return;
    }

    gl->tail->next = gn;
    gl->tail = gn;

    ++gl->n;
}

void globlist_free(globlist *gl) {
    if (gl == NULL) {
        return;
    }
    globnode *gn = gl->head;
    while (gn != NULL) {
        globnode *next = gn->next;
        glob_free(gn->g);
        free(gn);
        gn = next;
    }
    free(gl);
    gl = NULL;
}

// gitignore

struct _gitignore {
    char *path;
    globlist *local;
};

static globlist *gitignore_global = NULL;

void gitignore_init_global() {
    // Check for and parse the global .gitignore file
    //
    // TODO: Check ~/.gitconfig for core.excludesfile
    const char *home = NULL;
    char *ignorehome = NULL;
    if ((home = getenv("XDG_CONFIG_HOME")) != NULL) {
        size_t homelen = strlen(home);
        ignorehome = (char *)malloc((strlen(home) + sizeof("/git/ignore")) *
                                    sizeof(char));
        strncpy(ignorehome, home, homelen);
        strncpy(ignorehome + homelen, "/git/ignore", sizeof("/git/ignore"));
    } else if ((home = getenv("HOME")) != NULL) {
        size_t homelen = strlen(home);
        ignorehome = (char *)malloc(
            (strlen(home) + sizeof("/.config/git/ignore")) * sizeof(char));
        strncpy(ignorehome, home, homelen);
        strncpy(ignorehome + homelen, "/.config/git/ignore",
                sizeof("/.config/git/ignore"));
    }

    if (ignorehome == NULL || !isfile(ignorehome)) {
        free(ignorehome);
        gitignore_global = NULL;
        return;
    }

    with_file(f, ignorehome, "r") {
        char *line = NULL;
        size_t n = 0;
        ssize_t len = 0;
        while ((len = getline(&line, &n, f)) != -1) {
            // Discard the newline char
            line[len - 1] = '\0';

            glob *gl = glob_new(line);
            if (gl) {
                globlist_append(gitignore_global, gl);
            }
        }
        free(line);
    }

    free(ignorehome);
}

void gitignore_free_global() {
    if (gitignore_global == NULL) {
        return;
    }
    globlist_free(gitignore_global);
    gitignore_global = NULL;
}

gitignore *gitignore_new(const char *path) {
    if (!isdir(path)) {
        return NULL;
    }

    size_t pathlen = strlen(path);
    size_t len = pathlen + sizeof("/.gitignore");
    char *file = (char *)malloc(len * sizeof(char));
    strncpy(file, path, pathlen);
    strncpy(file + pathlen, "/.gitignore", sizeof("/.gitignore"));
    if (!isfile(file)) {
        free(file);
        return NULL;
    }

    gitignore *g = (gitignore *)malloc(sizeof(gitignore));
    g->path = strdup(path);
    g->local = globlist_new();

    // Parse the local .gitignore file
    with_file(f, file, "r") {
        char *line = NULL;
        size_t n = 0;
        ssize_t len = 0;
        while ((len = getline(&line, &n, f)) != -1) {
            // Discard the newline char
            line[len - 1] = '\0';

            glob *gl = glob_new(line);
            if (gl) {
                globlist_append(g->local, gl);
            }
        }
        free(line);
    }

    free(file);
    return g;
}

void gitignore_free(gitignore *g) {
    if (g == NULL) {
        return;
    }
    globlist_free(g->local);
    free(g->path);
    free(g);
    g = NULL;
}

bool gitignore_is_ignored(gitignore *g, const char *path, bool is_dir) {
    const char *rel = relpath(path, g->path);

    bool is_ignored = false;
    foreach_glob(gl, g->local) {
        if (glob_match(gl, rel, is_dir)) {
            is_ignored = true;
            if (gl->flags & WHITELISTED) {
                is_ignored = false;
            }
        }
    }

    foreach_glob(gl, gitignore_global) {
        if (glob_match(gl, path, is_dir)) {
            is_ignored = true;
            if (gl->flags & WHITELISTED) {
                is_ignored = false;
            }
        }
    }

    return is_ignored;
}
