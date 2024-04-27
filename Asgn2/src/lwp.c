#include "rr.c"
#include "helpers.c"
#include <sys/mman.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <string.h>

tid_t threadNum = 2; //system thread will be thread 1
thread currThread = NULL;
thread threadList = NULL; // all system threads for tid2thread
thread waitQueue = NULL; // waiting threads
thread exitedQueue = NULL; // finished threads

static void lwp_wrap(lwpfun fun, void *arg){
  printf("inside lwp_wrap\n");
  int rval;
  rval=fun(arg);
  lwp_exit(rval);
  return;
}

tid_t lwp_create(lwpfun fun, void *arg){
  scheduler s = lwp_get_scheduler();

  //tid
  tid_t tid = threadNum;
  
  //allocate stack
  long stackSize = 1000000; //8MB size
  long memPageSize = sysconf(_SC_PAGESIZE);
  struct rlimit limit;
  size_t lim = getrlimit(RLIMIT_STACK, &limit);
  if ((int)lim != -1 && limit.rlim_cur != RLIM_INFINITY) { // check if RLIMIT_STACK exists, if not use 8MB
    stackSize = limit.rlim_cur;
  }
  if (stackSize%memPageSize == 0) { // make sure that stack size is a multiple of the memory page size
    stackSize = (stackSize/memPageSize + 1) * (memPageSize);
  }
  unsigned long *stack = (unsigned long *) mmap(NULL, stackSize, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS|MAP_STACK, -1, 0);
  printf("stack address: %lx\n",(unsigned long) stack);
  printf("stack size: %lx\n",(unsigned long) stackSize);

  stack = stack + stackSize/(sizeof(unsigned long)); // go to bottom of stack
  printf("stack address: %lx\n",(unsigned long) stack);
  //here I want to push lwp_wrap to stack
  stack = stack - 2; //decrement stack by 16 bytes
  *stack = (unsigned long) lwp_wrap;

  printf("stack address: %lx\n", (unsigned long)stack);
  printf("val in stack: %lu\n", *stack);
  printf("stack address: %lx\n", (unsigned long)stack);

  //increment stack by 16 bytes to be above ret
  stack--;
  printf("stack address: %lx\n", (unsigned long)stack);

  //state setup
  rfile oldState;
  rfile newState;

  newState.rdi = (unsigned long) fun; // set return address to lwp_wrap function
  newState.rsi = (unsigned long) arg; // set rdi & other registers to input args
  newState.rbp = (unsigned long) stack;
  newState.fxsave=FPU_INIT; //set floating point state as specified in doc

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
  thread newThread = mmap(NULL, sizeof(thread), PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
  if (newThread != MAP_FAILED) {
    memcpy(newThread, &context, sizeof(context));
  }
  threadNum++;
  printRFile(&newThread->state);  

  //create thread m, admit to scheduler s and add to list
  s->admit(newThread);
  
  if (threadList==NULL){
    threadList=newThread;
  } else {
    while (threadList->lib_two!=NULL) { 
      threadList = threadList->lib_two;
    }
    newThread->lib_one=threadList;
    threadList->lib_two=newThread;
    while (threadList->lib_one!=NULL){
      threadList = threadList->lib_one;
    }
  }

  printf("comon\n");
printf("%d\n",(int) threadList->tid);
  return tid;
};

extern void lwp_start(void){
  printf("inside lwp_start\n");  
  //create system thread and add to scheduler
  scheduler s = lwp_get_scheduler();
  thread new = s->next();
  rfile systemState;
  rfile newState = new->state;
  struct rlimit limit;
  size_t lim = getrlimit(RLIMIT_STACK, &limit);
  if ((int)lim == -1) {
    fprintf(stderr, "error: getrlimit failed in lwp_start");
    abort();
  }
  tid_t tid = 1;
  //system context
  context context = {
    tid,
    NULL,
    limit.rlim_cur,
    systemState,
    LWP_LIVE,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL
  };
  thread systemThread = mmap(NULL, sizeof(thread), PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
  if (systemThread != MAP_FAILED) {
    memcpy(systemThread, &context, sizeof(context));
  }
  s->admit(systemThread);
  
  if (threadList==NULL){
    threadList=systemThread;
  } else {
    while (threadList->lib_two!=NULL) {
      printf("d\n");
      threadList = threadList->lib_two;
    }
    systemThread->lib_one=threadList;
    threadList->lib_two=systemThread;

    while (threadList->lib_one!=NULL){
      threadList = threadList->lib_one;  printf("d2\n");
    }
  }
  printf("%d\n",(int)new->tid);
  printf("%d\n",(int)threadList->tid);
  printThread(&threadList);
  //yield to new thread
  lwp_yield();
  return;
};

tid_t lwp_gettid(){
  if (currThread==NULL){
    return NO_THREAD;
  }
  return currThread->tid;
}

thread tid2thread(tid_t tid){
  thread ret = threadList;
  printf("debug\n");
  printf("%d\n",(int) threadList->tid);
  if (threadList->lib_one==NULL){
    printf("one null\n");
  }
  if (threadList->lib_two==NULL){
    printf("two null\n");
  }
  while (ret!=NULL){
    printf("%d\n",(int) tid);
    if (ret->tid==tid) {
      break;
    }
    printf("%d\n", (int)ret->tid);
    ret = ret->lib_two;
  }
  return ret;
}

void lwp_yield() {
  printf("inside lwp_yield\n");
  scheduler s = lwp_get_scheduler();
  tid_t curr = lwp_gettid();
  thread old;printf("here!\n");
  if (curr==NO_THREAD){ //check for system thread
    tid_t t = 1;printf("aaa!\n");
    old = tid2thread(t);
    printThread(&old);
  } else {
    old = tid2thread(curr);
  }printf("here!\n");
  thread new = s->next();printf("here!\n");
  printf("oncde\n");
  s->remove(new); printf("sddddd\n");// move thread to back of scheduler
  s->admit(new);printf("here!\n");
  tid_t o = old->tid;printf("hersdfe!\n");
  tid_t n = new->tid;printf("heredsf!\n");
  printf("%d\n", (int)o);
  printf("%d\n", (int)n);printf("here!\n");
  currThread=new;
  printf("here!\n");
  swap_rfiles(&old->state, &new->state);
  printf("here\n");
  return; 
}

scheduler lwp_get_scheduler(){
  return &rr;
}

void lwp_set_scheduler(scheduler fun){
  if (fun==NULL) {
    return;
  } else {
    rr=*fun;
    return;
  }
}

void lwp_exit(int status){
  printf("lwpexit\n");
  tid_t tid = lwp_gettid();
  if (tid==NO_THREAD){
    fprintf(stderr, "error: lwp_exit called on NO_THREAD");
    abort();
  }
   printf("tid is: %d\n", tid);
   printf("lwpexit2\n");
  thread curr = tid2thread(tid);
   printf("lwpexit2.5\n");
    printf("tid: %d\n",(int)curr->tid);
  curr->status = (unsigned int) status;
   printf("lwpexit3\n");
  if (exitedQueue==NULL){
    exitedQueue=curr;
  } else {
    thread head = exitedQueue;
    while (exitedQueue->exited!=NULL){
      exitedQueue = exitedQueue->exited;
    }
    exitedQueue->exited = curr;
    exitedQueue = head;
  }
   printf("lwpexit4\n");
  lwp_yield();

}
