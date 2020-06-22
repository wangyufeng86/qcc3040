/****************************************************************************
Copyright (c) 2004 - 2015 Qualcomm Technologies International, Ltd.

FILE NAME
	leds_manager.c        
	
DESCRIPTION
     The main leds manager file
*/ 
#include "leds_manager.h"
#include "leds_manager_if.h"

#include <print.h>

typedef struct leds_Tag
{
    Task 	 plugin ;    
}LEDS_t ;


/*The global leds manager library data structure to store which
  plugin is selected for leds display 
*/
static LEDS_t LEDS = {0} ;


/****************************************************************************
NAME 
    LedsInit

DESCRIPTION
 	Initialise the Leds data
RETURNS
    void
    
*/
void LedsInit(Task leds_plugin, const void * config, Task clientTask) 
{
    /*Send an message to the leds plugin */
	MAKE_LEDS_MESSAGE(LEDS_INITIAL_MSG) ;
    
	PRINT(("LEDS_MANAGER: LedsInit\n"));
    
	message->clientTask = clientTask ;
	message->params     = config ;

    MessageSend(leds_plugin, LEDS_INITIAL_MSG, message );    
    
    /*Store the leds plugin*/
    LEDS.plugin     = leds_plugin;
}

/****************************************************************************
NAME 
    LedsIndicateEvent

DESCRIPTION
    displays event notification
RETURNS
    void
*/

void LedsIndicateEvent ( uint8 pEvent, bool can_queue )
{
    /* send an message to the leds plugin */
    MAKE_LEDS_MESSAGE(LEDS_INDICATE_EVENT_MSG) ;
    
	PRINT(("LEDS_MANAGER: LedsIndicateEvent [%x]\n", pEvent));
           
	message->pEvent    = pEvent ;
	message->can_queue = can_queue ;

    MessageSend(LEDS.plugin, LEDS_INDICATE_EVENT_MSG, message ); 
}


/****************************************************************************
NAME 
    LedsIndicateState

DESCRIPTION
    displays state indication information

RETURNS
    void
    
*/
void LedsIndicateState (uint8 pHfpState, uint8 pA2dpState ) 
{
    uint8 pState = COMBINED_HEADSET_STATE(pHfpState, pA2dpState);

    PRINT(("LEDS_MANAGER: The combined headset state is [%x], Hfp State [%x], A2dp state [%x]\n", pState, pHfpState, pA2dpState));
    {
        MAKE_LEDS_MESSAGE(LEDS_INDICATE_STATE_MSG) ;
	    message->pState    = pState ;
        MessageSend(LEDS.plugin, LEDS_INDICATE_STATE_MSG, message );   
    } 
}


/****************************************************************************
NAME	
	LedsDisableLEDS

DESCRIPTION
    Disable LED indications and turn off all current LED Indications if not 
    overidden by state or filter  
    
RETURNS
	void
*/
void LedsDisableLEDS ( void )
{
    PRINT(("LEDS_MANAGER: Disable LEDS \n"));
    /* Send an message to the leds plugin */
    MessageSend(LEDS.plugin, LEDS_DISABLE_LEDS_MSG, 0 ); 
}


/****************************************************************************
NAME	
	LedsEnableLEDS

DESCRIPTION
    Enable LED indications
RETURNS
	void
    
*/
void LedsEnableLEDS ( void )
{
    PRINT(("LEDS_MANAGER: Enable LEDS \n"));
    /* Send an message to the leds plugin */
    MessageSend(LEDS.plugin, LEDS_ENABLE_LEDS_MSG, 0 ); 
}


/****************************************************************************
NAME	
	LedsResetLEDEventInd

DESCRIPTION
    Resets the LED event Indications and reverts to state indications
	  Sets the Flag to allow the Next Event to interrupt the current LED Indication
    Used if you have a permanent LED event indication that you now want to interrupt
RETURNS
	void
    
*/
void LedsResetLEDEventInd ( void )
{
    PRINT(("LEDS_MANAGER: LedsResetLEDEventInd\n"));
    /* Send an message to the leds plugin */
    MessageSend(LEDS.plugin, LEDS_RESET_EVENT_IND_MSG, 0 ); 
}


/****************************************************************************
NAME	
	LedsResetLEDTimeout

DESCRIPTION
    checks the led timeout state and resets it if required, this function is called from
    an event or volume button press to re-enable led indications as and when required
    to do so 
RETURNS
	void
    
*/
void LedsResetLEDTimeout( void )
{
    PRINT(("LEDS_MANAGER: LedsResetLEDTimeout\n"));
    /* Send an message to the leds plugin */
    MessageSend(LEDS.plugin, LEDS_RESET_LEDS_TIMEOUT_MSG, 0 );
}






