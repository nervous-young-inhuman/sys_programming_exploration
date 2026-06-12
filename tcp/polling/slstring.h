// not a generic library!!! meant only for use in poll.c
// users be warned!!!!!
#include <sys/types.h>
#define SLSTRING_MIN_CAPACITY 16

typedef unsigned char SlStringLen;
typedef struct {
  size_t capacity;
  SlStringLen len;
  char* data;
} SlString;



// will return NULL when can't malloc, so check for that
extern SlString* slstring__new(SlStringLen capacity);
// will return the new capacity and -1 on error
extern ssize_t slstring__reqappend(SlString *s, SlStringLen maxaddlen);

// negative values are errors
// -1 = written_len value passed is greater than available capacity.
// -2 = written_len value = capacity and when that's true, the last byte should equal '\0' byte
extern int slstring__complete_append(SlString *s, SlStringLen written_len);
extern void slstring__destroy(SlString *s);





