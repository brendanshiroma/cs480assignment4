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
  BodyConveyor *b = &s->bodyConveyor;
  b->capacity = BODY_CONVEYOR_CAPACITY;
  b->buffer = (BodyType *) calloc(b->capacity, sizeof(BodyType));
  b->head = b->tail = b->count = 0;
  b->typeLimit[Sedan] = MAX_SEDAN_ON_CONVEYOR;
  b->typeLimit[SUV]   = MAX_SUV_ON_CONVEYOR;
  for (int t = 0; t < BodyTypeN; t++) {
    b->inQueue[t]  = 0;
    b->produced[t] = 0;
    pthread_cond_init(&b->roomFor[t], NULL);
  }
  b->productionLimit = productionLimit;
  b->reserved = 0;
  b->taken = 0;
  for (int r = 0; r < PaintRobotTypeN; r++)
    b->consumed[r] = (unsigned int *) calloc(BodyTypeN, sizeof(unsigned int));
  pthread_mutex_init(&b->mutex, NULL);
  pthread_cond_init(&b->notEmpty, NULL);

  /* ---- paint conveyor ---- */
  PaintConveyor *p = &s->paintConveyor;
  p->capacity = PAINT_CONVEYOR_CAPACITY;
  p->buffer = (char (*)[PAINTED_NAME_LEN]) calloc(p->capacity, sizeof(*p->buffer));
  p->head = p->tail = p->count = 0;
  p->finished = 0;
  pthread_mutex_init(&p->mutex, NULL);
  pthread_cond_init(&p->notFull, NULL);
  pthread_cond_init(&p->notEmpty, NULL);

  return s;
}

void shared_destroy(SharedData *s) {
  if (s == NULL)
    return;

  BodyConveyor *b = &s->bodyConveyor;
  for (int t = 0; t < BodyTypeN; t++)
    pthread_cond_destroy(&b->roomFor[t]);
  pthread_cond_destroy(&b->notEmpty);
  pthread_mutex_destroy(&b->mutex);
  for (int r = 0; r < PaintRobotTypeN; r++)
    free(b->consumed[r]);
  free(b->buffer);

  PaintConveyor *p = &s->paintConveyor;
  pthread_cond_destroy(&p->notFull);
  pthread_cond_destroy(&p->notEmpty);
  pthread_mutex_destroy(&p->mutex);
  free(p->buffer);

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
