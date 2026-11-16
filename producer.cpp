#include "producer.h"
#include "shared.h"
#include "log.h"

/*
 * Body producer (welding robot).
 *
 * Each iteration:
 *   1. Claim one unit of the production limit (inside the monitor) so the
 *      two welders together produce exactly N bodies.
 *   2. Weld the body: sleep OUTSIDE the critical section.
 *   3. Enter the monitor, wait until there is room on the conveyor AND room
 *      for this body type, append it (FIFO), log, signal consumers.
 */
void *body_producer(void *arg) {
  ThreadArgs   *args = (ThreadArgs *) arg;
  BodyConveyor *bc   = &args->shared->bodyConveyor;
  BodyType      type = (BodyType) args->id;

  while (1) {
    /* 1. claim a production slot */
    pthread_mutex_lock(&bc->mutex);
    if (bc->reserved >= bc->productionLimit) {
      pthread_mutex_unlock(&bc->mutex);
      break;                      /* production limit reached: stop */
    }
    bc->reserved++;
    pthread_mutex_unlock(&bc->mutex);

    /* 2. simulate welding the body (outside the critical section) */
    sleep_ms(args->sleepTime);

    /* 3. put the body on the conveyor */
    pthread_mutex_lock(&bc->mutex);
    /* wait while conveyor is full OR this type is at its limit;
     * both conditions are re-checked after every wake-up */
    while (bc->count >= bc->capacity ||
           bc->inQueue[type] >= bc->typeLimit[type]) {
      pthread_cond_wait(&bc->roomFor[type], &bc->mutex);
    }

    bc->buffer[bc->tail] = type;
    bc->tail = (bc->tail + 1) % bc->capacity;
    bc->count++;
    bc->inQueue[type]++;
    bc->produced[type]++;

    /* log inside the critical section, right after the mutation */
    BodyAdded added;
    added.type     = type;
    added.produced = bc->produced;
    added.inQueue  = bc->inQueue;
    log_added_body(added);

    pthread_cond_signal(&bc->notEmpty);   /* a body is available */
    pthread_mutex_unlock(&bc->mutex);
  }

  return NULL;
}
