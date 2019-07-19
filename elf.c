#include "elf.h"
#include "util.h"


int
extract_module(struct mbi *mbi, unsigned *entry_point)
{
  // skip module after loading
  struct module *m  = (struct module *) mbi->mods_addr;
  mbi->mods_addr += sizeof(struct module);
  mbi->mods_count--;
  mbi->cmdline = m->string;

  // switch it on unconditionally, we assume that m->string is always initialized
  mbi->flags |=  MBI_FLAG_CMDLINE;

  // check elf header
  struct eh *elf = (struct eh *) m->mod_start;
  ERROR(-21, *((unsigned *) elf->e_ident) != 0x464c457f || *((short *) elf->e_ident+2) != 0x0101, "ELF header incorrect");
  ERROR(-22, elf->e_type!=2 || elf->e_machine!=3 || elf->e_version!=1, "ELF type incorrect");
  ERROR(-23, sizeof(struct ph) > elf->e_phentsize, "e_phentsize to small");

  for (unsigned i=0; i<elf->e_phnum; i++)
    {
      struct ph *ph = (struct ph *)(m->mod_start + elf->e_phoff+ i*elf->e_phentsize);
      if (ph->p_type != 1)
	continue;
      memcpy(ph->p_paddr, (char *)(m->mod_start+ph->p_offset), ph->p_filesz);
      memset(ph->p_paddr+ph->p_filesz, 0, ph->p_memsz - ph->p_filesz);
    }
  *entry_point = elf->e_entry;
  return 0;
}


/**
 * Start the first module as kernel.
 */
int
start_module(struct mbi *mbi)
{
  int res;
  unsigned entry_point;

  if ((res = extract_module(mbi, &entry_point)))
    return res;
  // wait(2000);
  out_char('\n');
  jmp_multiboot(mbi, entry_point);
}
