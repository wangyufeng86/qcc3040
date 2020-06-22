/****************************************************************************
Copyright (c) 2004 - 2015 Qualcomm Technologies International, Ltd.

FILE NAME
    leds_rom_manager_common.h
    
DESCRIPTION
	Header file for the leds_rom manager plugin, defining all of the functions for LED play.
*/

#ifndef _LEDS_ROM_MANAGER_COMMON_H_
#define _LEDS_ROM_MANAGER_COMMON_H_

#include <message.h>
#include "leds_manager.h"

#define LED_ON  TRUE    
#define LED_OFF FALSE

#define DIM_NUM_STEPS (0xf)
#define DIM_STEP_SIZE ((4096) / (DIM_NUM_STEPS + 1) ) 
#define DIM_PERIOD    (0x0)

#define LEDS_STATE_START_DELAY_MS 300

#define DIM_MSG_BASE  (0x1000)

#define LEDSA_MASK           0x6000
#define LEDSB_MASK           0x0300
#define LEDSC_MASK           0x000C
#define LEDSA_SHIFT          13
#define LEDSB_SHIFT          8
#define LEDSC_SHIFT          2

#define LEDSA_ONOFF_MASK     0x0800
#define LEDSB_ONOFF_MASK     0x0040
#define LEDSC_ONOFF_MASK     0x0001
#define LEDSA_ONOFF_SHIFT    11
#define LEDSB_ONOFF_SHIFT    6
#define LEDSC_ONOFF_SHIFT    0

/* Control information for LEDS display */
/* TODO: I think this data structure is private to the leds_rom display 
   and should not be observed by App. 
*/
typedef struct 
{
    Task                clientTask;
    uint16              gTheActiveFilters ;             /*Mask of Filters Active*/

    unsigned            pStatePatternIndex:5;
    unsigned            gLED_0_STATE:1 ;
    unsigned            gLED_1_STATE:1 ;  
    unsigned            gStateCanOverideDisable:1;
    
    unsigned 			gFollowing:1 ;                  /**do we currently have a follower active*/
    unsigned            gFollowPin:4 ;
    unsigned            gCurrentlyIndicatingEvent:1;    /*if we are currently indicating an event*/
}leds_ctrl_info;

extern leds_ctrl_info leds_rom_ctl;
extern leds_config_type * leds_config;

/****************************************************************************
NAME 
 ledsCheckEventForActiveFilter

DESCRIPTION
 This function checksif a filter has been configured for the given event, 
    if it has then activates / deactivates the filter 
    
    Regardless of whether a filter has been activated or not, the event is signalled as 
    completed as we have now deaklt with it (only checked for a filter if a pattern was not
    associated.

RETURNS
 void
 
*/      
void ledsCheckEventForActiveFilter( uint8 pEvent );


/****************************************************************************
NAME 
    ledsIndicateLedsPattern

DESCRIPTION
 	Given the indication type and leds pattern, Play the LED Pattern
RETURNS
    void
*/
void ledsIndicateLedsPattern(ledsPattern_t *lPattern, uint8 lIndex, leds_ind_type Ind_type);
        

/****************************************************************************
NAME 
 LedActiveFiltersCanOverideDisable

DESCRIPTION
    Check if active filters disable the global LED disable flag.
RETURNS
 	bool
*/
bool LedActiveFiltersCanOverideDisable(void);


/****************************************************************************
NAME 
 LedsIndicateNoState

DESCRIPTION
	remove any state indications as there are currently none to be displayed
RETURNS
 void
    
*/
void LedsIndicateNoState( void ) ;


/****************************************************************************
NAME 
 LedsResetAllLeds

DESCRIPTION
    resets all led pins to off and cancels all led and state indications
RETURNS
 void
*/
void LedsResetAllLeds( void ) ;


/****************************************************************************
NAME	
	LedsResetStateIndNumRepeatsComplete

DESCRIPTION
    Resets the LED Number of Repeats complete for the current state indication
       This allows the time of the led indication to be reset every time an event 
       occurs.
RETURNS
	void
    
*/
void LedsResetStateIndNumRepeatsComplete( void ) ;


/****************************************************************************
NAME 
    handleLedsInternalMessage

DESCRIPTION
    The main message handler for the LED task. Controls the PIO in question, then 
    queues a message back to itself for the next LED update.
*/ 
void handleLedsInternalMessage ( Task task , MessageId id, Message message );

/****************************************************************************
NAME 
    LedsIsStateMatched

DESCRIPTION
    To check whether the combined state is matched the state in the configured state
    
RETURN
    bool
*/ 
bool LedsIsStateMatched(uint8 config_state, uint8 state);

/****************************************************************************
NAME 
    LedsToPio

DESCRIPTION
    To get the PIO Pin number based on the configured leds value
    
RETURN
    uint8
*/ 
uint8 LedsToPioPin(uint8 leds);

#endif

