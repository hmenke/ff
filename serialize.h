#pragma once

#include <stdlib.h>

typedef struct _message message;

message *message_new(int id, const char *text);
int message_id(message *msg);
char *message_text(message *msg);
void message_free(message *msg);

typedef struct _archive archive;

archive *archive_new(size_t size, const char *data);
size_t archive_size(archive *ar);
char *archive_data(archive *ar);
void archive_free(archive *ar);

archive *message_serialize(const message *msg);
message *message_unserialize(const char *buffer);
