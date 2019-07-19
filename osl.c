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
#include "version.h"

char *version_string = "OSLO " VERSION "\n";
char *message_label = "OSLO:   ";

/**
 *  Hash all multiboot modules.
 */
static
int
mbi_calc_hash(struct mbi *mbi, struct Context *ctx)
{
  unsigned res;

  CHECK3(-11, ~mbi->flags & MBI_FLAG_MODS, "module flag missing");
  CHECK3(-12, !mbi->mods_count, "no module to hash");
  out_description("Hashing modules count:", mbi->mods_count);

  struct module *m  = (struct module *) (mbi->mods_addr);
  for (unsigned i=0; i < mbi->mods_count; i++, m++)
    {
      sha1_init(ctx);
      CHECK3(-13, m->mod_end < m->mod_start, "mod_end less than start");
      sha1(ctx, (unsigned char*) m->mod_start, m->mod_end - m->mod_start);
      sha1_finish(ctx);
      CHECK4(-14, (res = TPM_Extend(ctx->buffer, 19, ctx->hash)), "TPM extend failed", res);
    }
  return 0;
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
  CHECK3(-31,0x8000000A > cpuid_eax(0x80000000), "no ext cpuid");
  CHECK3(-32,!(0x4   & cpuid_ecx(0x80000001)), "no SVM support");
  CHECK3(-33,!(0x200 & cpuid_edx(0x80000001)), "no APIC support");
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
  CHECK3(-40, !(rdmsr(MSR_EFER) & EFER_SVME), "could not enable SVM");
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
  CHECK3(-51, !(value & (APIC_BASE_ENABLE | APIC_BASE_BSP)), "not BSP or APIC disabled");
  CHECK3(-52, (value >> 32) & 0xf, "APIC out of range");

  unsigned long *apic_icr_low = (unsigned long *)(((unsigned long)value & 0xfffff000) + APIC_ICR_LOW_OFFSET);
  
  CHECK3(-53, *apic_icr_low & APIC_ICR_PENDING, "Interrupt pending");
  *apic_icr_low = APIC_ICR_DST_ALL_EX | APIC_ICR_LEVEL_EDGE | APIC_ICR_ASSERT | APIC_ICR_INIT;

  while (*apic_icr_low & APIC_ICR_PENDING)
    wait(1);

  return 0;
}


/**
 * Prepare the TPM for skinit.
 * Returns a TIS_INIT_* value.
 */
static
int
prepare_tpm(unsigned char *buffer)
{
  int tpm, res;

  CHECK4(-60, 0 >= (tpm = tis_init(TIS_BASE)), "tis init failed", tpm);
  CHECK3(-61, !tis_access(TIS_LOCALITY_0, 1), "could not gain TIS ownership");
  if ((res=TPM_Startup_Clear(buffer)) && res!=0x26)
    out_description("TPM_Startup() failed",res);

  CHECK3(-62, tis_deactivate_all(), "tis_deactivate failed");
  return tpm;
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

  // set bootloader name
  mbi->flags |= MBI_FLAG_BOOT_LOADER_NAME;
  mbi->boot_loader_name = (unsigned) version_string;

  int revision = 0;
  if (0 >= prepare_tpm(buffer) && (0>(revision = check_cpuid())))
    {
      if (0 > revision)
	out_info("No SVN platform");
      else
	out_info("Could not prepare the TPM");

      ERROR(11, start_module(mbi), "start module failed");
    }

  ERROR(12, enable_svm(), "could not enable SVM");
  out_description("SVM revision:", revision);

  ERROR(13, stop_processors(), "sending an INIT IPI to other processors failed");

  out_info("call skinit");
  do_skinit();
}


static void
show_hash(char *s, unsigned char *hash)
{
  out_string(message_label);
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
  
  ERROR(20, !mbi, "no mbi in oslo()");

  if (tis_init(TIS_BASE))
    {
      ERROR(21, !tis_access(TIS_LOCALITY_2, 1), "could not gain TIS ownership");
      ERROR(22, mbi_calc_hash(mbi, &ctx),  "calc hash failed");
      show_hash("PCR[19]: ",ctx.hash);

#ifndef NDEBUG
      int res;
      dump_pcrs(ctx.buffer);

      CHECK4(24,(res = TPM_PcrRead(ctx.buffer, 17, ctx.hash)), "TPM_PcrRead failed", res);
      show_hash("PCR[17]: ",ctx.hash);
#endif
      ERROR(25, tis_deactivate_all(), "tis_deactivate failed");
  }

  ERROR(26, start_module(mbi), "start module failed");
  return 27;
}
