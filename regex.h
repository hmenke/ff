#pragma once

// C standard library
#include <stdbool.h>

typedef struct _regex regex;
typedef struct _regex_storage regex_storage;

regex *regex_compile(const char *pattern, bool icase);
bool regex_match(regex *re, regex_storage *mem, const char *str, int len);
void regex_free(regex *re);

regex_storage *regex_storage_new(regex *re);
void regex_storage_free(regex_storage *mem);
