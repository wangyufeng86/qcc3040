// -----------------------------------------------------------------------------
// Copyright (c) 2019                  Qualcomm Technologies International, Ltd.
//
#ifndef __RATE_ADJUST_GEN_C_H__
#define __RATE_ADJUST_GEN_C_H__

#ifndef ParamType
typedef unsigned ParamType;
#endif

// CodeBase IDs
#define RATE_ADJUST_RATE_ADJUST_CAP_ID	0x00B3
#define RATE_ADJUST_RATE_ADJUST_ALT_CAP_ID_0	0x407C
#define RATE_ADJUST_RATE_ADJUST_SAMPLE_RATE	0
#define RATE_ADJUST_RATE_ADJUST_VERSION_MAJOR	 1

// Constant Definitions


// Runtime Config Parameter Bitfields


unsigned *RATE_ADJUST_GetDefaults(unsigned capid);

#endif // __RATE_ADJUST_GEN_C_H__
