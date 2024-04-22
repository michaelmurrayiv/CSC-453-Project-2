#include "../include/rr.c"
#include <unistd.h>
#include <sys/time.h>
#include <sys/resource.h>
#include "lwp.h"
#include <inttypes.h>


int main(int argc, char *argv[]) {
  scheduler s = &rr;
  long size = sysconf(_SC_PAGESIZE);
  struct rlimit limit;
  int lim = getrlimit(RLIMIT_STACK, &limit);
  if (lim==-1){printf("error with getrlimit");}
  thread t = NULL;
  

  unsigned long rax = 0;            /* the sixteen architecturally-visible regs. */
  unsigned long rbx = 0;
  unsigned long rcx = 0;
  unsigned long rdx = 0;
  unsigned long rsi = 0;
  unsigned long rdi = 0;
  unsigned long rbp = 0x1ffefffdd0;
  unsigned long rsp = 0x1ffefffdd0;
  unsigned long r8 = 0;
  unsigned long r9 = 0;
  unsigned long r10 = 0;
  unsigned long r11 = 0;
  unsigned long r12 = 0;
  unsigned long r13 = 0;
  unsigned long r14 = 0;
  unsigned long r15 = 0;
  struct fxsave fxsave;
  rfile new = {
    rax, rbx, rcx, rdx, rsi, rdi, rbp, rsp, r8, r9, r10, r11, r12, r13, r14, r15, fxsave
  };
  rfile old = {
    rax, rbx, rcx, rdx, rsi, rdi, rbp, rsp, r8, r9, r10, r11, r12, r13, r14, r15, fxsave
  };
  //NEED TO SET NEW AS STACK TO MOVE TO

  swap_rfiles(&old, &new);
  s->admit(t); // SUCCESSFULLY ADMITS THREAD T TO QUEUE
  printf("%d\n", (int)size);
  printf("%lu\n", old.rbp); //prints out in decimal format, should convert to hexadecimal for my viewing. Maybe a way to print
  printf("%lX\n", old.rsp); //prints in hex!
  printf("%lX\n", old.rdi);
  printf("%lX\n", old.rax);
  printf("%lX\n", old.rbx);
  printf("%lX\n", old.rcx);
  printf("%lX\n", old.rdx);

  return 0;
}