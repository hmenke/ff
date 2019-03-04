#ifndef __cplusplus
#define _GNU_SOURCE
#endif

#include "serialize.h"

// C standard library
#include <stdlib.h>
#include <string.h>

struct _message {
    int id;
    char *text;
};

message *message_new(int id, const char *text) {
    message *msg = (message *)malloc(sizeof(message));
    msg->id = id;
    msg->text = text ? strdup(text) : NULL;
    return msg;
}

int message_id(message *msg) { return msg->id; }

char *message_text(message *msg) { return msg->text; }

void message_free(message *msg) {
    free(msg->text);
    free(msg);
}

struct _archive {
    size_t size;
    char *data;
};

archive *archive_new(size_t size, const char *data) {
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
}

archive *message_serialize(const message *msg) {
    archive *ar = archive_new(0, NULL);

    size_t len = strlen(msg->text) + 1;
    ar->size = sizeof(int) + len * sizeof(char);

    ar->data = (char *)malloc(ar->size);
    size_t offset = 0;

    memcpy(ar->data + offset, &msg->id, sizeof(int));
    offset += sizeof(int);

    memcpy(ar->data + offset, msg->text, len * sizeof(char));

    return ar;
}

message *message_unserialize(const char *buffer) {
    message *msg = message_new(0, NULL);

    size_t offset = 0;

    memcpy(&msg->id, buffer + offset, sizeof(int));
    offset += sizeof(int);

    // I hope you serialized that null terminator
    msg->text = strdup(buffer + offset);

    return msg;
}
