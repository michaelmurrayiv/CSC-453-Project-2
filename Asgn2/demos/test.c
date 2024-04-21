#include "../include/rr.c"

int main(int argc, char *argv[]) {
  scheduler s = &rr;

  thread t = NULL;

  s->admit(t); // SUCCESSFULLY ADMITS THREAD T TO QUEUE
  printf("here\n");
  return 0;
}