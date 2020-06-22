/****************************************************************************
 * Copyright 2017 Qualcomm Technologies International, Ltd.
****************************************************************************/
/**
 * \file hal_multi_core_chip.h
 * \ingroup hal
 *
 * Header for chip specific function to initialize the secondary cores.
 * This file should be included only in "hal_multi_core.c". The actual
 * implementations should be stored into "$CHIP_NAME/hal_$CHIP_NAME.c".
 */

#ifndef HAL_MULTI_CORE_CHIP_H
#define HAL_MULTI_CORE_CHIP_H

/****************************************************************************
Include Files
*/
#include "hal_multi_core.h"

/****************************************************************************
Public Function Declarations
*/

/**
 * \brief Chip specific implementation to configure the access control registers for PM banks.
 *
 * \param  core The processor core for which PM bank access control is to be set
 * \param  start_addr Start address in PM RAM where the core needs access rights.
 */
extern void hal_multi_core_configure_pm_bank_access_control_chip(unsigned core, unsigned start_addr);
extern void hal_multi_core_configure_arbiter_chip(unsigned bank, unsigned core, hal_dm_bank bus, bool allow_other_cores);
#endif /* HAL_MULTI_CORE_CHIP_H */
