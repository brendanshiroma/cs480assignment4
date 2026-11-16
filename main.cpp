#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>

#include "shared.h"
#include "producer.h"
#include "consumer.h"
#include "finalline.h"
#include "log.h"

/*
 * Multi-station car assembly line.
 *
 *   SedanWelder, SUVWelder  -> body conveyor  -> AquaCoat, PrimeSpray
 *   AquaCoat, PrimeSpray    -> paint conveyor -> FinalLine
 *
 * Usage: carassembly [-n N] [-s N] [-u N] [-a N] [-p N] [-f N]
 */
int main(int argc, char *argv[]) {
  unsigned int productionLimit = DEFAULT_PRODUCTION_LIMIT;
  unsigned int sedanMs = 0, suvMs = 0;      /* -s, -u */
  unsigned int aquaMs = 0, primeMs = 0;     /* -a, -p */
  unsigned int finalMs = 0;                 /* -f     */
  int option;

  while ((option = getopt(argc, argv, "n:s:u:a:p:f:")) != -1) {
    switch (option) {
      case 'n': productionLimit = (unsigned int) atoi(optarg); break;
      case 's': sedanMs = (unsigned int) atoi(optarg); break;
      case 'u': suvMs   = (unsigned int) atoi(optarg); break;
      case 'a': aquaMs  = (unsigned int) atoi(optarg); break;
      case 'p': primeMs = (unsigned int) atoi(optarg); break;
      case 'f': finalMs = (unsigned int) atoi(optarg); break;
      default:
        fprintf(stderr,
                "Usage: %s [-n N] [-s N] [-u N] [-a N] [-p N] [-f N]\n",
                argv[0]);
        exit(EXIT_FAILURE);
    }
  }

  /* shared data, created and initialized by the main thread */
  SharedData *shared = shared_create(productionLimit);
  if (shared == NULL) {
    fprintf(stderr, "Unable to allocate shared data\n");
    exit(EXIT_FAILURE);
  }

  /* per-thread arguments */
  ThreadArgs sedanArgs = { shared, Sedan,      sedanMs };
  ThreadArgs suvArgs   = { shared, SUV,        suvMs   };
  ThreadArgs aquaArgs  = { shared, AquaCoat,   aquaMs  };
  ThreadArgs primeArgs = { shared, PrimeSpray, primeMs };
  ThreadArgs finalArgs = { shared, 0,          finalMs };

  pthread_t sedanWelder, suvWelder, aquaCoat, primeSpray, finalLine;

  /* create and start ALL worker threads together */
  pthread_create(&sedanWelder, NULL, body_producer,        &sedanArgs);
  pthread_create(&suvWelder,   NULL, body_producer,        &suvArgs);
  pthread_create(&aquaCoat,    NULL, body_consumer,        &aquaArgs);
  pthread_create(&primeSpray,  NULL, body_consumer,        &primeArgs);
  pthread_create(&finalLine,   NULL, paintedbody_consumer, &finalArgs);

  /* barrier: block until FinalLine has finished the last car */
  sem_wait(&shared->barrier);

  /* final report */
  log_body_history(shared->bodyConveyor.produced,
                   shared->bodyConveyor.consumed);

  /* All workers have finished their work at this point; reap them
   * so the shared data can be released safely. */
  pthread_join(sedanWelder, NULL);
  pthread_join(suvWelder,   NULL);
  pthread_join(aquaCoat,    NULL);
  pthread_join(primeSpray,  NULL);
  pthread_join(finalLine,   NULL);

  shared_destroy(shared);
  return 0;
}
