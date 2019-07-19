/*
 * \brief   multiboot structures
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


#ifndef _MBI_H
#define _MBI_H

enum mbi_enum
  {
    MBI_MAGIC        = 0x2badb002,
    MBI_FLAG_MEM     = 1 << 0,
    MBI_FLAG_CMDLINE = 1 << 2,
    MBI_FLAG_MODS    = 1 << 3,
  };


struct mbi
{
  unsigned flags;
  unsigned mem_lower;
  unsigned mem_upper;
  unsigned boot_device;
  unsigned cmdline;
  unsigned mods_count;
  unsigned mods_addr;
};


struct module
{
  unsigned mod_start;
  unsigned mod_end;
  unsigned string;
  unsigned reserved;
};

#endif
