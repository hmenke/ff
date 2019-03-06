#pragma once

// C standard library
#include <stdlib.h>

// POSIX C library
#include <mqueue.h>

typedef struct _message message;

message *message_new(int depth, size_t len, const char *path);
int message_depth(message *msg);
size_t message_len(message *msg);
char *message_path(message *msg);
void message_free(message *msg);

typedef struct _archive archive;

size_t archive_size(archive *ar);
char *archive_data(archive *ar);
void archive_free(archive *ar);

archive *message_serialize(const message *msg);
message *message_unserialize(const char *buffer);

int message_send(mqd_t mqdes, const message *msg, unsigned int msg_prio);
message *message_receive(mqd_t mqdes, char *buffer, size_t size);
