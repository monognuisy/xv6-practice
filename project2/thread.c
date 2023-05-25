#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "x86.h"
#include "proc.h"
#include "spinlock.h"

extern struct ptable_t ptable;
extern struct proc *initproc;
extern void wakeup1(void *chan);

int thread_create(thread_t*, void*(*)(void*), void*);
int thread_join(thread_t, void**);
void thread_exit(void*);
void _cleanup(struct proc*);

// Creates and update thread to tid(thread's id) having start_routine with arg.
// Returns 0 for success, -1 for failure.
int
thread_create(thread_t *thread, void *(*start_routine)(void *), void *arg)
{
  struct proc* lwp;                     // current(will be created) thread
  struct proc* mother = myproc();       // process that called thread_create

  // Allocate lwp and initiailize it
  if ((lwp = allocproc()) == 0) {
    return -1; // Failed to allocate a new LWP
  }
  lwp->stackpage = 1;
  lwp->isthread = 1;

  // Share the same address space and parent process as the mother process
  lwp->pgdir = mother->pgdir;
  lwp->mother = mother;
  lwp->parent = mother->parent;

  // Allocate pages for lwp and update its mother's size as well
  if ((lwp->sz = mother->sz = allocuvm(lwp->pgdir, mother->sz, mother->sz + 2*PGSIZE)) == 0) {
    goto bad;
  }

  // Build stack guard
  clearpteu(lwp->pgdir, (char*)(lwp->sz - 2*PGSIZE));

  // Set up the LWP's stack
  uint sp = lwp->sz;
  uint ustack[2];
  ustack[1] = (uint)arg;
  ustack[0] = 0xffffffff; // Placeholder for the return address

  sp -= 2*sizeof(uint);

  // Allocate and copy the user stack
  if (copyout(lwp->pgdir, sp, ustack, 2*sizeof(uint)) < 0) {
    goto bad;
  }

  // Initialize the thread's trapframe
  *lwp->tf = *mother->tf;
  lwp->tf->esp = sp;
  lwp->tf->eip = (uint)start_routine;
  lwp->tf->eax = 0;

  // Copy file descriptor from mother
  for(int i=0;i<NOFILE;i++)
    if(mother->ofile[i])
      lwp->ofile[i]=filedup(mother->ofile[i]);
  lwp->cwd=idup(mother->cwd);

  // Copy name of mother process to thread
  safestrcpy(lwp->name,mother->name,sizeof(mother->name));

  // Set the thread ID
  *thread = lwp->tid = ++(mother->thread_num);

  // Add the LWP to the mother's thread array
  mother->threads[lwp->tid] = lwp;

  // Set sibling thread's size equal as current thread(lwp)
  for (int i = 1; i < NTHREAD; i++) {
    if (mother->threads[i]) {
      mother->threads[i]->sz = lwp->sz;
    }
  }

  // Mark the LWP as runnable
  acquire(&ptable.lock);
  lcr3(V2P(lwp->pgdir));  // switch to process's address space
  lwp->state = RUNNABLE;
  release(&ptable.lock);

  return 0; // Thread creation successful

bad:
  kfree(lwp->kstack);
  lwp->kstack = 0;
  lwp->state = UNUSED;
  return -1;
}

// Join given thread and lend retval from exited thread.
// Returns 0 for success, -1 for failure.
int 
thread_join(thread_t thread, void** retval) 
{
  // Process that will wait for its threads
  struct proc* curproc = myproc();

  acquire(&ptable.lock);
  if (thread >= NTHREAD || curproc->threads[thread] == 0) {
    release(&ptable.lock);
    return -1; // Invalid thread ID or thread does not exist
  }

  struct proc* lwp = curproc->threads[thread];

  // Wait for the LWP to complete
  while (lwp->state != ZOMBIE) {
    sleep(curproc, &ptable.lock);
  }

  *retval = lwp->retval;

  // Clean up the LWP
  lwp->mother->threads[lwp->tid] = 0;
  _cleanup(lwp);


  release(&ptable.lock);
  return 0; // Thread join successful
}


// Exit thread and set return value of thread to retval.
// Never returns.
void thread_exit(void* retval) {
  struct proc* lwp = myproc();
  struct proc* mother = lwp->mother;
  int fd;

  // Close all open files.
  for(fd = 0; fd < NOFILE; fd++){
    if(lwp->ofile[fd]){
      fileclose(lwp->ofile[fd]);
      lwp->ofile[fd] = 0;
    }
  }

  // Release lwp's working directory
  begin_op();
  iput(lwp->cwd);
  end_op();
  lwp->cwd = 0;

  acquire(&ptable.lock);

  lwp->retval = retval;         // Set the return value in the current lwp
  lwp->mother->thread_num--;    // Decrease number of thread of mother proc

  // Wake up the mother proc waiting in thread_join
  wakeup1(mother);

  // Adopt abandoned processes
  for(struct proc *p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->parent == lwp){
      p->parent = initproc;
      if(p->state == ZOMBIE)
        wakeup1(initproc);
    }
  }

  lwp->state = ZOMBIE;

  // Keep holding ptable.lock before jumping into the scheduler
  // Jump into the scheduler, never to return
  sched();
  panic("zombie exit");
}

// Cleanup function for internal use. 
// Only works when victim != myproc()
void
_cleanup(struct proc *victim)
{
  if (!victim) return;

  if (victim->kstack)
    kfree(victim->kstack);
  victim->kstack = 0;
  victim->pid = 0;
  victim->parent = 0;
  victim->mother = 0;
  victim->name[0] = 0;
  victim->killed = 0;
  victim->tf = 0;
  victim->sz = 0;
  victim->state = UNUSED;
}

// Clean all sibling thread except self. 
// If there's mother thread, cleanup as well.
int
clean_thread(struct proc* curproc)
{
  acquire(&ptable.lock);
  struct proc *mother;
  struct proc *sibling;

  mother = (curproc->isthread) ? curproc->mother : curproc;

  // Cleanup siblings
  for (int i = 1; i < NTHREAD; i++) {
    sibling = mother->threads[i];

    if (mother->thread_num == 0)
      break;          // No more needs for cleaning up thread

    if (!sibling) 
      continue;

    if (sibling == curproc) {
      mother->thread_num--;    // Decrese threadnum even for current thread
      continue;
    }

    // found scapegoat sibling
    _cleanup(sibling);
    mother->threads[i] = 0;
    mother->thread_num--;
  }

  // If current process is not mother process, then cleanup mother as well
  if (curproc != mother) {
    curproc->parent = mother->parent;
    curproc->mother = 0;
    _cleanup(mother);
  }

  release(&ptable.lock);
  return 0;
}