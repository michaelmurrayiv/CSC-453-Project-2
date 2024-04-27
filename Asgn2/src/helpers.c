#include "helpers.h"

void printThreadVal(thread link, char* name) {
  if (link==NULL){
    printf("\t%s: NULL\n", name);
  } 
  else {
    printf("\t%s: tid %d\n", name, link->tid);
  }
  return;
}
void printThread(thread *t) {
  thread thread = *t;
  printf("Thread Content:\n\ttid: %d\n",thread->tid);
  printf("\tstatus: %d\n", thread->status);
  printThreadVal(thread->lib_one, "lib_one");
  printThreadVal(thread->lib_two, "lib_two");
  printThreadVal(thread->sched_two, "sched_two");
  printThreadVal(thread->sched_one, "sched_one");
  printThreadVal(thread->exited, "exited");
  return;
}

void printRFile(rfile* file) {

  printf("rfile contents:\n");
  // printf("rax: %lx\n",file->rax);
  // printf("rbx: %lx\n",file->rbx);
  // printf("rcx: %lx\n",file->rcx);
  // printf("rdx: %lx\n",file->rdx);
  printf("rsi: %lx\n",file->rsi);
  printf("rdi: %lx\n",file->rdi);
  printf("rbp: %lx\n",file->rbp);
  // printf("rsp: %lx\n",file->rsp);
  // printf("r8: %lx\n",file->r8);
  // printf("r9: %lx\n",file->r9);
  // printf("r10: %lx\n",file->r10);
  // printf("r11: %lx\n",file->r11);
  // printf("r12: %lx\n",file->r12);
  // printf("r13: %lx\n",file->r13);
  // printf("r14: %lx\n",file->r14);
  // printf("r15: %lx\n",file->r15);

  return;
}
