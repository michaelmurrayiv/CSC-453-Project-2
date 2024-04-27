#include "lwp.h"
#include "rr.h"

thread head = NULL;

// void init_rr(){};
// void shutdown_rr(){};

void admit(thread new){ // add thread to linked list
  printf("Admitted tid: %d\n",new->tid);
  printf("qlen is %d\n" , qlen());

  new->sched_one=NULL;
  new->sched_two=NULL;
  if (head == NULL) {
    head = new;
    return;
  }printf("head tid: %d", head->tid);
  if (head->sched_one !=NULL){
    printf("what>\n");
  }  
  if (head->sched_two !=NULL){
    printf("what>\n");
  }
  while (head->sched_two != NULL) { // move to tail of list
    head = head->sched_two;  printf("here1\n");
  }
  if (new->sched_one != NULL) {
    printf("schedone: %d\n",new->sched_one->tid);
  }
    if (new->sched_two != NULL) {
    printf("schedtwo: %d\n",new->sched_two->tid);
  }
  new->sched_one = head; // set prev for new thread
  new->sched_two = NULL; // set next to null 
  head->sched_two = new; // set next as new thread
  if (new->sched_one != NULL) {
    printf("schedone: %d\n",new->sched_one->tid);
  }
    if (new->sched_two != NULL) {
    printf("schedtwo: %d\n",new->sched_two->tid);
  }
  while (head->sched_one != NULL) { // move to head of list
    head = head->sched_one;  
  }
  return;
  printf("here\n");
};
void removeThread(thread victim){ // remove thread from queue
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


