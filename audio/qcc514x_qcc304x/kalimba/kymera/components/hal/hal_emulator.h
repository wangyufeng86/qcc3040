/****************************************************************************
 * Copyright (c) 2019 Qualcomm Technologies International, Ltd.
****************************************************************************/
/**
 * \file  hal_emulator.h
 * \ingroup HAL
 *
 * Helper function to detect whether the firmware is running on an emulator.
 */

#ifndef HAL_EMULATOR_H
#define HAL_EMULATOR_H

#include "types.h"

#ifdef CHIP_BASE_HYDRA
extern bool hal_running_on_emulator(void);
#else
#define hal_running_on_emulator() FALSE
#endif

#endif /* HAL_EMULATOR_H */