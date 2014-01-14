#include "test_framework.c"
#include <pthread.h>

pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

int test_process_callback(jack_nframes_t nframes, void* arg)
{
  pthread_cond_wait(&cond, &mutex);
  been_in_process = true;
  return 1;
}
