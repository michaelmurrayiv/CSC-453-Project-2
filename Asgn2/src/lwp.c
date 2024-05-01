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

int nextTime = 0;

static void lwp_wrap(lwpfun fun, void *arg){
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
  if (stack==NULL) {
    perror("mmap failed");
  }
  stack = stack + stackSize/(sizeof(unsigned long)); // go to bottom of stack

  stack = stack - 2; //decrement stack by 16 bytes
  *stack = (unsigned long) lwp_wrap;
  stack--;
  
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

  return tid;
};

extern void lwp_start(void){
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
  thread systemThread = mmap(NULL, sizeof(context), PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
  if (systemThread != MAP_FAILED) {
    memcpy(systemThread, &context, sizeof(context));
  }
  s->admit(systemThread);
  
  if (threadList==NULL){
    threadList=systemThread;
  } else {
    while (threadList->lib_two!=NULL) {
      threadList = threadList->lib_two;
    }
    systemThread->lib_one=threadList;
    threadList->lib_two=systemThread;

    while (threadList->lib_one!=NULL){
      threadList = threadList->lib_one;
    }
  }
  //yield to new thread
  currThread=new;
  swap_rfiles(&(systemThread->state), &newState);
  return;
};

tid_t lwp_gettid(){
  if (currThread==NULL){
    return NO_THREAD;
  }
  return currThread->tid;
}

thread tid2thread(tid_t tid){
  while (threadList != NULL && threadList->lib_one!=NULL) {
    threadList = threadList->lib_one;
  }
  thread ret = threadList;
  while (ret!=NULL){
    if (ret->tid==tid) {
      break;
    }
    ret = ret->lib_two;
  }
  return ret;
}

void lwp_yield() {
  scheduler s = lwp_get_scheduler();
  tid_t curr = lwp_gettid();

  thread old;
  if (curr==NO_THREAD){ //check for system thread
    exit(0);
  } else {
    old = tid2thread(curr);
  }    
  s->remove(old);

  if (old->status==LWP_LIVE){
    s->admit(old);
  }

  thread new = s->next();
  if (new==NULL){
    lwp_exit(3);
  }

  currThread = new;

  swap_rfiles(&old->state, &new->state);
  return; 
}

scheduler lwp_get_scheduler(){
  return &rr;
}

void lwp_set_scheduler(scheduler fun){
  if (fun==NULL) {
    return;
  } else {
    scheduler s = lwp_get_scheduler();
    fun->init();
    while (s->next()!=NULL){
      thread tmp = s->next();
      s->remove(tmp);
      fun->admit(tmp);
    }
    rr=*fun;
    return;
  }
}

void lwp_exit(int status){
  scheduler s = lwp_get_scheduler();
  tid_t tid = lwp_gettid();
  if (tid==NO_THREAD){
    fprintf(stderr, "error: lwp_exit called on NO_THREAD");
    abort();
  }
  thread curr = tid2thread(tid);//get current thread
  curr->status = MKTERMSTAT(LWP_TERM, status); 

  if (tid==1){ //exiting the main thread
    if (curr->sched_two!=NULL){
      fprintf(stderr, "error: lwp_exit called on main with nonempty queue");
      abort();
    } else { //exit
      munmap(curr, sizeof(context));
      exit(0);
    }
  }


  if (exitedQueue==NULL){
    exitedQueue=curr;
  } else {
    thread tmp = exitedQueue;
    while (exitedQueue->exited!=NULL){
      exitedQueue = exitedQueue->exited;
    }
    exitedQueue->exited = curr;
    exitedQueue = tmp;
  }


  
  if (waitQueue!=NULL){
    thread toAdmit = waitQueue=waitQueue;
    waitQueue = waitQueue->exited;
    toAdmit->exited=NULL;
    toAdmit->status=LWP_LIVE;
    s->admit(toAdmit);
    int l2 = s->qlen();
  }

  lwp_yield();
}

tid_t lwp_wait(int *status){
  scheduler s = lwp_get_scheduler();
  tid_t tid = lwp_gettid();
  thread curr = tid2thread(tid);

  if (exitedQueue==NULL){ //if none have exited, block thread
    if (s->qlen() <= 1){ //if there are no threads remaining
      return NO_THREAD;
    }
    curr->status=LWP_TERM;
    if (waitQueue==NULL){
      waitQueue=curr;
    } else {
      thread tmp = waitQueue;
      while (waitQueue->exited!=NULL){
        waitQueue = waitQueue->exited;
      }
      waitQueue->exited = curr;
      waitQueue = tmp;
    }

    lwp_yield();
  } 
   //deallocate head of exited queue
    tid_t ret = exitedQueue->tid;

    if (status!=NULL) {
      *status = exitedQueue->status;
    }
    //remove from thread list
    while (threadList->tid!=ret){
      threadList=threadList->lib_two;
    }
    thread prev = threadList->lib_one;
    thread next = threadList->lib_two;
    if (prev==NULL && next==NULL) {
      threadList=NULL;
      return;
    } else if (prev==NULL) {
      threadList = next;
      threadList->lib_one = NULL;
    } else if (next==NULL) {
      threadList = prev;
      threadList->lib_two = NULL;
    } else {
      threadList=next;
      threadList->lib_one=prev;
      threadList=prev;
      threadList->lib_two=next;
    }

    //deallocate memory
    thread newHead = exitedQueue->exited;
    if (exitedQueue->tid!=(tid_t)1){
      unsigned long size = exitedQueue->stacksize;
      unsigned long* address = exitedQueue->stack + 3;
      address = address - size/(sizeof(unsigned long));
      int i = munmap(address, size);
    }
    int j = munmap(exitedQueue, sizeof(context));
    exitedQueue = newHead;
    return ret;
}
