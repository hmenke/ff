#pragma once

// C standard library
#include <stdlib.h>

#define QUEUE_PRIORITY_MAX ((size_t)-1)
#define QUEUE_PRIORITY_MIN ((size_t)0)

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
void queue_put_head(queue *q, message *msg);
void queue_put_tail(queue *q, message *msg);
message *queue_get(queue *q);
