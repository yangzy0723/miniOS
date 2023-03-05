#include "klib.h"
#include "vme.h"
#include "proc.h"

typedef union free_page
{
  union free_page *next;
  char buf[PGSIZE];
} page_t;

static page_t *free_page_list;

static TSS32 tss;

void init_gdt()
{
  static SegDesc gdt[NR_SEG];
  gdt[SEG_KCODE] = SEG32(STA_X | STA_R, 0, 0xffffffff, DPL_KERN);
  gdt[SEG_KDATA] = SEG32(STA_W, 0, 0xffffffff, DPL_KERN);
  gdt[SEG_UCODE] = SEG32(STA_X | STA_R, 0, 0xffffffff, DPL_USER);
  gdt[SEG_UDATA] = SEG32(STA_W, 0, 0xffffffff, DPL_USER);
  gdt[SEG_TSS] = SEG16(STS_T32A, &tss, sizeof(tss) - 1, DPL_KERN);
  set_gdt(gdt, sizeof(gdt[0]) * NR_SEG);
  set_tr(KSEL(SEG_TSS));
}

void set_tss(uint32_t ss0, uint32_t esp0)
{
  tss.ss0 = ss0;
  tss.esp0 = esp0;
}

static PD kpd;                                          // 页目录
static PT kpt[PHY_MEM / PT_SIZE] __attribute__((used)); // 页表

void init_page()
{
  extern char end;
  panic_on((size_t)(&end) >= KER_MEM - PGSIZE, "Kernel too big (MLE)");
  static_assert(sizeof(PTE) == 4, "PTE must be 4 bytes");
  static_assert(sizeof(PDE) == 4, "PDE must be 4 bytes");
  static_assert(sizeof(PT) == PGSIZE, "PT must be one page");
  static_assert(sizeof(PD) == PGSIZE, "PD must be one page");
  // Lab1-4: init kpd and kpt, identity mapping of [0 (or 4096), PHY_MEM)
  for (int i = 0; i < PHY_MEM / PT_SIZE; i++)
  {
    kpd.pde[i].val = MAKE_PDE((uint32_t)(&kpt[i]), 0x1);
    for (int j = 0; j < NR_PTE; j++)
      kpt[i].pte[j].val = MAKE_PTE((uint32_t)(i << DIR_SHIFT) | (j << TBL_SHIFT), 0x1);
  } // 恒等映射
  // TODO();
  kpt[0].pte[0].val = 0;
  set_cr3(&kpd);
  set_cr0(get_cr0() | CR0_PG);
  // Lab1-4: init free memory at [KER_MEM, PHY_MEM), a heap for kernel
  free_page_list = (page_t *)PAGE_DOWN(KER_MEM);
  for (int i = 1; i < (PHY_MEM - KER_MEM) / PGSIZE; i++)
  {
    free_page_list->next = (page_t *)PAGE_DOWN(KER_MEM + i * PGSIZE);
    free_page_list = free_page_list->next;
  }
  free_page_list->next = NULL;
  free_page_list = (page_t *)PAGE_DOWN(KER_MEM);
  // TODO();
}

void *kalloc()
{
  // Lab1-4: alloc a page from kernel heap, abort when heap empty
  // printf("free_page_list in kalloc1: %p\n", free_page_list);
  // printf("in kalloc: free_page_list and free_page_list->next %p %p\n", free_page_list, free_page_list->next);
  page_t *now = free_page_list;
  if (now == NULL)
    return NULL;
  free_page_list = free_page_list->next;
  // printf("free_page_list in kalloc2: %p\n", free_page_list);
  // memset(now, 0, PGSIZE);//这里不知道为什么不可以清零
  // printf("%p\n", now);
  return now;
  // TODO();
}

void kfree(void *ptr)
{
  // Lab1-4: free a page to kernel heap
  page_t *now = (page_t *)PAGE_DOWN(ptr);
  memset(now, 0, PGSIZE);
  // printf("now in kfree1: %p\n", now);
  now->next = free_page_list;
  // printf("%p ->next is %p\n", now, free_page_list);
  free_page_list = now;
  // printf("in kfree: free_page_list and free_page_list->next %p %p\n", free_page_list, free_page_list->next);
  // printf("free_page_list in kfree2: %p\n", free_page_list);
  // TODO();
}

PD *vm_alloc()
{
  // Lab1-4: alloc a new pgdir, map memory under PHY_MEM identityly
  PD *pd_addr = kalloc();
  for (int i = 0; i < PHY_MEM / PT_SIZE; i++)
    pd_addr->pde[i].val = MAKE_PDE(&kpt[i], 0x1);
  for (int i = PHY_MEM / PT_SIZE; i < NR_PDE; i++)
    pd_addr->pde[i].val = 0x0;
  return pd_addr;
  // TODO();
}

void vm_teardown(PD *pgdir)
{
  // Lab1-4: free all pages mapping above PHY_MEM in pgdir, then free itself
  // you can just do nothing :)
  for (int i = PHY_MEM / PT_SIZE; i < NR_PDE; i++)
  {
    PDE *now_pde = &(pgdir->pde[i]);
    if (now_pde->present)
    {
      PT *pt = PDE2PT(*now_pde);
      for (int j = 0; j < NR_PTE; j++)
      {
        PTE *now_pte = &(pt->pte[j]);
        if (now_pte->present)
        {
          kfree(PTE2PG(*now_pte));
          now_pte->present = 0;
        } // 释放物理页
      }
      kfree(pt); // 释放页表
    }
  }
  // TODO();
}

PD *vm_curr()
{
  return (PD *)PAGE_DOWN(get_cr3());
}

PTE *vm_walkpte(PD *pgdir, size_t va, int prot)
{
  // Lab1-4: return the pointer of PTE which match va
  // if not exist (PDE of va is empty) and prot&1, alloc PT and fill the PDE
  // if not exist (PDE of va is empty) and !(prot&1), return NULL
  // remember to let pde's prot |= prot, but not pte
  assert((prot & ~7) == 0);
  int pd_index = ADDR2DIR(va);
  PDE *pde = &(pgdir->pde[pd_index]);
  PT *pt;
  if (pde->present == 0) // PDE有效位为0
  {
    if (prot != 0)
    {
      pt = kalloc();
      memset(pt, 0, PGSIZE);
      pde->val = MAKE_PDE(pt, prot);
    }
    else
      return NULL;
  }
  else
  {
    pde->val = pde->val | prot;
    pt = PDE2PT(*pde);
  }
  int pt_index = ADDR2TBL(va);
  PTE *pte = &(pt->pte[pt_index]);
  return pte;
  // TODO();
}

void *vm_walk(PD *pgdir, size_t va, int prot)
{
  // Lab1-4: translate va to pa
  // if prot&1 and prot voilation ((pte->val & prot & 7) != prot), call vm_pgfault
  // if va is not mapped and !(prot&1), return NULL
  PTE *pte = vm_walkpte(pgdir, va, prot);
  if (pte == NULL)
    return NULL;
  if (pte->present == 0)
    return NULL;
  void *page = PTE2PG(*pte);
  return (void *)((uint32_t)page | ADDR2OFF(va));
  // TODO();
}

void vm_map(PD *pgdir, size_t va, size_t len, int prot)
{
  // Lab1-4: map [PAGE_DOWN(va), PAGE_UP(va+len)) at pgdir, with prot
  // if have already mapped pages, just let pte->prot |= prot
  size_t map_start = PAGE_DOWN(va);
  size_t map_end = PAGE_UP(va + len);
  for (int i = map_start; i < map_end; i += PGSIZE)
  {
    PTE *pte = vm_walkpte(pgdir, i, prot);
    if (pte->present == 0)
      pte->val = MAKE_PTE(kalloc(), prot);
    else
      pte->val = pte->val | prot;
  }
  assert(prot & PTE_P);
  assert((prot & ~7) == 0);
  size_t start = PAGE_DOWN(va);
  size_t end = PAGE_UP(va + len);
  assert(start >= PHY_MEM);
  assert(end >= start);
  // TODO();
}

void vm_unmap(PD *pgdir, size_t va, size_t len)
{
  // Lab1-4: unmap and free [va, va+len) at pgdir
  // you can just do nothing :)
  assert(ADDR2OFF(va) == 0);
  assert(ADDR2OFF(len) == 0);
  size_t map_start = PAGE_DOWN(va);
  size_t map_end = PAGE_UP(va + len);
  for (int i = map_start; i < map_end; i += PGSIZE)
  {
    PTE *pte = vm_walkpte(pgdir, i, 0x0);
    if (pte != NULL && pte->present)
    {
      kfree(PTE2PG(*pte));
      pte->val = 0;
    }
  }
  // TODO();
}

void vm_copycurr(PD *pgdir)
{
  // Lab2-2: copy memory mapped in curr pd to pgdir
  TODO();
}

void vm_pgfault(size_t va, int errcode)
{
  printf("pagefault @ 0x%p, errcode = %d\n", va, errcode);
  panic("pgfault");
}