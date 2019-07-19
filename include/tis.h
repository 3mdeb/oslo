/*
 * \brief   TIS data structures and header of tis.c
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

#ifndef _TIS_H_
#define _TIS_H_

enum tis_init
  {
    TIS_INIT_NO_TPM  = 0,
    TIS_INIT_STM = 1,
    TIS_INIT_INFINEON = 2
  };


enum tis_mem_offsets
  {
    TIS_BASE =  (int) 0xfed40000,
    TPM_DID_VID_0 = 0xf00,
    TIS_LOCALITY_0 = TIS_BASE+0x0000,
    TIS_LOCALITY_1 = TIS_BASE+0x1000,
    TIS_LOCALITY_2 = TIS_BASE+0x2000,
    TIS_LOCALITY_3 = TIS_BASE+0x3000,
    TIS_LOCALITY_4 = TIS_BASE+0x4000
  };


struct tis_id
{
  int did_vid;
  unsigned char rid;
} __attribute__((packed));


struct tis_mmap
{
  unsigned char  access;
  unsigned char  __dummy1[7];
  unsigned int   int_enable;
  unsigned char  int_vector;
  unsigned char  __dummy2[3];
  unsigned int   int_status;
  unsigned int   intf_capability;
  unsigned char  sts_base;
  unsigned short sts_burst_count;
  unsigned char  __dummy3[9];
  unsigned char  data_fifo;
} __attribute__((packed));


enum tis_access_bits
  {
    TIS_ACCESS_VALID    = 1<<7,
    TIS_ACCESS_RESERVED = 1<<6,
    TIS_ACCESS_ACTIVE   = 1<<5,
    TIS_ACCESS_SEIZED   = 1<<4,
    TIS_ACCESS_TO_SEIZE = 1<<3,
    TIS_ACCESS_PENDING  = 1<<2,
    TIS_ACCESS_REQUEST  = 1<<1,
    TIS_ACCESS_TOS      = 1<<0
  };


enum tis_sts_bits
  {
    TIS_STS_VALID       = 1<<7,
    TIS_STS_CMD_READY   = 1<<6,
    TIS_STS_TPM_GO      = 1<<5,
    TIS_STS_DATA_AVAIL  = 1<<4,
    TIS_STS_EXPECT      = 1<<3,
    TIS_STS_RESERVED_2  = 1<<2,
    TIS_STS_RETRY       = 1<<1,
    TIS_STS_RESERVED_0  = 1<<0
  };



void tis_dump();
enum tis_init tis_init();
int tis_deactivate();
int tis_access(int locality, int force);
int tis_transmit(int locality,
		 unsigned char *buffer,
		 unsigned write_count,
		 unsigned read_count);

#endif