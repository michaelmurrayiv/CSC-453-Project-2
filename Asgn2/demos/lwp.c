#include "rr.c"
#include <sys/mman.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/resource.h>


tid_t threadCount = 1;

static void lwp_wrap(lwpfun fun, void *arg){
  int rval;
  rval=fun(arg);
  //lwp_exit(rval);
  return;
}

tid_t lwp_create(lwpfun fun, void *arg){
  scheduler s = &rr;

  //tid
  tid_t tid = threadCount;
  threadCount++;

  //allocate stack
  int stackSize = 1000000; //8MB size
  long memPageSize = sysconf(_SC_PAGESIZE);
  struct rlimit limit;
  int lim = getrlimit(RLIMIT_STACK, &limit);
  if (lim != -1 && limit.rlim_cur != RLIM_INFINITY) { // check if RLIMIT_STACK exists, if not use 8MB
    stackSize = limit.rlim_cur;
  }
  if (stackSize%(int)memPageSize != 0) { // make sure that stack size is a multiple of the memory page size
    stackSize = stackSize/memPageSize+memPageSize;
  }
  unsigned long *stack = (unsigned long *) mmap(NULL, stackSize, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS|MAP_STACK, -1, 0);

  *stack = *stack + stackSize; // move sp to bottom of allocated memory


  //register setup

  rfile oldState;
  rfile newState;

  //set state
  newState.rsp = (unsigned long) lwp_wrap; // set return address to lwp_wrap function
  newState.rdi = (unsigned long) arg; // set rdi & other registers to input args
  newState.fxsave=FPU_INIT; //set floating point state as specified in doc

  swap_rfiles(&oldState, &newState);

  //exit status
  unsigned int exitStatus = LWP_LIVE;

  //set pointers
  thread lib_one = NULL;
  thread lib_two = NULL;
  thread sched_one = NULL;
  thread sched_two = NULL;
  thread exited = NULL;

  // create thread
  thread newThread = malloc(sizeof (thread));
  if (newThread == NULL) {
    fprintf(stderr, "error: malloc for newThread failed\n");
    abort();
  }

  newThread->tid = tid;
  newThread->stack = stack;
  newThread->stacksize = stackSize;
  newThread->state = newState;
  newThread->status = exitStatus;
  newThread->lib_one = lib_one;
  newThread->lib_two = lib_two;
  newThread->sched_one = sched_one;
  newThread->sched_two = sched_two;
  newThread->exited = exited;

//create thread m, admit to scheduler s
  s->admit(newThread);
  return tid;
};

extern void lwp_start(void){
  //start running first thread in scheduler
};
