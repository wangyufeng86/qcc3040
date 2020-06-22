/****************************************************************************
Copyright (c) 2004 - 2015 Qualcomm Technologies International, Ltd.

FILE NAME
    leds_flash_manager_common.c
    
DESCRIPTION
	Function implementation file for the leds_flash manager plugin, 
    defining all of the functions for LED FLASH play.
*/
#include <stdlib.h>
#include <print.h>

#include "leds_flash.h"
#include "leds_flash_manager_common.h"

/* The leds information stored and used to ccontrol the currently playing led pattern */
typedef struct ledsInfoTag
{
    Task                 clientTask;
    unsigned             pState:8;
	unsigned             active_filter:8 ;
    unsigned             gLedsFlashEnabled:1;
}ledsInfo_t ;

/* static variables */
static ledsInfo_t              ledsInfo ;
static leds_flash_config_type *leds_config = NULL;

/****************************************************************************
Externl variables and functions
*/
extern bool ledsPlay ( uint8 pNewPattern ) ;

#define LEDS_OFF (0)

/****************************************************************************
  LOCAL FUNCTIONS
****************************************************************************/

/****************************************************************************
NAME 
    ledsHeadsetStateIndex

DESCRIPTION
 	check the headset state and get the state index
RETURNS
    void
*/
static bool ledsHeadsetStateIndex(uint8 pState, uint16 *index)
{
    uint16 i = 0;

    for (i = 0 ; i < ((leds_config->state_pattern)->num_states) ; i++ )
    {
        ledsStatePattern_t ledStatePattern = ((leds_config->state_pattern)->pattern[i]);
        uint8 lState                       = (ledStatePattern.state);

        PRINT(("The state in index [%d] is hfp_state [%x] a2dp_state [%x]\n", i, HFP_STATE(lState), A2DP_STATE(lState) ));
        
        if(HFP_STATE(lState) == HFP_STATE(pState))
        {
            if( (A2DP_FLAG(lState)  &&  A2DP_STATE(lState) != A2DP_STATE(pState)) || 
                (!A2DP_FLAG(lState) &&  A2DP_STATE(lState) == A2DP_STATE(pState))   )
            {
                *index = i;
                return TRUE;
            }
        }
    }  
    
    return FALSE;
}
        

/****************************************************************************
NAME 
    ledsCheckEventForActiveFilter

DESCRIPTION
 	check the event filter and set/remove the active bit of filters
RETURNS
    void
*/
static void ledsCheckEventForActiveFilter( uint8 pEvent )
{
    uint16 i = 0;
    for (i = 0 ; i < ((leds_config->event_filters)->num_filters) ; i++ )
    {
        ledsEventFilter_t ledEventFilter = ((leds_config->event_filters)->filter[i]);
        uint8 lEvent                     = (ledEventFilter.event);
        
        if(lEvent == pEvent)
        {
            ledsFilterIndex index = (ledEventFilter.index);
                
            PRINT(("The Event is [%x] for active filter, its index is [%x]", pEvent, index));
        
            switch(index)
            {
                case LEDS_FLASH_NORMAL:
                    ledsInfo.active_filter &=  (0x01 << LEDS_FLASH_MUTE);
                    if(ledEventFilter.IsFilterActive)
                        ledsInfo.active_filter |=  (0x01 << LEDS_FLASH_NORMAL);  /* enable the normal pattern */
                    else
                        ledsInfo.active_filter &= ~(0x01 << LEDS_FLASH_NORMAL);
                break;
                case LEDS_FLASH_LOWBATTERY:
                    ledsInfo.active_filter &=  (0x01 << LEDS_FLASH_MUTE);
                    if(ledEventFilter.IsFilterActive)
                        ledsInfo.active_filter |=  (0x01 << LEDS_FLASH_LOWBATTERY);
                    else
                        ledsInfo.active_filter &= ~(0x01 << LEDS_FLASH_LOWBATTERY);
                break;
                case LEDS_FLASH_FULLBATTERY:
                    ledsInfo.active_filter &=  (0x01 << LEDS_FLASH_MUTE);
                     if(ledEventFilter.IsFilterActive)
                        ledsInfo.active_filter |=  (0x01 << LEDS_FLASH_FULLBATTERY);
                    else
                        ledsInfo.active_filter &= ~(0x01 << LEDS_FLASH_FULLBATTERY);
                break;
                case LEDS_FLASH_MUTE:
                    if(ledEventFilter.IsFilterActive)
                        ledsInfo.active_filter |=  (0x01 << LEDS_FLASH_MUTE);
                    else
                        ledsInfo.active_filter &= ~(0x01 << LEDS_FLASH_MUTE);
                break;
            }
        }
    }
}


/****************************************************************************
NAME 
    LedsFlashInit

DESCRIPTION
 	Initialise the Leds ROM play system
RETURNS
    void
    
*/
void LedsFlashInit(Task clientTask, const void * params)
{
    ledsInfo.clientTask    = clientTask;
    leds_config            = (leds_flash_config_type *)params;
    ledsInfo.active_filter = (0x01 << LEDS_FLASH_NORMAL);    
    
    PRINT(("The LEDS Flash subsystem has been initialised\n"));
}

/****************************************************************************
NAME 
    LedsFlashIndicateEvent

DESCRIPTION
    displays event notification
    
    coverts headset user events into led patterns to be played

	makes use of the auto generated LED patterns.
	
	Patterns can be defined using the .led file and autogenerated by ledparse
	
	Once generated, patterns can be assigned to events in the ledStatePatterns
	table at the top of this file
    
RETURNS
    void 
*/
void LedsFlashIndicateEvent(uint8 pEvent, bool can_queue)
{
    if( ledsInfo.gLedsFlashEnabled )
    {
        uint16 i = 0 ;
	    for (i = 0 ; i < ((leds_config->event_pattern)->num_events) ; i++ )
	    {
            ledsEventPattern_t ledEventPattern = ((leds_config->event_pattern)->pattern[i]);
 	      	if( ledEventPattern.event == pEvent )
	    	{
                PRINT(("Play the LED event pattern of [%x]\n", pEvent));
	    		ledsPlay( ledEventPattern.pattern ) ;
	    	}
	    }
	
	    /*change the stored state based upon the events recieved - 
	      used to update the state indication*/
        ledsCheckEventForActiveFilter(pEvent);
    
	    /*the state may need changing after this event has been received*/
        /*Send a message to App and request a state indication */
        /*MessageSend(ledsInfo.clientTask, LEDS_STATE_IND_REQ, 0); */ 
        LedsFlashIndicateState(ledsInfo.pState);
    }
}

/****************************************************************************
NAME 
    LedsFlashIndicateState

DESCRIPTION
    displays state indication information

    coverts headset user states into led patterns to be played

	makes use of the auto generated LED patterns.
	
	Patterns can be defined using the .led file and autogenerated by ledparse
	
	Once generated, patterns can be assigned to states in the ledStatePatterns
	table at the top of this file
    
RETURNS
    void
    
*/
void LedsFlashIndicateState(uint8 pState)
{
    uint16 state_index = 0;
        
    PRINT(("Play the LED State pattern of Hfp:[%x], A2dp:[%x]\n", HFP_STATE(pState), A2DP_STATE(pState))); 
    
    if(ledsInfo.gLedsFlashEnabled && ledsHeadsetStateIndex(pState, &state_index))
    {
        ledsStatePattern_t ledStatePattern = ((leds_config->state_pattern)->pattern[state_index]);

        PRINT(("The LED State pattern has been configured\n"));
        
        /* Store the current headset state */
        ledsInfo.pState  = pState;
        
    	if (ledsInfo.active_filter & (0x01 << LEDS_FLASH_MUTE) )
    	{
    		ledsPlay( ledStatePattern.mute ) ;	
    	}
    
        if(ledsInfo.active_filter & (0x01 << LEDS_FLASH_LOWBATTERY) )
    	{
    		ledsPlay( ledStatePattern.low_battery ) ;
    	}
    	else if(ledsInfo.active_filter & (0x01 << LEDS_FLASH_FULLBATTERY) ) 
    	{
    		ledsPlay( ledStatePattern.full_battery ) ;
    	}
        else if(ledsInfo.active_filter & (0x01 << LEDS_FLASH_NORMAL) )
        {
            ledsPlay( ledStatePattern.normal) ;
        }
    }
    else
    {
        PRINT(("The LED State pattern has not been configured\n"));
    }
}


/****************************************************************************
NAME	
	LedsFlashEnableLEDS

DESCRIPTION
    Enable LED indications
RETURNS
	void
    
*/
void LedsFlashEnableLEDS(void)
{
    ledsInfo.gLedsFlashEnabled = TRUE ;
      
    MessageSend(ledsInfo.clientTask, LEDS_ENABLE_LEDS_CFM, 0);
    
    MessageSend(ledsInfo.clientTask, LEDS_STATE_IND_REQ, 0);
}

/****************************************************************************
NAME	
	LedsFlashDisableLEDS

DESCRIPTION
    Disable LED indications and turn off all current LED Indications if not 
    overidden by state or filter 
    
RETURNS
	void
*/
void LedsFlashDisableLEDS(void)
{    
    /*Turn off all current LED Indications */
    ledsInfo.gLedsFlashEnabled = FALSE ;
    
    ledsPlay(LEDS_OFF);
    
    MessageSend(ledsInfo.clientTask, LEDS_DISABLE_LEDS_CFM, 0);
}


