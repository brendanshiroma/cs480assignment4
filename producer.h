#ifndef PRODUCER_H
#define PRODUCER_H

/* Station 1: welding robot thread (SedanWelder / SUVWelder).
 * arg is a ThreadArgs* whose id is the BodyType it produces. */
void *body_producer(void *arg);

#endif
