#include "boot.h"

// DO NOT DEFINE ANY NON-LOCAL VARIBLE!

void load_kernel()
{
  Elf32_Ehdr *elf = (void *)0x8000; // ELF头表的位置
  copy_from_disk(elf, 255 * SECTSIZE, SECTSIZE);
  Elf32_Phdr *ph, *eph;                        // 程序头表
  ph = (void *)((uint32_t)elf + elf->e_phoff); // 表示程序头表在文件中的偏移量
  eph = ph + elf->e_phnum;
  for (; ph < eph; ph++)
  {
    if (ph->p_type == PT_LOAD)
    {
      // TODO: Lab1-2, Load kernel and jump
      memcpy((void *)ph->p_vaddr, (void *)elf + ph->p_offset, ph->p_filesz);
      memset((void *)ph->p_vaddr + ph->p_filesz, 0, ph->p_memsz - ph->p_filesz);
    }
  }
  uint32_t entry = elf->e_entry; // change me
  ((void (*)())entry)();
}
