/****************************************************************************
 * Copyright 2017 Qualcomm Technologies International, Ltd.
****************************************************************************/
/**
 * \file hal_multi_core.h
 * \ingroup hal
 *
 * Header for functions shared between chips used to initialize the
 * secondary cores.
 */

#ifndef _HAL_MULTI_CORE_H_
#define _HAL_MULTI_CORE_H_

#include "types.h"

/****************************************************************************
Public Constant and macros
*/
#define HAL_MAX_NUM_CORES 2

/* Minimum number of cores to enable dual core feature */
#define HAL_MIN_NUM_CORES 2

#ifndef PM_RAM_SIZE_WORDS
#error "PM_RAM_SIZE_WORDS needs to be defined for the chip"
#endif

#ifndef NUMBER_PM_BANKS
#error "NUMBER_PM_BANKS needs to be defined for the chip"
#endif

#define PM_BANK_SIZE ((PM_RAM_SIZE_WORDS*PC_PER_INSTRUCTION)/NUMBER_PM_BANKS)

/* P1 Cache starts after the usable PM RAM ends */
#if !defined(PM_RAM_P1_CACHE_START_ADDRESS)
#error "PM_RAM_P1_CACHE_START_ADDRESS needs to be defined for the chip"
#endif


/****************************************************************************
Public types
*/

typedef enum
{
    DM1 = 0,
    DM2 = 1
} hal_dm_bank;

/****************************************************************************
Public Function Declarations
*/

/**
 * \brief Initialise the multi-core specific hardware registers
 *        hal_init() must be called before calling this API and it
 *        is required to be called if more than one core is supported.
 *        This must be called by P0 only.
 *
 * \params num_cores  - Number of cores to be enabled.
 *                      currently it must be always 2.
 */
extern void hal_multi_core_init(uint8 num_cores);

/**
 * \brief Resets multi-core specific hardware registers back to single-core
 *        defaults.
 */
extern void hal_multi_core_disable(void);

/**
 * \brief Return the number of active cores
 *
 *
 * \return num_cores : Number of active cores
 */
extern uint8 hal_get_active_num_cores(void);

/**
 * \brief  Sets the number of active core. Used when cores are disabled after
 *         boot. This can only be called by P0.
 *
 * \param  num_cores Number of active cores
 * \return           TRUE if successful and FALSE
 */
extern bool hal_set_active_num_cores(uint8 num_cores);

/**
 * \brief  Returns TRUE if the primary processor is running from ROM.
 *
 * \return TRUE if running from ROM, FALSE when running from SQIF
 */
extern bool hal_booted_from_rom(void);

/**
 * \brief  Returns TRUE if Curator has enabled the SQIF.
 */
extern bool hal_sqif_is_enabled(void);

/**
 * \brief Configure the access control registers for PM banks.
 * \param  core The processor core for which PM bank access control is to be set
 * \param  start_address Start address in PM RAM where the core needs access rights.
 */
extern void hal_multi_core_configure_pm_bank_access_control(unsigned core, unsigned start_address);

/**
 * \brief Configure a bank's arbiter
 *
 * \param bank The bank number.
 * \param core Number of the processor meant to own the bank.
 * \param bus  Which of DM1 or DM2 should access the bank.
 * \param allow_other_cores Whether the other (non-owner) core(s)
 *             has write access to this bank
 */
extern void hal_multi_core_configure_arbiter(unsigned bank, unsigned core, hal_dm_bank bus, bool allow_other_cores);

/**
 * \brief Configure the access control registers for PM banks.
 *
 * \param  core The processor core for which PM bank access control is to be set
 * \param  start_address Start address in PM RAM where the core needs access rights.
 */
extern void hal_thread_offload_configure_pm_bank_access_control(void);

#endif /* _HAL_MULTI_CORE_H_ */
