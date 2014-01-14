#include "test_framework.c"

int test_process_callback(jack_nframes_t nframes, void* arg)
{
  printf("Fail %d", 42);
  been_in_process = true;
  return 1;
}
