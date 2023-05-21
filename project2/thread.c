#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "x86.h"
#include "proc.h"
#include "spinlock.h"
// #include "user.h"

#define NTHREAD 8

extern struct ptable_t ptable;
extern void wakeup1(void *chan);

int thread_create(thread_t*, void*(*)(void*), void*);
int thread_join(thread_t, void**);
int thread_start(struct proc*); 
void thread_exit(void*);

int
thread_create(thread_t *thread, void *(*start_routine)(void *), void *arg)
{

  // Allocate a new `proc` structure for the LWP
  struct proc* lwp;
  struct proc* mother = myproc();

  if ((lwp = allocproc()) == 0) {
    return -1; // Failed to allocate a new LWP
  }

  // Share the same address space as the parent process
  lwp->pgdir = mother->pgdir;
  lwp->mother = mother;
  lwp->parent = mother->parent;

  if ((lwp->sz = mother->sz = allocuvm(lwp->pgdir, mother->sz, mother->sz + 2*PGSIZE)) == 0) {
    goto bad;
  }

  // Build stack guard
  clearpteu(lwp->pgdir, (char*)(lwp->sz - 2*PGSIZE));

  cprintf("is this okay???? pid: %d, func: %d\n", lwp->pid, start_routine);

  // Set up the LWP's stack
  uint sp = lwp->sz;
  uint ustack[2];
  ustack[0] = (uint)arg;
  ustack[1] = 0; // Placeholder for the return address

  sp -= 2*sizeof(uint);

  // Allocate and copy the user stack
  if (copyout(lwp->pgdir, sp, ustack, 2*sizeof(uint)) < 0) {
    goto bad;
  }

  // Initialize the thread's trapframe
  *lwp->tf = *mother->tf;
  lwp->tf->esp = sp;
  lwp->tf->eip = (uint)start_routine;

  // Set the thread ID
  *thread = lwp->tid = ++(mother->thread_num);

  // Add the LWP to the thread management data structure (e.g., array)
  mother->threads[lwp->tid] = lwp;

  // Mark the LWP as runnable
  // lwp->state = RUNNABLE;

  acquire(&ptable.lock);
  lwp->state = RUNNABLE;
  release(&ptable.lock);


  return 0; // Thread creation successful

bad:
  kfree(lwp->kstack);
  lwp->kstack = 0;
  lwp->state = UNUSED;
  return -1;
}

int thread_join(thread_t thread, void** retval) {
  struct proc* p = myproc();

  if (thread >= NTHREAD || p->threads[thread] == 0) {
    return -1; // Invalid thread ID or thread does not exist
  }

  struct proc* lwp = p->threads[thread];

  acquire(&ptable.lock);

  // Wait for the LWP to complete
  while (lwp->state != ZOMBIE) {
    sleep(lwp, &ptable.lock);
  }

  // Retrieve the LWP's return value
  if (retval != 0) {
    *retval = lwp->retval;
  }

  // Clean up the LWP
  kfree(lwp->kstack);
  lwp->kstack = 0;
  lwp->pid = 0;
  lwp->parent = 0;
  lwp->name[0] = 0;
  lwp->killed = 0;
  lwp->state = UNUSED;

  release(&ptable.lock);

  return 0; // Thread join successful
}

void thread_exit(void* retval) {
  struct proc* p = myproc();
  struct proc* parent = p->parent;

  // Set the return value in the current LWP's proc structure
  p->retval = retval;

  acquire(&ptable.lock);

  // Wake up the parent thread waiting in thread_join
  wakeup1(parent);

  // Clean up the LWP
  kfree(p->kstack);
  p->kstack = 0;
  p->pid = 0;
  p->parent = 0;
  p->name[0] = 0;
  p->killed = 0;
  p->state = ZOMBIE;

  // Release the process table lock before jumping into the scheduler
  release(&ptable.lock);

  // Jump into the scheduler, never to return
  sched();

  // Note: The execution will never reach this point
}