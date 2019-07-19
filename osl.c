/*
 * \brief   OSLO - Open Secure Loader
 * \date    2006-03-28
 * \author  Bernhard Kauer <kauer@tudos.org>
 */
/*
 * Copyright (C) 2006  Bernhard Kauer <kauer@tudos.org>
 * Technische Universitaet Dresden, Operating Systems Research Group
 *
 * This file is part of the OSLO package, which is distributed under
 * the  terms  of the  GNU General Public Licence 2.  Please see the
 * COPYING file for details.
 */


#include "util.h"
#include "sha.h"
#include "elf.h"
#include "tpm.h"
#include "osl.h"


char *version_string = "OSLO v.0.3.2\n";


/**
 *  Hash all multiboot modules.
 */
static
int
mbi_calc_hash(struct mbi *mbi, struct Context *ctx)
{
  ERROR(-11, ~mbi->flags & MBI_FLAG_MODS, "module flag missing");
  ERROR(-12, !mbi->mods_count, "no module to hash");
  out_description("Hashing modules count:", mbi->mods_count);

  sha1_init(ctx);
  struct module *m  = (struct module *) (mbi->mods_addr);
  for (unsigned i=0; i < mbi->mods_count; i++, m++)
    {
      ERROR(-13, m->mod_end < m->mod_start, "mod_end less than start");
      sha1(ctx, (unsigned char*) m->mod_start, m->mod_end - m->mod_start);
    }
  sha1_finish(ctx);
  return 0;
}


/**
 * Start the first module as kernel.
 */
static
int
start_module(struct mbi *mbi)
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

  wait(5000);
  jmp_multiboot(mbi, elf->e_entry);
}


/**
 * Checks whether we have SVM support and a local APIC.
 *
 * @return: the SVM revision of the processor or a negative value, if
 * not supported.
 */
int
check_cpuid()
{
  int res;
  ERROR(-31,0x8000000A > cpuid_eax(0x80000000), "no ext cpuid");
  ERROR(-32,!(0x4   & cpuid_ecx(0x80000001)), "no SVM support");
  ERROR(-33,!(0x200 & cpuid_edx(0x80000001)), "no APIC support");
  res = cpuid_eax(0x8000000A) & 0xff;
  return res;
}

/**
 * Enables SVM support.
 *
 */
int
enable_svm()
{
  enum
    {
      MSR_EFER = 0xC0000080,
      EFER_SVME = 1<<12,
    };

  unsigned long long value;
  value = rdmsr(MSR_EFER);
  wrmsr(MSR_EFER, value | EFER_SVME);
  ERROR(-40, !(rdmsr(MSR_EFER) & EFER_SVME), "could not enable SVM");
  return 0;
}


/**
 * Before a skinit can take place the ApplicationProcessors in an MP
 * system must be in the init state.
 */
int
stop_processors()
{
  enum
    {
      MSR_APIC_BASE    = 0x1B,
      APIC_BASE_ENABLE = 0x800,
      APIC_BASE_BSP    = 0x100,

      APIC_ICR_LOW_OFFSET = 0x300,

      APIC_ICR_DST_ALL_EX  = 0x3 << 18,
      APIC_ICR_LEVEL_EDGE  = 0x0 << 15,
      APIC_ICR_ASSERT      = 0x1 << 14,
      APIC_ICR_PENDING     = 0x1 << 12,
      APIC_ICR_INIT        = 0x5 << 8,
    };

  unsigned long long value;
  value = rdmsr(MSR_APIC_BASE);
  ERROR(-51, !(value & (APIC_BASE_ENABLE | APIC_BASE_BSP)), "not BSP or APIC disabled");
  ERROR(-52, (value >> 32) & 0xf, "APIC out of range");

  unsigned long *apic_icr_low = (unsigned long *)(((unsigned long)value & 0xfffff000) + APIC_ICR_LOW_OFFSET);
  
  ERROR(-53, *apic_icr_low & APIC_ICR_PENDING, "Interrupt pending");
  *apic_icr_low = APIC_ICR_DST_ALL_EX | APIC_ICR_LEVEL_EDGE | APIC_ICR_ASSERT | APIC_ICR_INIT;

  while (*apic_icr_low & APIC_ICR_PENDING)
    wait(1);

  return 0;
}


/**
 * This function runs before skinit and has to enable SVM in the processor
 * and disable all localities.
 */
int
_main(struct mbi *mbi, unsigned flags)
{

  unsigned char buffer[TCG_BUFFER_SIZE];

  out_string(version_string);
  ERROR(10, !mbi || flags != MBI_MAGIC, "Not loaded via multiboot");

  if (tis_init()) 
    {
      int res;

      ERROR(10, !tis_access(TIS_LOCALITY_0, 1), "could not gain TIS ownership");
      if ((res=TPM_Startup_Clear(TIS_LOCALITY_0, buffer)) && res!=0x26)
	out_description("TPM_Startup() failed",res);

      ERROR(11, tis_deactivate(), "tis_deactivate failed");
    }

  int revision;
  ERROR(12, 0>(revision = check_cpuid()), "No SVM platform");
  ERROR(13, enable_svm(), "could not enable SVM");
  out_description("SVM revision:", revision);

  ERROR(14, stop_processors(), "sending an INIT IPI to other processors failed");

  out_string("call skinit\n");
  do_skinit();
}


static void
show_hash(char *s, unsigned char *hash)
{
  out_string(s);
  for (unsigned i=0; i<20; i++)
    out_hex(hash[i], 2);
  out_char('\n');
}

/**
 * This code is executed after skinit.
 */
int
oslo(struct mbi *mbi)
{
  struct Context ctx;
  unsigned res;
  const int locality = TIS_LOCALITY_2;
  
  ERROR(20, !mbi, "no mbi in oslo()");
  ERROR(21, mbi_calc_hash(mbi, &ctx),  "calc hash failed");

  show_hash("HASH ",ctx.hash);

  if (tis_init())
    {
      ERROR(22, !tis_access(locality, 1), "could not gain TIS ownership");
      CHECK4(23, (res = TPM_Extend(locality, ctx.buffer, 19, ctx.hash)), "TPM extend failed", res);

      show_hash("PCR[19]: ",ctx.hash);

#ifndef NDEBUG
      dump_pcrs(locality, ctx.buffer);

      CHECK4(24,(res = TPM_PcrRead(locality, ctx.buffer, 17, ctx.hash)), "TPM_PcrRead failed", res);
      show_hash("PCR[17]: ",ctx.hash);
#endif
      ERROR(25, tis_deactivate(), "tis_deactivate failed");
    }

  ERROR(26, start_module(mbi), "start module failed");
  return 27;
}

