#include "rr.c"
#include "helpers.c"
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
  
  printf("%d\n", arg);
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

  *stack = *stack + stackSize; // move sp to bottom of allocated memory

  printf("here\n");

  //register setup

  rfile oldState;
  rfile newState;

  //set state
  newState.rdi = (unsigned long) lwp_wrap; // set return address to lwp_wrap function
  newState.rsi = (unsigned long) arg; // set rdi & other registers to input args
  newState.fxsave=FPU_INIT; //set floating point state as specified in doc
  newState.rbp = (unsigned long) 0x1ffefffde0; // valgrind says this is the return address
  newState.rsp = (unsigned long) stack; // valgrind says this is where SP changes to
  //NEED TO SET TO ADDRESS OF CALLED FUNCTION
  printRFile(&newState);
  swap_rfiles(&oldState, &newState); //SHOULD RETURN BACK TO HERE
  printRFile(&oldState);
 


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
  newThread->status = LWP_LIVE;
  newThread->lib_one = NULL;
  newThread->lib_two = NULL;
  newThread->sched_one = NULL;
  newThread->sched_two = NULL;
  newThread->exited = NULL;

//create thread m, admit to scheduler s
  s->admit(newThread);
  return tid;
};

extern void lwp_start(void){
  //start running first thread in scheduler
  
  rfile systemState;
  rfile newState;

  scheduler s = &rr;
  thread next = s->next();
  newState = next->state;

  printRFile(&newState);

  swap_rfiles(&systemState, &newState);
  printRFile(&systemState);
  
  struct rlimit limit;
  size_t lim = getrlimit(RLIMIT_STACK, &limit);
  if ((int)lim == -1) {
    fprintf(stderr, "error: getrlimit failed in lwp_start");
    abort();
  }

  thread systemThread = malloc(sizeof (thread));
  if (systemThread == NULL) {
    fprintf(stderr, "error: malloc for systemThread failed\n");
    abort();
  }

  systemThread->tid = threadCount;
  systemThread->stack = &systemState.rsp;
  systemThread->stacksize = limit.rlim_cur;
  systemThread->state = newState;
  systemThread->status = LWP_LIVE;
  systemThread->lib_one = NULL;
  systemThread->lib_two = NULL;
  systemThread->sched_one = NULL;
  systemThread->sched_two = NULL;
  systemThread->exited = NULL;

  threadCount++;

  s->admit(systemThread);
  return;
};
