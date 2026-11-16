#include <stdio.h>

#include "finalline.h"
#include "shared.h"
#include "log.h"

/*
 * Painted body consumer (FinalLine robot).
 *
 * Each iteration:
 *   1. Enter the paint conveyor monitor, wait for a painted body, remove it
 *      in FIFO order, log, signal the paint robots.
 *   2. Install trim / inspect: sleep OUTSIDE the critical section.
 * After the last car (finished == production limit and conveyor empty),
 * the thread returns; main() joins this thread to know when it's done.
 */
void *paintedbody_consumer(void *arg) {
  ThreadArgs    *args   = (ThreadArgs *) arg;
  SharedData    *shared = args->shared;
  PaintConveyor *pc     = &shared->paintConveyor;
  int            done   = (shared->productionLimit == 0);

  while (!done) {
    char painted[PAINTED_NAME_LEN];

    /* 1. take a painted body off the paint conveyor */
    pthread_mutex_lock(&pc->mutex);
    while (pc->count == 0) {
      pthread_cond_wait(&pc->notEmpty, &pc->mutex);
    }

    snprintf(painted, sizeof(painted), "%s", pc->buffer[pc->head]);
    pc->head = (pc->head + 1) % pc->capacity;
    pc->count--;
    pc->finished++;

    log_removed_paintedbody(painted, pc->count, pc->finished);

    pthread_cond_signal(&pc->notFull);   /* a paint robot may proceed */
    pthread_mutex_unlock(&pc->mutex);

    /* 2. simulate trim install + inspection (outside critical section) */
    sleep_ms(args->sleepTime);

    /* last car rolled off the line? */
    pthread_mutex_lock(&pc->mutex);
    done = (pc->finished >= shared->productionLimit && pc->count == 0);
    pthread_mutex_unlock(&pc->mutex);
  }

  return NULL;
}
