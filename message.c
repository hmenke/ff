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

// opaque message and archive types

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

struct _archive {
    size_t size;
    char *data;
};

static archive *archive_new(size_t size, const char *data) {
    archive *ar = (archive *)malloc(sizeof(archive));
    ar->data = NULL;
    ar->size = size;
    if (size > 0) {
        memcpy(ar->data, data, size);
    }
    return ar;
}

size_t archive_size(archive *ar) { return ar->size; }

char *archive_data(archive *ar) { return ar->data; }

void archive_free(archive *ar) {
    free(ar->data);
    free(ar);
    ar = NULL;
}

// serialization functions

archive *message_serialize(const message *msg) {
    archive *ar = archive_new(0, NULL);

    size_t len = msg->len + 1;
    ar->size = sizeof(int) + sizeof(size_t) + len * sizeof(char);

    ar->data = (char *)malloc(ar->size);
    size_t offset = 0;

    memcpy(ar->data + offset, &msg->i, sizeof(int));
    offset += sizeof(int);

    memcpy(ar->data + offset, &msg->len, sizeof(size_t));
    offset += sizeof(size_t);

    memcpy(ar->data + offset, msg->str, len * sizeof(char));
    //offset += len * sizeof(char);

    return ar;
}

message *message_unserialize(const char *buffer) {
    message *msg = message_new(0, 0, NULL);

    size_t offset = 0;

    memcpy(&msg->i, buffer + offset, sizeof(int));
    offset += sizeof(int);

    memcpy(&msg->len, buffer + offset, sizeof(size_t));
    offset += sizeof(size_t);

    size_t len = msg->len + 1;
    msg->str = (char *)malloc(len * sizeof(char));
    memcpy(msg->str, buffer + offset, len * sizeof(char));
    //offset += len * sizeof(char);

    return msg;
}

// Interaction with mqueue

int message_send(mqd_t mqdes, const message *msg, unsigned int msg_prio) {
    archive *ar = message_serialize(msg);
    int ret = mq_send(mqdes, archive_data(ar), archive_size(ar), msg_prio);
    archive_free(ar);
    return ret;
}

message *message_receive(mqd_t mqdes, char *buffer, size_t size) {
    switch (mq_receive(mqdes, buffer, size, NULL)) {
    case 0:
        return NULL;
        break;
    case -1:
        perror("mq_receive");
        return NULL;
        break;
    default:
        return message_unserialize(buffer);
        break;
    }
}
