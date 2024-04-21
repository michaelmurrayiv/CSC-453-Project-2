#include "lwp.h"
#include "rr.h"



// initial functions to write
extern tid_t lwp_create(lwpfun,void *)
  //create thread m, admit to scheduler s
;

extern void lwp_start(void){
  //start running first thread in scheduler
};

// temporary main for testing rr scheduler:
int main(){
  scheduler s = &rr;
};