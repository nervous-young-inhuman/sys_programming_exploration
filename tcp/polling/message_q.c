#include "message_queue.h"

#include <stdlib.h>
#include <string.h>

MessageQueue *message_queue__init(size_t capacity) {
    if (capacity == 0)
        return NULL;

    MessageQueue *mq = malloc(sizeof(*mq));
    if (!mq)
        return NULL;

    sds *messages = malloc(sizeof(sds) * capacity);
    if (!messages) {
        free(mq);
        return NULL;
    }

    memset(messages, 0, sizeof(sds) * capacity);

    mq->messages = messages;
    mq->capacity = capacity;
    mq->start    = MQ_EMPTY_SENTINEL;
    mq->end      = 0;

    return mq;
}

int message_queue__q(MessageQueue *mq, const char *msg, size_t msglen) {
    if (!mq || !msg)
        return ERR_NULL_POINTER;

    if (mq->end == mq->start)
        return ERR_MQ_FULL;

    if (msglen > MAX_MESSAGE_LENGTH)
        return ERR_MESSAGE_TOO_LONG;

    sds message = sdsnewlen(msg, msglen);
    if (!message)
        return ERR_ALLOC_FAIL;

    mq->messages[mq->end] = message;
    mq->start = mq->start == MQ_EMPTY_SENTINEL ? 0 : mq->start;
    mq->end = (mq->end + 1) % (ssize_t)mq->capacity;

    return 0;
}

// the caller is supposed to free/modify this memory by calling message_string__free
MessageString message_queue__dq(MessageQueue *mq) {
    if (!mq || mq->start == MQ_EMPTY_SENTINEL)
        return NULL;

    sds message = mq->messages[mq->start];
    sds rvm = sdsdup(message);
    if (!rvm)
        return NULL;  // message stays in queue

    sdsfree(message);
    mq->messages[mq->start] = NULL;
    mq->start = (mq->start + 1) % (ssize_t)mq->capacity;

    if (mq->start == mq->end)
        mq->start = MQ_EMPTY_SENTINEL;

    return rvm;
}

void message_queue__free(MessageQueue *mq) {
    if (!mq || !mq->messages)
        return;

    if (mq->start != MQ_EMPTY_SENTINEL)
        for (ssize_t start = mq->start; start != mq->end;
             start = (start + 1) % (ssize_t)mq->capacity) {
            sdsfree(mq->messages[start]);
        }

    free(mq->messages);
    free(mq);
}
