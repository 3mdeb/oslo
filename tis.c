/*
 * \brief   TIS access routines
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
#include "tis.h"


/**
 * Init the TIS driver.
 * Returns a TIS_INIT_* value.
 */
enum tis_init
tis_init()
{
  struct tis_id *id = (struct tis_id *)(TIS_LOCALITY_0 + TPM_DID_VID_0);

  switch (id->did_vid)
    {
    case 0x2e4d5453:   /* "STM." */
      out_description("TPM STM rev:", id->rid);
      return TIS_INIT_STM;
    case 0xb15d1:
      out_description("TPM Infinion rev:", id->rid);
      return TIS_INIT_INFINEON;
    case 0:
    case -1:
      out_string("No TPM found!\n");
      return TIS_INIT_NO_TPM;
    default:
      out_description("Unknown TPM found! ID:",id->did_vid);
      return TIS_INIT_NO_TPM;
    }
      
}


/**
 * Deactivate all localities.
 */
int
tis_deactivate()
{
  int res = 0;
  for (unsigned i=0; i<4; i++)
    {
      volatile struct tis_mmap *mmap = (struct tis_mmap *)(TIS_BASE+(i<<12));
      mmap->access = TIS_ACCESS_ACTIVE;
      res |= mmap->access & TIS_ACCESS_ACTIVE;
    }
  return res;
}


/**
 * Request access for a given locality.
 * @param locality: address of the locality e.g. TIS_LOCALITY_2
 * Returns 0 if we could not gain access.
 */
int
tis_access(int locality, int force)
{
  assert(locality>=TIS_LOCALITY_0 && locality <= TIS_LOCALITY_4);
  volatile struct tis_mmap *mmap = (struct tis_mmap *) locality;

  CHECK3(1, !(mmap->access & TIS_ACCESS_VALID), "access register not valid");
  CHECK3(2, mmap->access & TIS_ACCESS_ACTIVE, "locality already active");

  mmap->access = force ? TIS_ACCESS_TO_SEIZE : TIS_ACCESS_REQUEST;

  wait(10);
  // make the tpm ready -> abort a command
  mmap->sts_base = TIS_STS_CMD_READY;

  return mmap->access & TIS_ACCESS_ACTIVE;
}


static
void
wait_state(volatile struct tis_mmap *mmap, unsigned state)
{
  for (unsigned i=0; i<750 && (mmap->sts_base & state)!=state; i++)
    wait(1);
}


/**
 * Write the given buffer to the TPM.
 * Returns the numbers of bytes transfered or an value < 0 on errors.
 */
static
int
tis_write(int locality, unsigned char *buffer, unsigned int size)
{
  volatile struct tis_mmap *mmap = (struct tis_mmap *) locality;
  unsigned res;

  if (!(mmap->sts_base & TIS_STS_CMD_READY))
    {
      // make the tpm ready -> wakeup from idle state
      mmap->sts_base = TIS_STS_CMD_READY;
      wait_state(mmap, TIS_STS_CMD_READY);
    }
  CHECK3(-1, !(mmap->sts_base & TIS_STS_CMD_READY), "tis_write() not ready");


  for(res=0; res < size;res++)
      mmap->data_fifo = *buffer++;

  wait_state(mmap, TIS_STS_VALID);
  CHECK3(-2, mmap->sts_base & TIS_STS_EXPECT,   "TPM expects more data");

  //execute the command
  mmap->sts_base = TIS_STS_TPM_GO;

  return res;
}


/**
 * Read into the given buffer from the TPM.
 * Returns the numbers of bytes received or an value < 0 on errors.
 */
static
int
tis_read(int locality, unsigned char *buffer, unsigned int size)
{
  volatile struct tis_mmap *mmap = (struct tis_mmap *) locality;
  unsigned res = 0;

  wait_state(mmap, TIS_STS_VALID | TIS_STS_DATA_AVAIL);
  CHECK4(-2, !(mmap->sts_base & TIS_STS_VALID), "sts not valid",mmap->sts_base);

  for (res=0; res < size && mmap->sts_base & TIS_STS_DATA_AVAIL; res++)
      *buffer++ = mmap->data_fifo;

  CHECK3(-3, mmap->sts_base & TIS_STS_DATA_AVAIL, "more data availabe");

  // make the tpm ready again -> this allows tpm background jobs to complete
  mmap->sts_base = TIS_STS_CMD_READY;
  return res;
}


/**
 * Transmit a command to the TPM and wait for the response.
 * This is our high level TIS function used by all TPM commands.
 */
int
tis_transmit(int locality, unsigned char *buffer, unsigned write_count, unsigned read_count)
{
  assert(locality>=TIS_LOCALITY_0 && locality <= TIS_LOCALITY_4);
  unsigned int res;

  res = tis_write(locality, buffer, write_count);
  CHECK4(-1, res<=0, "  TIS write error:",res);
  
  res = tis_read(locality, buffer, read_count);
  CHECK4(-2, res<=0, "  TIS read error:",res);

  return res;
}