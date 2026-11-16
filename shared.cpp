#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <errno.h>

#include "shared.h"

SharedData *shared_create(unsigned int productionLimit) {
  SharedData *s = (SharedData *) calloc(1, sizeof(SharedData));
  if (s == NULL)
    return NULL;

  s->productionLimit = productionLimit;

  /* ---- body conveyor ---- */
  BodyConveyor *bc = &s->bodyConveyor;
  bc->capacity = BODY_CONVEYOR_CAPACITY;
  bc->buffer = (BodyType *) calloc(bc->capacity, sizeof(BodyType));
  bc->head = bc->tail = bc->count = 0;
  bc->typeLimit[Sedan] = MAX_SEDAN_ON_CONVEYOR;
  bc->typeLimit[SUV]   = MAX_SUV_ON_CONVEYOR;
  for (int t = 0; t < BodyTypeN; t++) {
    bc->inQueue[t]  = 0;
    bc->produced[t] = 0;
    pthread_cond_init(&bc->roomFor[t], NULL);
  }
  bc->productionLimit = productionLimit;
  bc->reserved = 0;
  bc->taken = 0;
  for (int r = 0; r < PaintRobotTypeN; r++)
    bc->consumed[r] = (unsigned int *) calloc(BodyTypeN, sizeof(unsigned int));
  pthread_mutex_init(&bc->mutex, NULL);
  pthread_cond_init(&bc->notEmpty, NULL);

  /* ---- paint conveyor ---- */
  PaintConveyor *pc = &s->paintConveyor;
  pc->capacity = PAINT_CONVEYOR_CAPACITY;
  pc->buffer = (char (*)[PAINTED_NAME_LEN]) calloc(pc->capacity, sizeof(*pc->buffer));
  pc->head = pc->tail = pc->count = 0;
  pc->finished = 0;
  pthread_mutex_init(&pc->mutex, NULL);
  pthread_cond_init(&pc->notFull, NULL);
  pthread_cond_init(&pc->notEmpty, NULL);

  /* ---- barrier for main thread (precedence constraint) ---- */
  sem_init(&s->barrier, 0, 0);

  return s;
}

void shared_destroy(SharedData *s) {
  if (s == NULL)
    return;

  BodyConveyor *bc = &s->bodyConveyor;
  for (int t = 0; t < BodyTypeN; t++)
    pthread_cond_destroy(&bc->roomFor[t]);
  pthread_cond_destroy(&bc->notEmpty);
  pthread_mutex_destroy(&bc->mutex);
  for (int r = 0; r < PaintRobotTypeN; r++)
    free(bc->consumed[r]);
  free(bc->buffer);

  PaintConveyor *pc = &s->paintConveyor;
  pthread_cond_destroy(&pc->notFull);
  pthread_cond_destroy(&pc->notEmpty);
  pthread_mutex_destroy(&pc->mutex);
  free(pc->buffer);

  sem_destroy(&s->barrier);
  free(s);
}

void sleep_ms(unsigned int ms) {
  if (ms == 0)
    return;
  struct timespec req;
  req.tv_sec  = ms / 1000;
  req.tv_nsec = (long)(ms % 1000) * 1000000L;
  /* resume the sleep if interrupted by a signal */
  while (nanosleep(&req, &req) == -1 && errno == EINTR)
    ;
}
