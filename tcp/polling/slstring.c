#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "slstring.h"

/* SlString slstring__from(char *src, SlStringLen len) { */
/*   char *dst = malloc(len); */
/*   memcpy(dst, src, len); */
  
/*   return (SlString){.len=len, .data=dst, .capacity=len}; */
/* } */


SlString* slstring__new(SlStringLen capacity) {
	char *dst = malloc(capacity < 1 ? capacity : SLSTRING_MIN_CAPACITY);
	if (!dst)
		return NULL;

	SlString *nn = malloc(sizeof(*nn));
	if (!nn) {
		free(dst);
		return NULL;
	}

	memset(dst, '\0', capacity);
	nn->len = 0;
	nn->data = dst;
	nn->capacity = capacity;

	return nn;
}

ssize_t slstring__reqappend(SlString *s, SlStringLen maxaddlen) {
	// will return the new capacity
	const size_t remaining = s->capacity - s->len;
	if (remaining > maxaddlen)
		return s->capacity;

	// +1 for the NUL/delimiter byte
	int pp_capacity = s->capacity + maxaddlen + 1;
	char *pp = malloc(pp_capacity);
	if (!pp) {
		return -1;
	}
	memcpy(pp, s->data, s->len);
	free(s->data);

	memset(pp + s->len, '\0', remaining);
	s->data = pp;
	s->capacity = pp_capacity;
	return pp_capacity;
}

int slstring__complete_append(SlString *s, SlStringLen written_len) {
	const size_t remaining = s->capacity - s->len;
	if (written_len > remaining)
		return -1; // written value must be lesser than the available capacity

	if ((written_len == remaining) && (s->data[s->capacity - 1] != '\0'))
		return -2; // if written value == pending capacity then it must end with
	// '\0' delimiter

	s->len += written_len;
	s->data[s->len - 1] = '\0';
	return 0;
}

void slstring__destroy(SlString *s) {
	if (s) {
		if (s->data)
			free(s->data);
		free(s);
	}
}
