#pragma once

// C standard library
#include <stdbool.h>

typedef struct _gitignore gitignore;

gitignore *gitignore_new(const char *path);
void gitignore_free(gitignore *g);
bool gitignore_is_ignored(gitignore *g, const char *path, bool is_dir);
