#include "klib.h"
#include "cte.h"
#include "proc.h"

#define PROC_NUM 64

static __attribute__((used)) int next_pid = 1;

proc_t pcb[PROC_NUM];
static proc_t *curr = &pcb[0];

void init_proc()
{
  // Lab2-1, set status and pgdir
  curr->status = RUNNING;
  curr->pgdir = vm_curr();
  curr->kstack = (void *)(KER_MEM - PGSIZE);
  // Lab2-4, init zombie_sem
  sem_init(&(curr->zombie_sem), 0);
  curr->cwd = iopen("/", TYPE_NONE);
  // Lab3-2, set cwd
}

proc_t *proc_alloc()
{
  // Lab2-1: find a unused pcb from pcb[1..PROC_NUM-1], return NULL if no such one
  for (int i = 1; i < PROC_NUM; i++)
    if (pcb[i].status == UNUSED)
    {
      pcb[i].pid = next_pid;
      next_pid++;
      pcb[i].status = UNINIT;
      pcb[i].pgdir = vm_alloc();
      pcb[i].brk = 0;
      pcb[i].kstack = kalloc();
      pcb[i].ctx = &(pcb[i].kstack->ctx);
      pcb[i].child_num = 0;
      pcb[i].parent = NULL;
      sem_init(&pcb[i].zombie_sem, 0);
      for (int j = 0; j < MAX_USEM; j++)
        pcb[i].usems[j] = NULL;
      for (int j = 0; j < MAX_UFILE; j++)
        pcb[i].files[j] = NULL;
      pcb[i].cwd = NULL;
      return &pcb[i];
    }
  return NULL;
  // TODO();
  // init ALL attributes of the pcb
}

void proc_free(proc_t *proc)
{
  // Lab2-1: free proc's pgdir and kstack and mark it UNUSED
  vm_teardown(proc->pgdir);
  kfree(proc->kstack);
  proc->status = UNUSED;
  // TODO();
}

proc_t *proc_curr()
{
  return curr;
}

void proc_run(proc_t *proc)
{
  proc->status = RUNNING;
  curr = proc;
  set_cr3(proc->pgdir);
  set_tss(KSEL(SEG_KDATA), (uint32_t)STACK_TOP(proc->kstack));
  irq_iret(proc->ctx);
}

void proc_addready(proc_t *proc)
{
  // Lab2-1: mark proc READY
  proc->status = READY;
}

void proc_yield()
{
  // Lab2-1: mark curr proc READY, then int $0x81
  curr->status = READY;
  INT(0x81);
}

void proc_copycurr(proc_t *proc)
{
  // Lab2-2: copy curr proc
  proc_t *now_proc = proc_curr();
  vm_copycurr(proc->pgdir);
  proc->brk = now_proc->brk;
  proc->kstack->ctx = now_proc->kstack->ctx;
  proc->ctx = &(proc->kstack->ctx);
  proc->ctx->eax = 0;
  proc->parent = now_proc;
  now_proc->child_num++;
  for (int i = 0; i < MAX_USEM; i++)
  {
    proc->usems[i] = now_proc->usems[i];
    if (now_proc->usems[i] != NULL)
      usem_dup(now_proc->usems[i]);
  }
  for (int i = 0; i < MAX_UFILE; i++)
  {
    proc->files[i] = now_proc->files[i];
    if (now_proc->files[i] != NULL)
      fdup(now_proc->files[i]);
  }
  proc->cwd = idup(now_proc->cwd);
  // Lab2-5: dup opened usems
  // Lab3-1: dup opened files
  // Lab3-2: dup cwd
  // TODO();
}

void proc_makezombie(proc_t *proc, int exitcode)
{
  // Lab2-3: mark proc ZOMBIE and record exitcode, set children's parent to NULL
  proc->status = ZOMBIE;
  proc->exit_code = exitcode;
  for (int i = 1; i < PROC_NUM; i++)
    if (pcb[i].parent == proc)
      pcb[i].parent = NULL;
  if (proc->parent != NULL)
    sem_v(&(proc->parent->zombie_sem));
  for (int i = 0; i < MAX_USEM; i++)
    if (proc->usems[i] != NULL)
      usem_close(proc->usems[i]);
  for (int i = 0; i < MAX_UFILE; i++)
    if (proc->files[i] != NULL)
      fclose(proc->files[i]);
  iclose(proc->cwd);
  // Lab2-5: close opened usem
  // Lab3-1: close opened files
  // Lab3-2: close cwd
  // TODO();
}

proc_t *proc_findzombie(proc_t *proc)
{
  // Lab2-3: find a ZOMBIE whose parent is proc, return NULL if none
  for (int i = 1; i < PROC_NUM; i++)
    if (pcb[i].parent == proc && pcb[i].status == ZOMBIE)
      return &pcb[i];
  return NULL;
  // TODO();
}

void proc_block()
{
  // Lab2-4: mark curr proc BLOCKED, then int $0x81
  curr->status = BLOCKED;
  INT(0x81);
}

int proc_allocusem(proc_t *proc)
{
  // Lab2-5: find a free slot in proc->usems, return its index, or -1 if none
  for (int i = 0; i < MAX_USEM; i++)
    if (proc->usems[i] == NULL)
      return i;
  return -1;
  // TODO();
}

usem_t *proc_getusem(proc_t *proc, int sem_id)
{
  // Lab2-5: return proc->usems[sem_id], or NULL if sem_id out of bound
  if (sem_id < 0 || sem_id > 31)
    return NULL;
  return proc->usems[sem_id];
  // TODO();
}

int proc_allocfile(proc_t *proc)
{
  // Lab3-1: find a free slot in proc->files, return its index, or -1 if none
  for (int i = 0; i < MAX_UFILE; i++)
    if (proc->files[i] == NULL)
      return i;
  return -1;
  // TODO();
}

file_t *proc_getfile(proc_t *proc, int fd)
{
  // Lab3-1: return proc->files[fd], or NULL if fd out of bound
  if (fd >= MAX_UFILE)
    return NULL;
  return proc->files[fd];
  // TODO();
}

void schedule(Context *ctx)
{
  // Lab2-1: save ctx to curr->ctx, then find a READY proc and run it
  proc_curr()->ctx = ctx;
  int i;
  for (i = 0; i < PROC_NUM; i++)
    if (proc_curr() == &pcb[i])
      break;
  for (int j = 1; j <= PROC_NUM; j++)
    if (pcb[(i + j) % PROC_NUM].status == READY)
      proc_run(&pcb[(i + j) % PROC_NUM]);
  // TODO();
}
