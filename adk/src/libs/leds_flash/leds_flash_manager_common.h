/****************************************************************************
Copyright (c) 2004 - 2015 Qualcomm Technologies International, Ltd.

FILE NAME
    leds_flash_manager_common.h
    
DESCRIPTION
	Header file for the leds_flash manager plugin, defining all of the functions for LED play.
*/

#ifndef _LEDS_FLASH_MANAGER_COMMON_H_
#define _LEDS_FLASH_MANAGER_COMMON_H_

#include <message.h>

/****************************************************************************
NAME 
    LedsFlashInit

DESCRIPTION
 	Initialise the Leds ROM play system
RETURNS
    void
    
*/
void LedsFlashInit(Task clientTask, const void * params);

/****************************************************************************
NAME 
    LedsFlashIndicateEvent

DESCRIPTION
    displays event notification
    
RETURNS
    void 
*/
void LedsFlashIndicateEvent(uint8 pEvent, bool can_queue);

/****************************************************************************
NAME 
    LedsFlashIndicateState

DESCRIPTION
    displays state indication information

RETURNS
    void
    
*/
void LedsFlashIndicateState(uint8 pState);


/****************************************************************************
NAME	
	LedsFlashEnableLEDS

DESCRIPTION
    Enable LED indications
RETURNS
	void
    
*/
void LedsFlashEnableLEDS(void);

/****************************************************************************
NAME	
	LedsFlashDisableLEDS

DESCRIPTION
    Disable LED indications and turn off all current LED Indications if not 
    overidden by state or filter 
    
RETURNS
	void
*/
void LedsFlashDisableLEDS(void);

#endif /*_LEDS_FLASH_MANAGER_COMMON_H_*/

