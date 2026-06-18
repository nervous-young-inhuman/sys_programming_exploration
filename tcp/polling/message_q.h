#ifndef MESSAGE_QUEUE_H
#define MESSAGE_QUEUE_H

#include <stddef.h>
#include <stdio.h>
#include <sys/types.h>
#include "sds.h"

#define MAX_MESSAGE_LENGTH   512
#define MQ_EMPTY_SENTINEL    (-1)

/* error codes, (should be < 0) */
#define MQERR_FULL             (-40)
#define MQERR_MESSAGE_TOO_LONG (-41)
#define MQERR_NULL_POINTER     (-42)
#define MQERR_ALLOC_FAIL       (-43)
#define MQERR_EMPTY            (-44)

typedef sds MessageString;
#define message_string__len  sdslen
#define message_string__free sdsfree

typedef struct message_queue__client {
  int fd; // for now fd == unique id, its a horrible representation i know
  ssize_t roff; // read offset
} MQClient;


typedef struct message_queue {
  size_t   capacity;
  sds     *messages;
  ssize_t  start;  /* MQ_EMPTY_SENTINEL when empty */
  ssize_t end;
} MessageQueue;



MessageQueue  *message_queue__init(size_t capacity);
int message_queue__q(MessageQueue *mq, const char *msg, size_t msglen);
// the caller is supposed to free/modify this memory by calling message_string__free
MessageString message_queue__dq(MessageQueue *mq);
MessageString message_queue__get(MessageQueue *mq, size_t index);
void           message_queue__free(MessageQueue *mq);

#endif /* MESSAGE_QUEUE_H */
