#include "klib.h"
#include "serial.h"
#include "vme.h"
#include "cte.h"
#include "loader.h"
#include "fs.h"
#include "proc.h"
#include "timer.h"
#include "dev.h"

void init_user_and_go();

int main()
{
  init_gdt();
  init_serial();
  init_fs();
  init_page();  // uncomment me at Lab1-4
  init_cte();   // uncomment me at Lab1-5
  init_timer(); // uncomment me at Lab1-7
  init_proc();  // uncomment me at Lab2-1
  // init_dev(); // uncomment me at Lab3-1
  printf("Hello from OS!\n");
  init_user_and_go();
  panic("should never come back");
}

void init_user_and_go()
{
  // Lab1-2: ((void(*)())eip)();
  // Lab1-4: pdgir, stack_switch_call
  // Lab1-6: ctx, irq_iret
  // Lab1-8: argv
  // Lab2-1: proc
  // Lab3-2: add cwd

  // PD *pgdir = vm_alloc();
  // uint32_t eip = load_elf(pgdir, "brktest");
  // assert(eip != -1);
  // set_cr3(pgdir);
  // stack_switch_call((void *)(USR_MEM - 16), (void *)eip, 0);

  // PD *pgdir = vm_alloc();
  // Context ctx;
  // char *argv[] = {NULL};
  // assert(load_user(pgdir, &ctx, "sh1", argv) == 0);
  // set_cr3(pgdir);
  // set_tss(KSEL(SEG_KDATA), (uint32_t)kalloc() + PGSIZE); // 确定内核栈的位置，从用户态进入内核态的时候，栈指针要指在内核栈的栈顶
  // irq_iret(&ctx);

  // proc_t *proc = proc_alloc();
  // assert(proc);
  // char *argv[] = {"sh1", NULL};
  // assert(load_user(proc->pgdir, proc->ctx, "sh1", argv) == 0);
  // proc_addready(proc);
  // while (1)
  //   proc_yield();

  proc_t *proc = proc_alloc();
  assert(proc);
  char *argv[] = {"sh", NULL};
  assert(load_user(proc->pgdir, proc->ctx, "sh", argv) == 0);
  proc_addready(proc);

  // proc = proc_alloc();
  // assert(proc);
  // argv[1] = "1919810";
  // assert(load_user(proc->pgdir, proc->ctx, "ping2", argv) == 0);
  // proc_addready(proc);

  sti();
  while (1)
    ;
}
