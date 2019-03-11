#ifndef __cplusplus
#define _GNU_SOURCE
#endif

#include "message.h"

// C standard library
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// POSIX C library
#include <mqueue.h>
#include <pthread.h>
#include <semaphore.h>

struct _message {
    int i;
    size_t len;
    char *str;
};

message *message_new(int i, size_t len, const char *str) {
    message *msg = (message *)malloc(sizeof(message));
    msg->i = i;
    msg->len = len;
    msg->str = str ? strdup(str) : NULL;
    return msg;
}

int message_depth(message *msg) { return msg->i; }

size_t message_len(message *msg) { return msg->len; }

char *message_path(message *msg) { return msg->str; }

void message_free(message *msg) {
    free(msg->str);
    free(msg);
    msg = NULL;
}

typedef struct _node node;
struct _node {
    size_t priority;
    message *msg;
    node *next;
};

struct _queue {
    sem_t length;
    node *head;
    node *tail;
    pthread_mutex_t lock;
};

queue *queue_new() {
    queue *q = (queue *)malloc(sizeof(queue));
    sem_init(&q->length, 0, 0);
    q->head = NULL;
    q->tail = NULL;
    pthread_mutex_init(&q->lock, NULL);
    return q;
}

void queue_free(queue *q) {
    sem_destroy(&q->length);
    pthread_mutex_destroy(&q->lock);
    free(q);
}

void queue_put(queue *q, message *msg, size_t priority) {
    node *new = (node *)malloc(sizeof(node));
    new->priority = priority;
    new->msg = msg;
    new->next = NULL;

    pthread_mutex_lock(&q->lock);
    int length;
    sem_getvalue(&q->length, &length);
    if (length == 0) {
        q->head = new;
        q->tail = new;
        goto unlock;
    }

    node *p = q->head;
    while (p != q->tail && p->priority > priority) {
        if (p->next == NULL) {
            break;
        }
        p = p->next;
    }

    node *next = p->next;
    p->next = new;
    new->next = next;

unlock:
    pthread_mutex_unlock(&q->lock);
    sem_post(&q->length);
}

message *queue_get(queue *q) {
    sem_wait(&q->length);
    pthread_mutex_lock(&q->lock);
    message *msg = q->head->msg;
    node *oldhead = q->head;
    q->head = q->head->next;
    free(oldhead);
    pthread_mutex_unlock(&q->lock);
    return msg;
}
