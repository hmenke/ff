#pragma once

// C standard library
#include <stdbool.h>

typedef struct _gitignore gitignore;

void gitignore_init_global();
void gitignore_free_global();
gitignore *gitignore_new(const char *path);
void gitignore_free(gitignore *g);
bool gitignore_is_ignored(gitignore *g, const char *path, bool is_dir);
