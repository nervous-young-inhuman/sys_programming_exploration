#ifndef MESSAGE_QUEUE_H
#define MESSAGE_QUEUE_H

#include <stddef.h>
#include <sys/types.h>
#include "sds.h"

#define MAX_MESSAGE_LENGTH   512
#define MQ_EMPTY_SENTINEL    (-1)

/* error codes */
#define ERR_MQ_FULL          (-40)
#define ERR_MESSAGE_TOO_LONG (-41)
#define ERR_NULL_POINTER     (-42)
#define ERR_ALLOC_FAIL       (-43)
#define ERR_MQ_EMPTY         (-44)

typedef sds MessageString;
#define message_string__len  sdslen
#define message_string__free sdsfree

typedef struct message_queue {
    size_t   capacity;
    sds     *messages;
    ssize_t  start;  /* MQ_EMPTY_SENTINEL when empty */
    ssize_t  end;
} MessageQueue;

MessageQueue  *message_queue__init(size_t capacity);
int message_queue__q(MessageQueue *mq, const char *msg, size_t msglen);
// the caller is supposed to free/modify this memory by calling message_string__free
MessageString  message_queue__dq(MessageQueue *mq);
void           message_queue__free(MessageQueue *mq);

#endif /* MESSAGE_QUEUE_H */
