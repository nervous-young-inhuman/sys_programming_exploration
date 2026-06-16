#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sds.h"
#include <sys/types.h>

#define MAX_MESSAGE_LENGTH 512
#define MQ_EMPTY_SENTINEL -1

#define ERR_MQ_FULL -40
#define ERR_MESSAGE_TOO_LONG -41


typedef sds MessageString;
#define message_string__len sdslen
#define message_string__free sdsfree


typedef struct message_queue {
  size_t capacity;
  sds *messages;
  ssize_t start;
  ssize_t end;
} MessageQueue;

MessageQueue* message_queue__init(size_t capacity) {
  assert(capacity > 0); // lazy programming, here to catch my call mistakes
  MessageQueue *mq = malloc(sizeof(*mq));
  if (!mq) {
    assert(mq != NULL);
  }
  sds *messages = malloc(sizeof(sds *) * capacity);
  if (!messages) {
    free(mq);
  }
  assert(messages != NULL); // crash / exit the same way
  mq->messages = messages;
  mq->capacity = capacity;
  mq->start = MQ_EMPTY_SENTINEL;
  mq->end = 0;

  memset(mq->messages, 0, sizeof(sds *) * capacity);
  return mq;
}


int message_queue__q(MessageQueue *mq, char *msg, size_t msglen) {
  assert(msg != NULL);
  if (mq->end == mq->start)
    return ERR_MQ_FULL;

  if (msglen > MAX_MESSAGE_LENGTH)
    return ERR_MESSAGE_TOO_LONG;

  sds message = sdsnewlen(msg, msglen);
  mq->messages[mq->end] = message;

  mq->start = mq->start == MQ_EMPTY_SENTINEL ? 0 : mq->start;
  mq->end = (mq->end + 1) % mq->capacity;
}

// the caller is supposed to free/modify this memory by calling message_string__free
MessageString message_queue__dq(MessageQueue *mq) {
  ssize_t current_idx = mq->start;
  assert(current_idx != MQ_EMPTY_SENTINEL);

  sds message = mq->messages[mq->start];
  sds rvm = sdsdup(message);
  assert(rvm != NULL);

  sdsfree(message);
  mq->start = (mq->start + 1) % mq->capacity;
  if (mq->start == mq->end)
    mq->start = MQ_EMPTY_SENTINEL;
  
  return rvm;
}


void message_queue__free(MessageQueue *mq) {
  if (!mq || !mq->messages)
    return;

  if (mq->start != MQ_EMPTY_SENTINEL)
    for (ssize_t start = mq->start; start != mq->end;
         start = (start + 1) % mq->capacity) {
      sds message = mq->messages[start];
      sdsfree(message);
    }

  free(mq->messages);
  free(mq);
}
