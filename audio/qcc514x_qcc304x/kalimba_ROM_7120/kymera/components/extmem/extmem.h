/*************************************************************************
 * Copyright (c) 2018 - 2019  Qualcomm Technologies International, Ltd.
 * All Rights Reserved.
 * Qualcomm Technologies International, Ltd. Confidential and Proprietary.
*************************************************************************/
/**
 * \defgroup Extmem external memory manager
 *
 * \file extmem.h
 * \ingroup extmem
 *
 * Public definitions for the extmem module. Some of the chip variants supports an
 * external data memory( currently only SPI RAM). The APIs present here
 * allows to enable of disable the memory when required. The capabilities
 * can enable the memory when using the external memory first time and
 * disable it to save power when it is no longer using it. If multiple
 * capabilities are using the external memory, the framework reference counts
 * the usage and disable the memory only when the last user disables it.
 */
#ifndef _EXT_MEM_H_
#define _EXT_MEM_H_

/****************************************************************************
Include Files
*/
#include "types.h"


/* EXT_MEM Types */
typedef enum{
     EXTMEM_SPI_RAM,
     EXTMEM_UNKNOWN
}EXTMEM_TYPE;

/* EXT MEM Modes */
typedef enum{
      EXTMEM_OFF,   /* Turn OFF External memory */
      EXTMEM_ON,    /* Turn ON External memory with current clock setting*/
      EXTMEM_INVALID, /* External memory is not valid */
}EXTMEM_MODE;

/*
 *  The clock of the external memory which is only for informative purpose returned
 *  in the callback  and cannot be changed while enabling the external memory.
 *  The framework internally uses EXTMEM_CLK_32MHZ mode if possible whenever the
 *  system is configured to run  at low power mode with external memory enabled.
 *  Otherwise when enabled, it runs at  EXTMEM_CLK_80MHZ mode. If the external memory is off,
 *  it returns EXMEM_CLK_OFF or EXTMEM_CLK_PLL_OFF.
 */
typedef enum{
       EXTMEM_CLK_PLL_OFF,      /* RAM and PLL are OFF */
       EXTMEM_CLK_OFF,          /* RAM is OFF */
       EXTMEM_CLK_32MHZ,        /* 32MHZ ext memory clock */
       EXTMEM_CLK_80MHZ         /* 80MHz ext memory clock */
}EXTMEM_CLK;


/**
 * \brief Callback on external memory state change
 *
 * \param type External memory type
 * \param mode Enable state. EXTMEM_ON if enabled otherwise EXTMEM_OFF
 * \param clk  current external memory clock
 *
 * \return void
 *
 * Callback type to receive a notification on successfully completing
 * an external memory enable or disable operation.
 */
typedef void (*EXTMEM_CBACK)(EXTMEM_TYPE type, EXTMEM_MODE mode, EXTMEM_CLK clk);


/**
 * \brief Power on or off the external memory.
 *
 * \param type     External memory type
 * \param mode     power mode. EXTMEM_ON to enabled nd EXTMEM_OFF to disable
 * \param callback callback to return the enable status if present
 *
 * \return TRUE on initiating the operation.
 *
 * The external memory can be powered on or off using the following API.
 * If the external memory is already enabled, it returns TRUE by doing a
 * reference count of enable requests. If a power on is already pending,
 * this API returns FALSE. To avoid this, the caller can grab the extmem
 * lock before making this call. This avoids a low priority task getting
 * interrupted after acquiring an exclusive lock and getting into a deadlock
 * situation due to some other higher priority task waiting on the lock,
 * block the interrupts while acquiring the lock and doing the operation.
 *
 * e.g:
 *  bool done=FALSE;
 *  do{
 *        LOCK_INTERRUPTS;
 *        if((done=extmem_lock(EXTMEM_SPI_RAM, TRUE)))
 *           {
 *                 / * Do an external memory power on/power off * /
 *                if(!extmem_enable( EXTMEM_SPI_RAM, EXTMEM_ON/ EXTMEM_OFF, callback))
 *                {
 *                   / * unexpected failure - Panic/Fault * /
 *                }
 *                extmem_unlock(EXTMEM_SPI_RAM);
 *           }
 *          UNLOCK_INTERRRUPTS;
 *     }while(!done)
 *
 */
extern bool extmem_enable(EXTMEM_TYPE type, EXTMEM_MODE mode, EXTMEM_CBACK cback);

/**
 * \brief Attempt to power on or off the external memory until success or timeout.
 *
 * \param type     External memory type
 * \param mode     power mode. EXTMEM_ON to enabled nd EXTMEM_OFF to disable
 * \param callback callback to return the enable status if present
 * \param timeout  maximum attempt time in micro seconds
 *
 * \return TRUE on initiating the operation.
 *
 * \brief This function re-attempts calling extme_enable for a finite time before
 * returning the failure
 */
extern bool extmem_enable_with_retry(EXTMEM_TYPE type, EXTMEM_MODE mode,
                                     EXTMEM_CBACK cback, TIME wait_time);


/**
 * \brief Grab an inusage lock.
 *
 * \param type       External memory type
 * \param exclusive  Don't allow anyone else to grab the lock.
 *
 * \return TRUE on successfully grabing the lock
 *
 * The external memory provides a lock which can act as a mutex or countable semaphore.
 * If the exclusive parameter is set to TRUE, it acts like a mutex (called exclusive lock),
 * otherwise a countable semaphore (called non-exclusive lock). Use the exclusive lock
 * while changing the external memory state like enabling or disabling it. Use the
 * non-exclusive lock while accessing the data memory using extBuffer APIs.
 *
 * e.g:
 * bool done=FALSE;
 * do{
 *   LOCK_INTERRUPTS;
 *   if((done=extmem_lock(EXTMEM_SPI_RAM, TRUE)))
 *   {
 *       / * Do an external memory power on/power off / clock change * /
 *       extmem_unlock(EXTMEM_SPI_RAM);
 *   }
 *   UNLOCK_INTERRRUPTS;
 * }while(!done)
 *
 * Use non-exclusive lock while doing an extBuffer operation, so it will not
 * block other users to do
 */
extern bool extmem_lock(EXTMEM_TYPE type, bool exclusive);


/**
 * \brief  Release the  lock.
 *
 * \param type External memory type
 *
 * \return TRUE on successfully grabing the lock
 */
extern bool extmem_unlock(EXTMEM_TYPE type);

/**
 * \brief  Get the current mode
 *
 * \param type External memory type
 *
 * \return current mode
 */
extern EXTMEM_MODE extmem_get_mode(EXTMEM_TYPE type);


#endif /*_EXT_MEM_H_*/
