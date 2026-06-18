#include "message_q.h"

#include <stdio.h>
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
    mq->clients  = clients;
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
