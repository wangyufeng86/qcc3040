/****************************************************************************
 * Copyright (c) 2013 - 2017 Qualcomm Technologies International, Ltd.
****************************************************************************/
#ifndef ARCHITECTURE_H_INCLUDED
#define ARCHITECTURE_H_INCLUDED

#if defined CHIP_GORDON
#include "gordon.h"
#elif defined CHIP_RICK
#include "rick.h"
#elif defined CHIP_CRESCENDO
#include "crescendo.h"
#if CHIP_BUILD_VER == CHIP_MINOR_VERSION_d00
#error Compiling for Crescendo D00 is not supported anymore.
#endif
#elif defined CHIP_AURA
#include "aura.h"
#elif defined CHIP_STREPLUS
#include "streplus.h"
#else
#error "Unknown or unsupported CHIP_XXXX"
#endif


#endif

