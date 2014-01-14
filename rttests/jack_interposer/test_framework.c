#include <jack/jack.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>

bool been_in_process = false;

void handle_abort(int sig)
{
  // we should abort
  exit(0);
}

int test_process_callback(jack_nframes_t nframes, void* arg);

int main()
{
  signal(SIGABRT, handle_abort);

  jack_client_t * client = jack_client_open("testclient", JackNullOption, NULL);
  jack_set_process_callback(client, test_process_callback, NULL);
  jack_activate(client);
  sleep(1);
  jack_deactivate(client);
  printf("Been in process: %d\n", been_in_process);
  return -1;
}
