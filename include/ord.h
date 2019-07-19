#ifndef _ORD_H
#define _ORD_H

enum tpm_ords {
	TPM_ORD_OIAP=10,
	TPM_ORD_OSAP=11,
	TPM_ORD_Extend=20,
	TPM_ORD_PcrRead,
	TPM_ORD_Quote,
	TPM_ORD_Seal,
	TPM_ORD_EvictKey=34,
	TPM_ORD_Reset=90,
	TPM_ORD_OwnerClear=91,
	TPM_ORD_ForceClear=93,
	TPM_ORD_GetCapability=101,
	TPM_ORD_ReadPubek=124,
	TPM_ORD_TerminateHandle=150,
	TPM_ORD_SaveState=152,
	TPM_ORD_Startup,
	TPM_ORD_SHA1Start=160,
	TPM_ORD_SHA1Update,
	TPM_ORD_SHA1Complete,
	TPM_ORD_SHA1CompleteExtend,
};

enum tpm_caps {
	TPM_CAP_PROPERTY=5,
	TPM_CAP_VERSION,
	TPM_CAP_KEY_HANDLE,
};

enum tpm_subcaps {
	TPM_CAP_PROP_PCR   = 257,
	TPM_CAP_PROP_SLOTS = 260,
};

enum tpm_subcaps_size {
  TPM_NO_SUBCAP=0,
  TPM_SUBCAP=4,
};


enum tpm_startup_code {
	TPM_ST_CLEAR=1,
	TPM_ST_STATE,
	TPM_ST_DEACTIVATED,
};

#endif
