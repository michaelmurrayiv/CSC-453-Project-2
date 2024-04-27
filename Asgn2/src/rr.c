#include "lwp.h"
#include "rr.h"

thread head = NULL;

// void init_rr(){};
// void shutdown_rr(){};

void admit(thread new){ // add thread to linked list
  //HOW TO ACCESS GLOBAL THREAD HEAD
  if (head == NULL) {
    head = new;
    return;
  }
  while (head->sched_two != NULL) { // move to tail of list
    head = head->sched_two;
  }
  new->sched_one = head; // set prev for new thread
  new->sched_two = NULL; // set next to null 
  head->sched_two = new; // set next as new thread

  while (head->sched_one != NULL) { // move to head of list
    head = head->sched_one;
  }
  return;

};
void removeThread(thread victim){ // remove thread from queue
  printf("inside removethread\n");
  if (head == NULL) {
    printf("Error: no threads scheduled");
    return;
  }
  tid_t curr = head->tid;
  while (curr != victim->tid) { // find victim
    head = head->sched_two;
    curr = head->tid;
  }
  thread prev = head->sched_one;
  thread next = head->sched_two;

  //deallocate thread?
  if (prev==NULL && next==NULL) {
    head=NULL;
    return;
  } else if (prev==NULL) {
    printf("here?\n");
    head = next;
    head->sched_one = NULL;
  } else if (next==NULL) {
    head = prev;
    head->sched_two = NULL;
  } else {
    head=next;
    head->sched_one=prev;
    head=prev;
    head->sched_two=next;
  }

  //if the list is not empty, return to head
  while (head->sched_one != NULL) {
    head = head->sched_one;
  }
  return;

};
thread next(){ // select a thread to schedule
  return head;
};

int qlen(){ // number of ready threads 
  if (head==NULL) {
    return 0;
  }
  int count = 1;
  while (head->sched_two!=NULL) {
    count++;
    head=head->sched_two;
  }
  while (head->sched_one!=NULL) {
    head=head->sched_one;
  }
  return count;
};

struct scheduler rr = {NULL, NULL, admit, removeThread, next, qlen};


