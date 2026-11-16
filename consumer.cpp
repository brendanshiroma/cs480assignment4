#include <stdio.h>

#include "consumer.h"
#include "shared.h"
#include "log.h"

/*
 * Body consumer / painted body producer (paint robot).
 *
 * Each iteration:
 *   1. Enter the body conveyor monitor, wait for a body (or for the end of
 *      production), remove it in FIFO order, log, signal the welders.
 *   2. Paint the body: sleep OUTSIDE any critical section.
 *   3. Enter the paint conveyor monitor, wait for a free spot, append the
 *      painted body (FIFO), log, signal FinalLine.
 */
void *body_consumer(void *arg) {
  ThreadArgs     *args  = (ThreadArgs *) arg;
  BodyConveyor   *bc    = &args->shared->bodyConveyor;
  PaintConveyor  *pc    = &args->shared->paintConveyor;
  PaintRobotType  robot = (PaintRobotType) args->id;

  while (1) {
    /* 1. take a welded body off the body conveyor */
    pthread_mutex_lock(&bc->mutex);
    while (bc->count == 0 && bc->taken < bc->productionLimit) {
      pthread_cond_wait(&bc->notEmpty, &bc->mutex);
    }
    if (bc->count == 0) {
      /* every body has been taken: nothing left to paint */
      pthread_mutex_unlock(&bc->mutex);
      break;
    }

    BodyType type = bc->buffer[bc->head];
    bc->head = (bc->head + 1) % bc->capacity;
    bc->count--;
    bc->inQueue[type]--;
    bc->taken++;
    bc->consumed[robot][type]++;

    BodyRemoved removed;
    removed.consumer = robot;
    removed.type     = type;
    removed.consumed = bc->consumed[robot];
    removed.inQueue  = bc->inQueue;
    log_removed_body(removed);

    /* Removing a body frees a conveyor slot (both welders may proceed)
     * and a slot for this body type. Wake both welders; each re-checks
     * its own conditions. */
    for (int t = 0; t < BodyTypeN; t++)
      pthread_cond_signal(&bc->roomFor[t]);

    /* If that was the last body, wake the other paint robot so it exits */
    if (bc->taken >= bc->productionLimit)
      pthread_cond_broadcast(&bc->notEmpty);
    pthread_mutex_unlock(&bc->mutex);

    /* 2. simulate painting (consuming body / producing painted body) */
    sleep_ms(args->sleepMs);

    char painted[PAINTED_NAME_LEN];
    snprintf(painted, sizeof(painted), "%s%s",
             body_consumerNames[robot], body_producerNames[type]);

    /* 3. put the painted body on the paint conveyor */
    pthread_mutex_lock(&pc->mutex);
    while (pc->count >= pc->capacity) {
      pthread_cond_wait(&pc->notFull, &pc->mutex);
    }

    snprintf(pc->buffer[pc->tail], PAINTED_NAME_LEN, "%s", painted);
    pc->tail = (pc->tail + 1) % pc->capacity;
    pc->count++;

    log_added_paintedbody(painted, pc->count);

    pthread_cond_signal(&pc->notEmpty);   /* FinalLine may proceed */
    pthread_mutex_unlock(&pc->mutex);
  }

  return NULL;
}
