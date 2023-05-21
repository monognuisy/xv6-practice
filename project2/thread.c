#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "x86.h"
#include "proc.h"
#include "spinlock.h"
// #include "user.h"

extern struct ptable_t ptable;

int thread_create(thread_t*, void*(*)(void*), void*);
int thread_join(thread_t*, void**);
int thread_start(struct proc*); 
void thread_exit(void*);

int
thread_create(thread_t *thread, void *(*start_routine)(void *), void *arg)
{
  struct proc *mother = myproc();
  struct proc *son;

  // cprintf("%d\n", mother->tf->eip);
  
  // parent


  if ((son = allocproc()) == 0) {
    return -1;
  }


  son->pgdir = mother->pgdir;

  // child (thread)
  uint sp, ustack[4];
  uint argc = 0;

  son->isthread = 1;
  son->mother = mother;
  son->parent = mother->parent;
  *son->tf = *mother->tf;
  son->tf->eax = 0;

  // set thread's id to proc's next thread number
  *thread = son->tid = ++(mother->thread_num);

  uint sz = mother->sz;

  // increase mother's, son's size
  if ((son->sz = mother->sz = 
        allocuvm(son->pgdir, sz, sz + 2*PGSIZE)) == 0) 
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
  son->tf->eip = (uint)start_routine;
  son->tf->esp = sp;

  cprintf("mother eip: %p, son eip: %p\n", mother->tf->eip, son->tf->eip);

  // admit son as thread of mother proc
  mother->threads[son->tid] = son;

  // update siblings' size
  struct proc **th;
  for (th = mother->threads; th < &mother->threads[NPROC]; th++) {
    if (!*th) continue;
    if (!(*th)->isthread) continue;
    
    (*th)->sz = son->sz;
  }

  int i;
  for(i = 0; i < NOFILE; i++)
    if(mother->ofile[i])
      son->ofile[i] = filedup(mother->ofile[i]);
  son->cwd = idup(mother->cwd);

  safestrcpy(son->name, mother->name, sizeof(mother->name));

  cprintf("mother pid: %d\nson pid: %d\n", mother->pid, son->pid);

  acquire(&ptable.lock);

  switchuvm(son);

  son->state = RUNNABLE;
  release(&ptable.lock);


  return 0;

bad:
  cprintf("bad thread!\n");
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

  // switchuvm(p_thread);  
  pushcli();
  lcr3(V2P(p_thread->pgdir));
  popcli(); 
  p_thread->state = RUNNABLE;

  return 0;
}