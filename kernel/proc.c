#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "spinlock.h"
#include "proc.h"
#include "defs.h"

struct cpu cpus[NCPU];

struct proc proc[NPROC];

struct proc *initproc;

int nextpid = 1;
struct spinlock pid_lock;

int nexttid = 1; //*****THREADS*****
struct spinlock tid_lock; //*****THREADS*****

extern void forkret(void);
static void freeproc(struct proc *p);

extern char trampoline[]; // trampoline.S

// helps ensure that wakeups of wait()ing
// parents are not lost. helps obey the
// memory model when using p->parent.
// must be acquired before any p->lock.
struct spinlock wait_lock;

extern void* sigret_func_start(void); //task 2.4
extern void* sigret_func_end(void); 


// Allocate a page for each process's kernel stack.
// Map it high in memory, followed by an invalid
// guard page.
void
proc_mapstacks(pagetable_t kpgtbl) {
  struct proc *p;
  
  for(p = proc; p < &proc[NPROC]; p++) {
    char *pa = kalloc();
    if(pa == 0)
      panic("kalloc");
    uint64 va = KSTACK((int) (p - proc));
    kvmmap(kpgtbl, va, (uint64)pa, PGSIZE, PTE_R | PTE_W);
  }
}

// initialize the proc table at boot time.
void
procinit(void)
{
  struct proc *p;
  
  initlock(&pid_lock, "nextpid");
  initlock(&tid_lock, "nexttid"); //*****THREADS*****
  initlock(&wait_lock, "wait_lock");
  for(p = proc; p < &proc[NPROC]; p++) {
      initlock(&p->lock, "proc");
      p->kstack = KSTACK((int) (p - proc));
  }
}

// Must be called with interrupts disabled,
// to prevent race with process being moved
// to a different CPU.
int
cpuid()
{
  int id = r_tp();
  return id;
}

// Return this CPU's cpu struct.
// Interrupts must be disabled.
struct cpu*
mycpu(void) {
  int id = cpuid();
  struct cpu *c = &cpus[id];
  return c;
}

// Return the current struct proc *, or zero if none.
struct proc*
myproc(void) {
  push_off();
  struct cpu *c = mycpu();
  struct proc *p = c->proc;
  pop_off();
  return p;
}

//*****THREADS*****
struct thread*
mythread(void) {
  push_off();
  struct cpu *c = mycpu();
  struct thread *t = c->thread;
  pop_off();
  return t;
}

int
allocpid() {
  int pid;
  
  acquire(&pid_lock);
  pid = nextpid;
  nextpid = nextpid + 1;
  release(&pid_lock);

  return pid;
}

//*****THREADS*****
int
alloctid() {
  int tid;
  
  acquire(&tid_lock);
  tid = nexttid;
  nexttid = nexttid + 1;
  release(&tid_lock);

  return tid;
}

// Look in the process table for an UNUSED proc.
// If found, initialize state required to run in the kernel,
// and return with p->lock held.
// If there are no free procs, or a memory allocation fails, return 0.
static struct proc*
allocproc(void)
{
  struct proc *p;
  struct thread *t;

  for(p = proc; p < &proc[NPROC]; p++) {
    acquire(&p->lock);
    if(p->state == UNUSED) {
      goto found;
    } else {
      release(&p->lock);
    }
  }
  return 0;

found:
  p->pid = allocpid();
  p->state = USED;

 for(t = p->thread; t < &(p->thread[NTHREAD]); t++){
    t->state = T_UNUSED;
 }
  // Allocate a trapframe page.
  if((p->trapframe = (struct trapframe *)kalloc()) == 0){
    freeproc(p);
    release(&p->lock);
    return 0;
  }
  //*****THREADS*****
  int threadNum = 0; 
  //TODO : make sure this is the right way
  for(t = p->thread; t < &(p->thread[NTHREAD]); t++) {
    t->trapframe = p->trapframe + (sizeof(struct trapframe))*threadNum; 
    threadNum += 1; 
  }
  // An empty user page table.
  p->pagetable = proc_pagetable(p);
  if(p->pagetable == 0){
    freeproc(p);
    release(&p->lock);
    return 0;
  }

  // Set up new context to start executing at forkret,
  // which returns to user space.

  //*****THREADS REMOVED***** 
  //TODO: make sure
  // memset(&p->context, 0, sizeof(p->context));
  // p->context.ra = (uint64)forkret;
  // p->context.sp = p->kstack + PGSIZE;

  //2.1.2 updating process creation behavior
  //maybe we need to move towards all the array and intialize it to SIG_DFL. 
  p->signalMask = 0; 
  p->pendingSignals = 0;
  p->sigHandlerFlag=0;
  p->frozen = 0; 
  p->backupTrapframe = (struct trapframe *)kalloc();
  // struct sigaction sa;
  // memset(&sa,0,sizeof (sa));
  for(int i = 0; i<32; i++){
     p->signalHandlers[i] =  (void*) SIG_DFL; /* default signal handling */
     p->sigMaskArray[i] = 0;
  }

  return p;
}

static struct thread* 
allocthread(struct proc *p)
{
  struct thread *t;
  acquire(&p->lock);
    for(t = p->thread; t < &(p->thread[NTHREAD]); t++) {
      if(t->state == T_ZOMBIE){
        t->tid = 0;
        t->killed = 0;
        t->state = T_UNUSED;
        //t->kstack = 0;
        //kfree(t->kstack);
      }
      if(t->state == T_UNUSED) {
        goto found;
      } 
    }
  release(&p->lock);
  return 0;

  found:
  t->tid = alloctid(); 
  t->state = T_EMBRYO;
  t->killed = 0;
  //t-> parent = p; 
 
  // Set up new context to start executing at forkret,
  // which returns to user space.
  memset(&t->context, 0, sizeof(t->context));
  t->context.ra = (uint64)forkret;
  t->context.sp = t->kstack + PGSIZE; //TODO: make sure it's the right way

}
// free a proc structure and the data hanging from it,
// including user pages.
// p->lock must be held.
static void
freeproc(struct proc *p)
{
  struct thread *t;

  if(p->trapframe)
    kfree((void*)p->trapframe);
  p->trapframe = 0;
  if(p->backupTrapframe)
    kfree((void*)p->backupTrapframe);
  p->backupTrapframe = 0;
  if(p->pagetable)
    proc_freepagetable(p->pagetable, p->sz);
  p->pagetable = 0;
  p->sz = 0;
  p->pid = 0;
  p->parent = 0;
  p->name[0] = 0;
  //p->chan = 0;
  p->killed = 0;
  p->xstate = 0;
  p->state = UNUSED;

  //*****THREADS*****
  for(t = p->thread; t < &(p->thread[NTHREAD]); t++) {
    t->trapframe = 0; 
    t->killed = 0; 
    t->chan = 0;
    t->xstate = 0;
    t->state = T_UNUSED;
    //t->parent = 0;
  }
}

// Create a user page table for a given process,
// with no user memory, but with trampoline pages.
pagetable_t
proc_pagetable(struct proc *p)
{
  pagetable_t pagetable;

  // An empty page table.
  pagetable = uvmcreate();
  if(pagetable == 0)
    return 0;

  // map the trampoline code (for system call return)
  // at the highest user virtual address.
  // only the supervisor uses it, on the way
  // to/from user space, so not PTE_U.
  if(mappages(pagetable, TRAMPOLINE, PGSIZE,
              (uint64)trampoline, PTE_R | PTE_X) < 0){
    uvmfree(pagetable, 0);
    return 0;
  }

  // map the trapframe just below TRAMPOLINE, for trampoline.S.
  if(mappages(pagetable, TRAPFRAME, PGSIZE,
              (uint64)(p->trapframe), PTE_R | PTE_W) < 0){
    uvmunmap(pagetable, TRAMPOLINE, 1, 0);
    uvmfree(pagetable, 0);
    return 0;
  }

  return pagetable;
}

// Free a process's page table, and free the
// physical memory it refers to.
void
proc_freepagetable(pagetable_t pagetable, uint64 sz)
{
  uvmunmap(pagetable, TRAMPOLINE, 1, 0);
  uvmunmap(pagetable, TRAPFRAME, 1, 0);
  uvmfree(pagetable, sz);
}

// a user program that calls exec("/init")
// od -t xC initcode
uchar initcode[] = {
  0x17, 0x05, 0x00, 0x00, 0x13, 0x05, 0x45, 0x02,
  0x97, 0x05, 0x00, 0x00, 0x93, 0x85, 0x35, 0x02,
  0x93, 0x08, 0x70, 0x00, 0x73, 0x00, 0x00, 0x00,
  0x93, 0x08, 0x20, 0x00, 0x73, 0x00, 0x00, 0x00,
  0xef, 0xf0, 0x9f, 0xff, 0x2f, 0x69, 0x6e, 0x69,
  0x74, 0x00, 0x00, 0x24, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00
};

// Set up first user process.
void
userinit(void)
{
  struct proc *p;
  struct thread *t; //Task 3
  p = allocproc();
  t = allocthread(p); //Task 3
  initproc = p;

  
  // allocate one user page and copy init's instructions
  // and data into it.
  uvminit(p->pagetable, initcode, sizeof(initcode));
  p->sz = PGSIZE;

  // prepare for the very first "return" from kernel to user.
  p->trapframe->epc = 0;      // user program counter
  p->trapframe->sp = PGSIZE;  // user stack pointer

  safestrcpy(p->name, "initcode", sizeof(p->name));
  p->cwd = namei("/");

  p->state = RUNNABLE;
  t->state = T_RUNNABLE; //Task 3

  release(&p->lock);
}

// Grow or shrink user memory by n bytes.
// Return 0 on success, -1 on failure.
int
growproc(int n)
{
  uint sz;
  struct proc *p = myproc();

  sz = p->sz;
  if(n > 0){
    if((sz = uvmalloc(p->pagetable, sz, sz + n)) == 0) {
      return -1;
    }
  } else if(n < 0){
    sz = uvmdealloc(p->pagetable, sz, sz + n);
  }
  p->sz = sz;
  return 0;
}

// Create a new process, copying the parent.
// Sets up child kernel stack to return as if from fork() system call.
int
fork(void)
{
  int i, pid;
  struct proc *np;
  struct thread *nt; // child process's thread. 
  struct proc *p = myproc();
  struct thread *t = mythread(); 
  // Allocate process.
  if((np = allocproc()) == 0){
    return -1;
  }
  nt = allocthread(np); // allocate new thread to the child process. 

  // Copy user memory from parent to child.
  if(uvmcopy(p->pagetable, np->pagetable, p->sz) < 0){
    freeproc(np);
    release(&np->lock);
    return -1;
  }
  np->sz = p->sz;
  np->signalMask = p->signalMask; 
  for(i=0; i<32; i++){
    np->signalHandlers[i] = p->signalHandlers[i]; 
    np->sigMaskArray[i] = p->sigMaskArray[i]; 
  }
  


  // copy saved user registers.
  *(np->trapframe) = *(p->trapframe);

  // Cause fork to return 0 in the child.
  np->trapframe->a0 = 0;

  // increment reference counts on open file descriptors.
  for(i = 0; i < NOFILE; i++)
    if(p->ofile[i])
      np->ofile[i] = filedup(p->ofile[i]);
  np->cwd = idup(p->cwd);

  safestrcpy(np->name, p->name, sizeof(p->name));

  pid = np->pid;

  release(&np->lock);

  acquire(&wait_lock);
  np->parent = p;
  release(&wait_lock);

  acquire(&np->lock);
  np->state = RUNNABLE;
  acquire(&t->lock);
  nt->state = T_RUNNABLE; 
  release(&t->lock);
  release(&np->lock);

  return pid;
}

// Pass p's abandoned children to init.
// Caller must hold wait_lock.
void
reparent(struct proc *p)
{
  struct proc *pp;

  for(pp = proc; pp < &proc[NPROC]; pp++){
    if(pp->parent == p){
      pp->parent = initproc;
      wakeup(initproc);
    }
  }
}

void
exitThread(int status)
{
  struct proc *p = myproc();
  struct thread *curthread = mythread();
  struct thread *t;
  int runningThreads = 0; 

  for(t = p->thread; t < &(p->thread[NTHREAD]); t++){
    if (t != curthread && t->state != T_UNUSED && t->state != T_ZOMBIE) 
      runningThreads += 1;
  }

  if (runningThreads !=0 && p->state != ZOMBIE) {
    release(&p->lock); //TODO : make sure 
    exit(-1);
  }
  acquire(&p->lock);
  acquire(&curthread->lock);
  curthread->state = T_ZOMBIE; 
  curthread->xstate = status;
  release(&curthread->lock);
  release(&p->lock);

  for(t = p->thread; t < &(p->thread[NTHREAD]); t++){
    if(t->chan == curthread && t->state == T_SLEEPING) {
      t->state == T_RUNNABLE;
    }
  }
}

// Exit the current process.  Does not return.
// An exited process remains in the zombie state
// until its parent calls wait().
void
exit(int status)
{
  struct proc *p = myproc();
  struct thread * curthread = mythread();
  struct thread* t;

  if(p == initproc)
    panic("init exiting");

  // Close all open files.
  for(int fd = 0; fd < NOFILE; fd++){
    if(p->ofile[fd]){
      struct file *f = p->ofile[fd];
      fileclose(f);
      p->ofile[fd] = 0;
    }
  }

  begin_op();
  iput(p->cwd);
  end_op();
  p->cwd = 0;
  
  acquire(&p->lock);
  for(t = p->thread; t < &(p->thread[NTHREAD]); t++) {
    acquire(&t->lock);
    if(t != curthread){
      continue;
    }
    if(t->state != T_UNUSED){
      t->killed = 1;
    }
    release(&t->lock);
  }
  release(&p->lock);

  MakeSureOthersAreDead:

  // First we would like to wake up them so they will be able to kill themselves
  acquire(&p->lock);
  for(t = p->thread; t < &(p->thread[NTHREAD]); t++){
    acquire(&t->lock);
    if (t->state == T_SLEEPING) {
      t->state = T_RUNNABLE;
    }
    release(&t->lock);
  }
  release(&p->lock);

  acquire(&p->lock);
  for(t = p->thread; t < &(p->thread[NTHREAD]); t++){
    acquire(&t->lock);
    if(t == curthread)
      continue;
    if(t->state != T_ZOMBIE && t->state != T_UNUSED){
      release(&t->lock);
      goto MakeSureOthersAreDead;
    }
    else release(&t->lock);
  }
  release(&p->lock);

  acquire(&p->lock);


  acquire(&wait_lock);

  // Give any children to init.
  reparent(p);

  // Parent might be sleeping in wait().
  wakeup(p->parent);
  // TODO: Check if we need to pass abandoned children to init
  acquire(&p->lock);


  p->xstate = status;
  p->state = ZOMBIE;
  exitThread(status); 

  release(&wait_lock);

  // Jump into the scheduler, never to return.
  sched();
  panic("zombie exit");
}
// Wait for a child process to exit and return its pid.
// Return -1 if this process has no children.
int
wait(uint64 addr)
{
  struct proc *np;
  struct thread *t;
  int havekids, pid;
  struct proc *p = myproc();

  acquire(&wait_lock);

  for(;;){
    // Scan through table looking for exited children.
    havekids = 0;
    for(np = proc; np < &proc[NPROC]; np++){
      if(np->parent == p){
        // make sure the child isn't still in exit() or swtch().
        acquire(&np->lock);

        havekids = 1;
        if(np->state == ZOMBIE){
          // Found one.
          pid = np->pid;
          for(t = np->thread; t < &(np->thread[NTHREAD]); t++){
            acquire(&t->lock);
            if(t->state == T_ZOMBIE){
              //kfree(t->kstack);
              t->kstack = 0;
              t->state = T_UNUSED;
            }
            release(&t->lock);
          }
          if(addr != 0 && copyout(p->pagetable, addr, (char *)&np->xstate,
                                  sizeof(np->xstate)) < 0) {
            release(&np->lock);
            release(&wait_lock);
            return -1;
          }
          freeproc(np);
          release(&np->lock);
          release(&wait_lock);
          return pid;
        }
        release(&np->lock);
      }
    }

    // No point waiting if we don't have any children.
    if(!havekids || p->killed){
      release(&wait_lock);
      return -1;
    }
    
    // Wait for a child to exit.
    sleep(p, &wait_lock);  //DOC: wait-sleep
  }
}

// Per-CPU process scheduler.
// Each CPU calls scheduler() after setting itself up.
// Scheduler never returns.  It loops, doing:
//  - choose a process to run.
//  - swtch to start running that process.
//  - eventually that process transfers control
//    via swtch back to the scheduler.
void
scheduler(void)
{
  struct proc *p;
  struct thread *t; 

  struct cpu *c = mycpu();
  
  c->proc = 0;
  c->t = 0; 

  for(;;){
    // Avoid deadlock by ensuring that devices can interrupt.
    intr_on();

    for(p = proc; p < &proc[NPROC]; p++) {
      acquire(&p->lock);
      if(p->state == RUNNABLE) { //TODO : make sure we need to cheack also this condition
        // Switch to chosen process.  It is the process's job
        // to release its lock and then reacquire it
        // before jumping back to us.
        for(t = p->thread; t < &(p->thread[NTHREAD]); t++) {
          acquire(&t->lock);
          if(t->state == T_RUNNABLE){
            c->proc = p; // found runnable proc
            c->thread = t; // found runnable thread 
            t->state = T_RUNNING; 
            p->state = RUNNING;
            swtch(&c->context, &t->context); //TODO: check of this is the right way to switch between threads on cpu 
            // switch to another thread on the same procces 
            c->thread = 0;
          }
          release(&t->lock);
        }
        // Process is done running for now.
        // It should have changed its p->state before coming back.
        c->proc = 0;
      }
      release(&p->lock);
    }
  }
}

// Switch to scheduler.  Must hold only p->lock
// and have changed proc->state. Saves and restores
// intena because intena is a property of this
// kernel thread, not this CPU. It should
// be proc->intena and proc->noff, but that would
// break in the few places where a lock is held but
// there's no process.
void
sched(void)
{
  int intena;
  struct proc *p = myproc();
  struct threadNum *t = mythread();

  if(!holding(&p->lock))
    panic("sched p->lock");
  if(!holding(&t->lock))
    panic("sched t->lock");
  if(mycpu()->noff != 1)
    panic("sched locks");
  if(p->state == RUNNING)
    panic("sched running");
  if(t->state == RUNNING)
    panic("sched thread running");
  if(intr_get())
    panic("sched interruptible");

  intena = mycpu()->intena;
  swtch(&t->context, &mycpu()->context);
  mycpu()->intena = intena;
}


// Give up the CPU for one scheduling round.
void
yield(void)
{
  struct proc *p = myproc();
  struct thread *t = mythread();
  acquire(&p->lock);
  acquire(&t->lock);
  p->state = RUNNABLE;
  t->state = T_RUNNABLE;
  sched();
  acquire(&t->lock);
  release(&p->lock);
}

// A fork child's very first scheduling by scheduler()
// will swtch to forkret.
void
forkret(void)
{
  static int first = 1;

  // Still holding p->lock from scheduler.
  release(&mythread()->lock);
  //  release(&myproc()->lock);
  if (first) {
    // File system initialization must be run in the context of a
    // regular process (e.g., because it calls sleep), and thus cannot
    // be run from main().
    first = 0;
    fsinit(ROOTDEV);
  }
  usertrapret();
}

// Atomically release lock and sleep on chan.
// Reacquires lock when awakened.
void
sleep(void *chan, struct spinlock *lk)
{
  struct proc *p = myproc();
  struct thread *t = mythread(); 
  
  if(p == 0 || t ==0 )
    panic("sleep");
  // Must acquire p->lock in order to
  // change p->state and then call sched.
  // Once we hold p->lock, we can be
  // guaranteed that we won't miss any wakeup
  // (wakeup locks p->lock),
  // so it's okay to release lk.

  acquire(&t->lock); 
  release(lk);

  // Thread go to sleep.
  t->chan = chan;
  t->state = T_SLEEPING;

  sched();

  // Tidy up.
  t->chan = 0;

  // Reacquire original lock.
  release(&t->lock);
  acquire(lk);
}

// Wake up all processes sleeping on chan.
// Must be called without any p->lock.
void
wakeup(void *chan)
{
  struct proc *p;
  struct threadNum *t; 

  for(p = proc; p < &proc[NPROC]; p++) {
    if(p != myproc()){
      acquire(&p->lock);
      for(t = p->thread; t < &(p->thread[NTHREAD]); t++) {
        if(t != mythread()){
          acquire(&t->lock);
          if(t->state == T_SLEEPING && t->chan == chan) {
              t->state = T_RUNNABLE;
            }
          release(&t->lock);
        }
      }
      release(&p->lock);
    }
  }
}

// Kill the process with the given pid.
// The victim won't exit until it tries to return
// to user space (see usertrap() in trap.c).
int
kill(int pid, int signum)
{
  struct proc *p;
  struct thread *t; // NEW
  if(signum < 0 || signum >= 32)
    return -1;

  for(p = proc; p < &proc[NPROC]; p++){
    acquire(&p->lock);
      if(p->pid == pid){
        if(signum == SIGKILL){
          p->killed = 1;
          if(p->state == SLEEPING){
            // Wake process from sleep().
            p->state = RUNNABLE;
          }
          for(t = p->thread; t < &(p->thread[NTHREAD]); t++){
            acquire(&t->lock);
            t->killed = 1;
            // Wake process from sleep if necessary.
            if (t->state == T_SLEEPING)
              t->state = T_RUNNABLE;
            release(&t->lock);
          }
        }
        else{
          p->pendingSignals = (p->pendingSignals | 1<<signum); 
        }
        release(&p->lock);
        return 0;
      }
    release(&p->lock);
  }
  return -1;
}

// Copy to either a user address, or kernel address,
// depending on usr_dst.
// Returns 0 on success, -1 on error.
int
either_copyout(int user_dst, uint64 dst, void *src, uint64 len)
{
  struct proc *p = myproc();
  if(user_dst){
    return copyout(p->pagetable, dst, src, len);
  } else {
    memmove((char *)dst, src, len);
    return 0;
  }
}

// Copy from either a user address, or kernel address,
// depending on usr_src.
// Returns 0 on success, -1 on error.
int
either_copyin(void *dst, int user_src, uint64 src, uint64 len)
{
  struct proc *p = myproc();
  if(user_src){
    return copyin(p->pagetable, dst, src, len);
  } else {
    memmove(dst, (char*)src, len);
    return 0;
  }
}

// Print a process listing to console.  For debugging.
// Runs when user types ^P on console.
// No lock to avoid wedging a stuck machine further.
void
procdump(void)
{
  static char *states[] = {
  [UNUSED]    "unused",
  [EMBRYO]    "embryo",
  [SLEEPING]  "sleep ",
  [RUNNABLE]  "runble",
  [RUNNING]   "run",
  [TZOMBIE]   "tzombie", 
  [INVAILD]   "invalid"
  };
  struct proc *p;
  struct thread *t; 
  char *state;

  printf("\n");
  for(p = proc; p < &proc[NPROC]; p++){
    if(p->state == UNUSED)
      continue;
    for(t = p->thread; t < &(p->thread[NTHREAD]); t++){
      if(t->state >= 0 && t->state < NELEM(states) && states[t->state])
        state = states[t->state];
      else
        state = "???";
      printf("pid:%d tid:%d %s %s", p->pid, t->tid, state, p->name);
      printf("\n");
    }
  }
}

uint
sigprocmask(uint sigmask) {
  struct proc *p = myproc();
  uint oldMask = p->signalMask; 
  p->signalMask = sigmask; 
  return oldMask; 
  // This will update the process signal mask, the return value should be the old mask
}

int sigaction (int signum, const struct sigaction *act, struct sigaction *oldact){
  if (signum == SIGKILL|| signum == SIGSTOP || signum < 0 || signum >=32)
    return -1; 

  struct proc *p = myproc();
  struct sigaction tempAct; 

  if (copyin(p->pagetable, (char*)&tempAct, (uint64)act, sizeof(struct sigaction)) != 0){
    return -1; 
  }
  if(oldact != null){
    if(copyout(p->pagetable, (uint64)&oldact->sa_handler, (char*)&p->signalHandlers[signum],sizeof(struct sigaction))!= 0)
      return -1; 
    if(copyout(p->pagetable, (uint64)&oldact->sigmask, (char*)&p->sigMaskArray[signum],sizeof(struct sigaction))!= 0)
      return -1; 
  }
  p->signalHandlers[signum] = tempAct.sa_handler; 
  p->sigMaskArray[signum] = tempAct.sigmask; 

  return 0; 
}

void sigret (void){
  struct proc *p;
  p = myproc(); 
  if (p!=0){
    memmove(p->trapframe, p->backupTrapframe,sizeof(struct trapframe));
    p->signalMask=p->backupSigMask;
  }
}

void sigkillHandler(void){
  myproc()->killed = 1; 
}

void sigstopHandler(void){
  struct proc *p = myproc(); 
  p->frozen = 1;
  while(p->frozen){
    if((p->pendingSignals&(1<<SIGKILL)) !=0 ){
      p->killed = 1;
      return;
    }
    yield();
  }
}

void sigcontHandler(void){
  if (myproc()->frozen == 1){
    myproc()->frozen = 0;
  }
}

// for each signal check if can be handled in the kernel space or need to be handle in the user space
void signalHandler(void){
  struct proc *p = myproc(); 
  if(p!=0){
    if(p->sigHandlerFlag == 1) //there's a signal that being handling now
      return;
    for(int i=0;i<32;++i){
      if((1<<i&p->pendingSignals) && !((1<<i)&p->signalMask)){
        if(p->signalHandlers[i] == (void *)SIG_IGN){continue;}
        if(p->signalHandlers[i] ==(void *)SIG_DFL){ //kernel space handler
            switch(i){
                case SIGKILL:
                    sigkillHandler();
                    break;
                case SIGSTOP:
                    sigstopHandler();
                    break;
                case SIGCONT:
                    sigcontHandler();  
                    break;
                default:
                    sigkillHandler();
                    break;
            }
            p->pendingSignals = p->pendingSignals & (~ (1 << i)); // ~ is Bitwise complement, we remove the signem from the pending signals
        }
        else { //user space handler
          p->backupSigMask= p->signalMask;
          userSpaceHandler(p,i);
        }
      }
    }
  }
  return;
}

void userSpaceHandler(struct proc *p, int signum) {
  int sigret_size = 0; 
  // the process is at "signal handling" -> turn on a flag.
  if (p->sigHandlerFlag == 0)
    p->sigHandlerFlag=1;
  else
    return;
  // // memmove(p->trapframe,p->trapframe-(sizeof(struct trapframe))),(sizeof(struct trapframe)))
  // memmove(p->backupTrapframe,(p->trapframe-(sizeof(struct trapframe))),(sizeof(struct trapframe)));
  // // memmove(p->backupTrapframe,p->trapframe,(sizeof(struct trapframe)));
  acquire(&p->lock); 
  // back up the process "general" signal mask, and set the current process signal mask to be the signal handler mask.
  memmove(p->backupTrapframe,p->trapframe,sizeof(struct trapframe));

  // the process trapframe stack pointer - the size of the trapframe
  sigret_size = sigret_func_end - sigret_func_start; 

  p->trapframe->sp -= sigret_size;

  //  "copyout" (from kernel to user) this function, to the process trapframe stack pointer
  if(copyout(p->pagetable, p->trapframe->sp, (char*)&sigret_func_start, sigret_size) != 0){
    printf("faild in copyout \n"); 
  }
  
  // save the return address of the tack pointer
  p->trapframe->ra = p->trapframe->sp;

  p->trapframe->a0 = signum;

  // change mask : save the current mask as backup
  p->backupSigMask = p->signalMask;

  // save new sigmask in signalMask
  p->signalMask = p->sigMaskArray[signum]; 

  // return to the calling function
  p->trapframe->epc = (uint64)p->signalHandlers[signum]; 
  p->pendingSignals = p->pendingSignals & (~ (1 << signum)); // ~ is Bitwise complement, we remove the signem from the pending signals
  release(&p->lock); 
}

// Upon success, this function returns the caller thread’s id. In case of error, a negative error code is returned
int 
kthread_id(void)
{
  struct thread *t = mythread();
  if(t == 0)
    return -1; 

  return t->tid; 
}

// Calling kthread_create will create a new thread within the context of the calling process 
// int 
// kthread_create (void ( *start_func ) ( ) , void *stack) 
// {

// }

// // This function terminates the execution of the calling thread 
// void 
// kthread_exit(int status)
// {

// }

// // This function suspends the execution of the calling thread until the target thread, indicated by the argument thread id, terminates
// int 
// kthread_join(int thread_id, int* status)
// {

// }
