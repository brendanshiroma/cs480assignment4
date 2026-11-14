#ifndef CONSUMER_H
#define CONSUMER_H

/* Station 2: paint robot thread (AquaCoat / PrimeSpray).
 * Consumes welded bodies and produces painted bodies.
 * arg is a ThreadArgs* whose id is the PaintRobotType. */
void *body_consumer(void *arg);

#endif
