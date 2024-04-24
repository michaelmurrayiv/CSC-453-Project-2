#include "rr.c"
#include "helpers.c"
#include <sys/mman.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/resource.h>


tid_t threadCount = 1;

static void lwp_wrap(lwpfun fun, void *arg){
  printf("inside lwp_wrap\n");
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
  size_t lim = getrlimit(RLIMIT_STACK, &limit);
  if ((int)lim != -1 && limit.rlim_cur != RLIM_INFINITY) { // check if RLIMIT_STACK exists, if not use 8MB
    stackSize = limit.rlim_cur;
  }
  if (stackSize%(int)memPageSize != 0) { // make sure that stack size is a multiple of the memory page size
    stackSize = stackSize/memPageSize+memPageSize;
  }
  unsigned long *stack = (unsigned long *) mmap(NULL, stackSize, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS|MAP_STACK, -1, 0);

  stack = stack + stackSize; // move sp to bottom of allocated memory

  //state setup
  rfile oldState;
  rfile newState;

// How do I call lwp_wrap/set up the registers for that?
//??? which registers for which functions

  newState.rdi = (unsigned long) fun; // set return address to lwp_wrap function
  newState.rsi = (unsigned long) arg; // set rdi & other registers to input args
  
  newState.rsp = (unsigned long) fun; // current stack??
  newState.fxsave=FPU_INIT; //set floating point state as specified in doc

  printf("val is %lx\n",fun);
  // create thread
  
  context context = {
    tid,
    stack,
    stackSize,
    newState,
    LWP_LIVE,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL
  };
  thread newThread = &context;
  printf("%lx\n", (unsigned long)newThread->state.rdi);

//create thread m, admit to scheduler s
  s->admit(newThread);
  return tid;
};

extern void lwp_start(void){
  //start running first thread in scheduler
  printf("inside lwp_start\n");  
  scheduler s = &rr;
  thread next = s->next();
  
  //below call causes invalid read of size 8
  rfile newState = next->state;

  printRFile(&newState);

  //create system context and add to scheduler
  rfile systemState;
  struct rlimit limit;
  size_t lim = getrlimit(RLIMIT_STACK, &limit);
  if ((int)lim == -1) {
    fprintf(stderr, "error: getrlimit failed in lwp_start");
    abort();
  }

  context context = {
    threadCount,
    &systemState.rsp,
    limit.rlim_cur,
    systemState,
    LWP_LIVE,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL
  };
  thread systemThread = &context;

  threadCount++;
  s->admit(systemThread);

  //newState set up incorrectly -> leads to error
  swap_rfiles(&systemState, &newState);
  return;
};
