#pragma once

// C standard library
#include <stdbool.h>
#include <stddef.h>

typedef struct pattern_list gitignore;

void gitignore_init_global();
void gitignore_free_global();
gitignore *gitignore_new(const char *path);
void gitignore_free(gitignore *g);
bool gitignore_is_ignored(gitignore *g, char *path, size_t pathlen, int dtype);
