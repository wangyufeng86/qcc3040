// *****************************************************************************
// Copyright (c) 2007 - 2017 Qualcomm Technologies International, Ltd.
// %%version
//
// *****************************************************************************

#ifndef CVC_MODULES_H
#define CVC_MODULES_H

.CONST $CVC_VERSION                    0x002000;

#include "portability_macros.h"
#include "filter_bank/filter_bank_library.h"
#include "harm100/harm100_library.h"
#include "dms200/dms200_library.h"
#include "ndvc200/ndvc200_library.h"
#include "agc400/agc400_library.h"
#include "vad400/vad400_library.h"
#include "vad410/vad410_library.h"
#include "AdapEq/AdapEq_library.h"
#include "mgdc100/mgdc100_library.h"
#include "asf100/asf100_library.h"
#include "dmss/dmss_library.h"
#include "aec520/aec520_library.h"
#include "aed100/aed100_library.h"
#include "stream/stream_library.h"
#include "asf200/asf200_library.h"

#if 0
#include "adf200/adf200_library.h"
#include "ssr/ssr_library.h"
#include "nc100/nc100_library.h"
#endif

#endif // CVC_MODULES_H
