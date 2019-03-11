#pragma once

// C standard library
#include <stdlib.h>

typedef struct _message message;
typedef struct _queue queue;

message *message_new(int depth, size_t len, const char *path);
int message_depth(message *msg);
size_t message_len(message *msg);
char *message_path(message *msg);
void message_free(message *msg);

queue *queue_new();
void queue_free(queue *q);
void queue_put(queue *q, message *msg, size_t priority);
message *queue_get(queue *q);
