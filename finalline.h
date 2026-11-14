#ifndef FINALLINE_H
#define FINALLINE_H

/* Station 3: FinalLine robot thread, consumer of painted bodies.
 * arg is a ThreadArgs*. Posts the shared barrier after the last car. */
void *paintedbody_consumer(void *arg);

#endif
