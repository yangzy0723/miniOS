#include "klib.h"
#include "cte.h"
#include "sysnum.h"
#include "vme.h"
#include "serial.h"
#include "loader.h"
#include "proc.h"
#include "timer.h"
#include "file.h"

typedef int (*syshandle_t)(uint32_t, uint32_t, uint32_t, uint32_t, uint32_t);

extern void *syscall_handle[NR_SYS];

void do_syscall(Context *ctx)
{
  // TODO: Lab1-5 call specific syscall handle and set ctx register
  int sysnum = 0;
  uint32_t arg1 = 0;
  uint32_t arg2 = 0;
  uint32_t arg3 = 0;
  uint32_t arg4 = 0;
  uint32_t arg5 = 0;
  int res;
  sysnum = ctx->eax;
  arg1 = ctx->ebx;
  arg2 = ctx->ecx;
  arg3 = ctx->edx;
  arg4 = ctx->esi;
  arg5 = ctx->edi;
  if (sysnum < 0 || sysnum >= NR_SYS)
    res = -1;
  else
    res = ((syshandle_t)(syscall_handle[sysnum]))(arg1, arg2, arg3, arg4, arg5);
  ctx->eax = res;
}

int sys_write(int fd, const void *buf, size_t count)
{
  // TODO: rewrite me at Lab3-1
  return serial_write(buf, count);
}

int sys_read(int fd, void *buf, size_t count)
{
  // TODO: rewrite me at Lab3-1
  return serial_read(buf, count);
}

int sys_brk(void *addr)
{
  // TODO: Lab1-5
  proc_t *now_proc_t = proc_curr(); // use brk of proc instead of this in Lab2-1
  size_t new_brk = PAGE_UP(addr);   // brk和new_brk是天生已经对齐的
  if (now_proc_t->brk == 0)
    now_proc_t->brk = new_brk;
  else if (new_brk > now_proc_t->brk)
  {
    // printf("new_brk is biger: new_brk %p, brk %p\n", new_brk, brk);
    vm_map(vm_curr(), now_proc_t->brk, new_brk - now_proc_t->brk, 0x7);
    now_proc_t->brk = new_brk;
  }
  else if (new_brk < now_proc_t->brk)
  {
    vm_unmap(vm_curr(), new_brk, now_proc_t->brk - new_brk);
    now_proc_t->brk = new_brk;
  }
  return 0;
}

void sys_sleep(int ticks)
{
  // TODO(); // Lab1-7
  uint32_t time = get_tick();
  while (get_tick() - time < ticks)
  {
    proc_yield();
  }
}

int sys_exec(const char *path, char *const argv[])
{
  PD *pgdir = vm_alloc();
  Context ctx;
  int ret = load_user(pgdir, &ctx, path, argv);
  if (ret != 0)
  {
    vm_teardown(pgdir);
    return -1;
  }
  PD *old_dir = vm_curr();
  set_cr3(pgdir);
  proc_curr()->pgdir = pgdir;
  vm_teardown(old_dir);
  irq_iret(&ctx);
  // TODO(); // Lab1-8, Lab2-1
}

int sys_getpid()
{
  return proc_curr()->pid; // Lab2-1
}

void sys_yield()
{
  proc_yield();
}

int sys_fork()
{
  proc_t *new_proc = proc_alloc();
  if (new_proc == NULL)
    return -1;
  proc_copycurr(new_proc);
  proc_addready(new_proc);
  return new_proc->pid;

  // TODO(); // Lab2-2
}

void sys_exit(int status)
{
  // while (1)
  //   proc_yield();
  proc_makezombie(proc_curr(), status);
  INT(0X81);
  // Lab2-3
}

int sys_wait(int *status)
{
  // sys_sleep(250);
  // return 0;
  proc_t *now_proc = proc_curr();
  if (now_proc->child_num == 0)
    // Lab2-3, Lab2-4
    return -1;
  proc_t *child_zombie;
  while ((child_zombie = proc_findzombie(now_proc)) == NULL)
  {
    proc_yield();
  }

  if (status != NULL)
  {
    *status = child_zombie->exit_code;
  }
  proc_free(child_zombie);
  now_proc->child_num--;
  return child_zombie->pid;
}

int sys_sem_open(int value)
{
  TODO(); // Lab2-5
}

int sys_sem_p(int sem_id)
{
  TODO(); // Lab2-5
}

int sys_sem_v(int sem_id)
{
  TODO(); // Lab2-5
}

int sys_sem_close(int sem_id)
{
  TODO(); // Lab2-5
}

int sys_open(const char *path, int mode)
{
  TODO(); // Lab3-1
}

int sys_close(int fd)
{
  TODO(); // Lab3-1
}

int sys_dup(int fd)
{
  TODO(); // Lab3-1
}

uint32_t sys_lseek(int fd, uint32_t off, int whence)
{
  TODO(); // Lab3-1
}

int sys_fstat(int fd, struct stat *st)
{
  TODO(); // Lab3-1
}

int sys_chdir(const char *path)
{
  TODO(); // Lab3-2
}

int sys_unlink(const char *path)
{
  return iremove(path);
}

// optional syscall

void *sys_mmap()
{
  TODO();
}

void sys_munmap(void *addr)
{
  TODO();
}

int sys_clone(void (*entry)(void *), void *stack, void *arg)
{
  TODO();
}

int sys_kill(int pid)
{
  TODO();
}

int sys_cv_open()
{
  TODO();
}

int sys_cv_wait(int cv_id, int sem_id)
{
  TODO();
}

int sys_cv_sig(int cv_id)
{
  TODO();
}

int sys_cv_sigall(int cv_id)
{
  TODO();
}

int sys_cv_close(int cv_id)
{
  TODO();
}

int sys_pipe(int fd[2])
{
  TODO();
}

int sys_link(const char *oldpath, const char *newpath)
{
  TODO();
}

int sys_symlink(const char *oldpath, const char *newpath)
{
  TODO();
}

void *syscall_handle[NR_SYS] = {
    [SYS_write] = sys_write,
    [SYS_read] = sys_read,
    [SYS_brk] = sys_brk,
    [SYS_sleep] = sys_sleep,
    [SYS_exec] = sys_exec,
    [SYS_getpid] = sys_getpid,
    [SYS_yield] = sys_yield,
    [SYS_fork] = sys_fork,
    [SYS_exit] = sys_exit,
    [SYS_wait] = sys_wait,
    [SYS_sem_open] = sys_sem_open,
    [SYS_sem_p] = sys_sem_p,
    [SYS_sem_v] = sys_sem_v,
    [SYS_sem_close] = sys_sem_close,
    [SYS_open] = sys_open,
    [SYS_close] = sys_close,
    [SYS_dup] = sys_dup,
    [SYS_lseek] = sys_lseek,
    [SYS_fstat] = sys_fstat,
    [SYS_chdir] = sys_chdir,
    [SYS_unlink] = sys_unlink,
    [SYS_mmap] = sys_mmap,
    [SYS_munmap] = sys_munmap,
    [SYS_clone] = sys_clone,
    [SYS_kill] = sys_kill,
    [SYS_cv_open] = sys_cv_open,
    [SYS_cv_wait] = sys_cv_wait,
    [SYS_cv_sig] = sys_cv_sig,
    [SYS_cv_sigall] = sys_cv_sigall,
    [SYS_cv_close] = sys_cv_close,
    [SYS_pipe] = sys_pipe,
    [SYS_link] = sys_link,
    [SYS_symlink] = sys_symlink};
