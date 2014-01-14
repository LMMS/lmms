#include "test_framework.c"

class Object 
{
};

int test_process_callback(jack_nframes_t nframes, void* arg)
{
  new Object();
  return 1;
}
