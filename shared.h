#ifndef SHARED_H
#define SHARED_H

#include <pthread.h>
#include <semaphore.h>

#include "car_assembly.h"

#define BODY_CONVEYOR_CAPACITY   16   /* max bodies on body conveyor        */
#define MAX_SEDAN_ON_CONVEYOR    12   /* max Sedan bodies on body conveyor  */
#define MAX_SUV_ON_CONVEYOR       6   /* max SUV bodies on body conveyor    */
#define PAINT_CONVEYOR_CAPACITY  10   /* max painted bodies on paint conv.  */
#define PAINTED_NAME_LEN         64   /* e.g. "AquaCoatSUV"                 */
#define DEFAULT_PRODUCTION_LIMIT 100

/*
 * Monitor for the body conveyor (station 1 -> station 2).
 * FIFO circular buffer of body types, protected by a mutex,
 * with condition variables for producers (per body type) and consumers.
 */
struct BodyConveyor {
  BodyType *buffer;            /* circular buffer (heap allocated)         */
  unsigned int capacity;       /* BODY_CONVEYOR_CAPACITY                    */
  unsigned int head;           /* index of next body to remove              */
  unsigned int tail;           /* index of next free slot                   */
  unsigned int count;          /* number of bodies currently on conveyor    */

  unsigned int typeLimit[BodyTypeN];  /* per-type max on the conveyor       */
  unsigned int inQueue[BodyTypeN];    /* per-type count on conveyor now     */
  unsigned int produced[BodyTypeN];   /* per-type count ever added          */

  unsigned int productionLimit;  /* total number of cars (-n)               */
  unsigned int reserved;         /* production slots claimed by producers   */
  unsigned int taken;            /* bodies removed by paint robots          */

  /* consumed[robot] -> array indexed by BodyType (heap allocated) */
  unsigned int *consumed[PaintRobotTypeN];

  pthread_mutex_t mutex;
  pthread_cond_t  roomFor[BodyTypeN];  /* producer of type t waits here     */
  pthread_cond_t  notEmpty;            /* paint robots wait here            */
};

/*
 * Monitor for the paint conveyor (station 2 -> station 3).
 * FIFO circular buffer of painted-body names.
 */
struct PaintConveyor {
  char (*buffer)[PAINTED_NAME_LEN];  /* circular buffer (heap allocated)    */
  unsigned int capacity;             /* PAINT_CONVEYOR_CAPACITY             */
  unsigned int head;
  unsigned int tail;
  unsigned int count;

  unsigned int finished;             /* painted bodies consumed so far      */

  pthread_mutex_t mutex;
  pthread_cond_t  notFull;           /* paint robots wait here              */
  pthread_cond_t  notEmpty;          /* FinalLine waits here                */
};

/* Everything shared among the worker threads */
struct SharedData {
  BodyConveyor  bodyConveyor;
  PaintConveyor paintConveyor;
  unsigned int  productionLimit;
  sem_t         barrier;   /* posted by FinalLine after the last car */
};

/* Per-thread argument: shared data + thread-specific info */
struct ThreadArgs {
  SharedData  *shared;
  int          id;         /* BodyType for welders, PaintRobotType for painters */
  unsigned int sleepMs;    /* simulated work time in milliseconds */
};

/* Create / destroy the shared data structure */
SharedData *shared_create(unsigned int productionLimit);
void        shared_destroy(SharedData *shared);

/* Sleep for the given number of milliseconds (no-op for 0) */
void sleep_ms(unsigned int ms);

#endif
