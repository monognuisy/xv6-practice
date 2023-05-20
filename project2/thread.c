#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "x86.h"
#include "proc.h"
#include "spinlock.h"
#include "user.h"

extern struct ptable_t ptable;

int thread_create(thread_t*, void*(*)(void*), void*);
int thread_join(thread_t*, void**);
int thread_start(struct proc*); 
void thread_exit(void*);

int
thread_create(thread_t *thread, void *(*start_routine)(void *), void *arg)
{
  struct proc *mother = myproc();
  int pid;
  int ppid = mother->pid;
  
  // parent
  if ((pid = fork())) {
    // parent of child will be changed
    // so, return without wait()
    return 0;
  }

  acquire(&ptable.lock);

  // child (thread)
  struct proc *son = myproc();
  uint sp, ustack[4];
  uint argc = 0;

  son->isthread = 1;
  son->mother = mother;
  son->pgdir = mother->pgdir;
  son->parent = mother->parent;

  // set thread's id to proc's next thread number
  *thread = son->tid = ++(mother->thread_num);

  // increase mother's, son's size
  if ((son->sz = mother->sz = 
        allocuvm(mother->pgdir, mother->sz, mother->sz + 2*PGSIZE)) == 0) 
    goto bad;

  // Build stack guard
  clearpteu(son->pgdir, (char*)(son->sz - 2*PGSIZE));
  sp = son->sz; 

  ustack[3] = 0;                    // end of user stack
  ustack[2] = sp - (argc + 1)*4;    // argv pointer
  ustack[1] = argc;
  ustack[0] = 0xffffffff;           // fake return PC

  sp -= (3+argc+1) * 4;

  if(copyout(son->pgdir, sp, ustack, (3+argc+1)*4) < 0)
    goto bad;

  // change entry point of thread to start_routine
  son->tf->eip = start_routine;
  son->tf->esp = sp;

  // admit son as thread of mother proc
  mother->threads[son->tid] = son;

  // update siblings' size
  struct proc **th;
  for (th = mother->threads; th < &mother->threads[NPROC]; th++) {
    if (!*th) continue;
    if (!(*th)->isthread) continue;
    
    (*th)->sz = son->sz;
  }

  if (thread_start(son) < 0)
    goto bad;

  release(&ptable.lock);
  
bad:
  release(&ptable.lock);
  son->isthread = 0;
  mother->threads[son->tid] = 0;
  kill(son->pid);
  return -1;
}

int
thread_start(struct proc* p_thread)
{
  if (p_thread->isthread) return -1;

  switchuvm(p_thread);  
  p_thread->state = RUNNABLE;

  return 0;
}