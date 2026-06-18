#include "message_q.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

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
    SLIST_INIT(&mq->clients);
    return mq;
}

// return < 0 on errors
int message_queue__q(MessageQueue *mq, const char *msg, size_t msglen) {
    if (!mq || !msg)
        return MQERR_NULL_POINTER;

    if (mq->end == mq->start)
        return MQERR_FULL;

    if (msglen > MAX_MESSAGE_LENGTH)
        return MQERR_MESSAGE_TOO_LONG;

    sds message = sdsnewlen(msg, msglen);
    if (!message)
        return MQERR_ALLOC_FAIL;

    mq->messages[mq->end] = message;
    mq->start = mq->start == MQ_EMPTY_SENTINEL ? 0 : mq->start;
    mq->end = (mq->end + 1) % (ssize_t)mq->capacity;

    return 0;
}

static size_t get_length(MessageQueue *mq) {
  if (mq->start == MQ_EMPTY_SENTINEL)
    return 0;
  if (mq->start < mq->end)
    return (mq->end - mq->start);
  else if (mq->start == mq->end)
    return mq->capacity;
  else
    return (mq->capacity - mq->start) + (mq->end);
}

// the caller is 
MessageString message_queue__get(MessageQueue *mq, size_t index) {
  if (!mq || (mq->start == MQ_EMPTY_SENTINEL))
    return NULL;

  // is out of bounds
  if (index >= get_length(mq))
    return NULL;

  MessageString s = mq->messages[(mq->start + index) % mq->capacity];
  sds rvm = sdsdup(s);
  if (!rvm)
    return NULL;
  
  return rvm;
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
      for (ssize_t start = mq->start; start != mq->end; start = (start + 1) % (ssize_t)mq->capacity) {
        sdsfree(mq->messages[start]);
      }

    free(mq->messages);
    free(mq);
}

int message_queue__register_client(MessageQueue *mq, int clientfd) {
    if (!mq) return -1;
    MQClient *client = malloc(sizeof(MQClient));
    if (!client) return -1;
    client->fd = clientfd;
    client->roff = mq->start == MQ_EMPTY_SENTINEL ? 0 : mq->start;
    SLIST_INSERT_HEAD(&mq->clients, client, next);
    return 0;
}

int message_queue__unregister_client(MessageQueue *mq, int clientfd) {
    if (!mq) return -1;
    MQClient *client = NULL;
    SLIST_FOREACH(client, &mq->clients, next) {
        if (client->fd == clientfd) {
            SLIST_REMOVE(&mq->clients, client, message_queue__client, next);
            free(client);
            return 0;
        }
    }
    return -1;
}

static int __flush_client_messages(MessageQueue *mq, MQClient *client) {
    ssize_t i = client->roff;
    while (i != mq->end) {
        if (mq->messages[i]) {
            ssize_t n = write(client->fd, mq->messages[i], sdslen(mq->messages[i]));
            if (n < 0) {
                client->roff = i;
                return -1;
            }
        }
        i = (i + 1) % (ssize_t)mq->capacity;
    }
    client->roff = mq->end;
    return 0;
}

int message_queue__flush(MessageQueue *mq, int clientfd) {
    if (!mq) return MQERR_NULL_POINTER;

    if (get_length(mq) == 0) {
        return 0;
    }

    MQClient *client = NULL;
    SLIST_FOREACH(client, &mq->clients, next) {
        if (client->fd == clientfd) {
            return __flush_client_messages(mq, client);
        }
    }
    return -1;
}

void message_queue__drop_slow_consumers(MessageQueue *mq) {
    if (!mq || mq->start == MQ_EMPTY_SENTINEL)
        return;

    size_t length = get_length(mq);

    if (length < (mq->capacity * 8) / 10)
        return;

    /* First 40% of the ring from start */
    size_t drop_before_offset = (size_t)(mq->capacity * 0.4);

    MQClient *client = SLIST_FIRST(&mq->clients);
    while (client) {
        MQClient *next = SLIST_NEXT(client, next);

        size_t client_off =
            (mq->capacity + client->roff - mq->start) % mq->capacity;

        if (client_off < drop_before_offset) {
            SLIST_REMOVE(&mq->clients,
                         client,
                         message_queue__client,
                         next);
            free(client);
        }

        client = next;
    }
}
