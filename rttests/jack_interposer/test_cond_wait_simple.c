#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>

pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

bool done = false;

void* signaller(void* arg)
{
  while(!done)
  { 
    pthread_cond_signal(&cond);
  }
}

int main()
{
  pthread_t thread;
  pthread_create(&thread, NULL, &signaller, NULL);
  pthread_cond_wait(&cond, &mutex);
  done = 1;
  pthread_join(thread, NULL);
  return 0;
}
