#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "date.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"

uint64
sys_exit(void)
{
  int n;
  if(argint(0, &n) < 0)
    return -1;
  exit(n);
  return 0;  // not reached
}

uint64
sys_getpid(void)
{
  return myproc()->pid;
}

uint64
sys_fork(void)
{
  return fork();
}

uint64
sys_wait(void)
{
  uint64 p;
  if(argaddr(0, &p) < 0){
    return -1;
  }

  return wait(p);
}

uint64
sys_sbrk(void)
{
  int addr;
  int n;

  if(argint(0, &n) < 0)
    return -1;
  addr = myproc()->sz;
  if(growproc(n) < 0)
    return -1;
  return addr;
}

uint64
sys_sleep(void)
{
  int n;
  uint ticks0;

  if(argint(0, &n) < 0)
    return -1;
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(myproc()->killed){
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}

uint64
sys_kill(void)
{
  int pid;
  int signum; 

  if(argint(0, &pid) < 0)
    return -1;

  if(argint(1, &signum) < 0)
    return -1;  

  return kill(pid, signum);
}

// return how many clock tick interrupts have occurred
// since start.
uint64
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}

uint64
sys_sigprocmask(void)
{
  //calls sigprocmask uint sigprocmask (uint sigmask); 
  int sigmask; 
  if(argint(0, &sigmask) < 0)
    return -1;

  if (sigmask < 0)
    return -1;
  //*****NEW****
  // int oldmask = myproc()->signalMask;
  // myproc()->signalMask = (uint)sigmask;
  // return oldmask;
 //*****END_NEW****

  return (uint64)sigprocmask((uint)sigmask);
}

uint64 
sys_sigaction(void)
{
  //This system call will register a new handler for a given signal number (signum). sigaction returns 0 on success, on error, -1 is returned.
  
  int signum;
  const struct sigaction* act;
  struct sigaction* oldact;

  if(argint(0, &signum) < 0 || argstr(1, (void*)&act, sizeof(*act)) < 0 || argstr(2, (void*)&oldact, sizeof(*oldact)) < 0)
    return -1;
  
  return (uint64)sigaction(signum, act, oldact); 
}

uint64
sys_sigret(void)
{
  // This system call will be called implicitly when returning from user space after handling a signal.
  sigret();
  return 0; 
}

