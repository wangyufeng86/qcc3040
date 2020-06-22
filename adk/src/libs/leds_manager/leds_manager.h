/****************************************************************************
Copyright (c) 2004 - 2015 Qualcomm Technologies International, Ltd.

FILE NAME
    leds_manager.h
    
DESCRIPTION
    header file for the leds manager library	
    
    This defines the Application Programming interface to the leds library.
    
    i.e. the interface between the VM application and the leds library.
        
    see the leds_manager_if documentation for details of the API between the 
    leds manager library and an underlying leds manager plugin (leds_rom/leds_flash).
*/

#ifndef _LEDS_MANAGER_H_
#define _LEDS_MANAGER_H_

#include <message.h>

/* The following messages are sent from leds manager/plugins to apps */
#define LEDS_MESSAGE_BASE	0x6700

typedef enum
{
	LEDS_INITIAL_CFM = LEDS_MESSAGE_BASE,
    LEDS_EVENT_IND_CFM,
    LEDS_STATE_IND_REQ,  
    LEDS_STATE_IND_TIMEOUT,
    LEDS_ENABLE_LEDS_CFM,
    LEDS_DISABLE_LEDS_CFM,
	LEDS_MESSAGE_TOP
} LedsPluginMessageId;

/*the message sent on completeion of an event */
typedef struct 
{
    bool   status ;                 /* 0: not complete; 1: complete */
} LEDS_STATE_IND_TIMEOUT_T;

/*the message sent on completeion of an event */
typedef struct 
{
    uint8  pEvent ;  
    bool   status ;                 /* 0: not complete; 1: complete */
} LEDS_EVENT_IND_CFM_T;


/*!
    @brief This message is returned when the leds subsystem has been initialised.
*/
typedef struct
{
  bool status;
} LEDS_INITIAL_CFM_T;


/****************************************************************************
NAME 
    LedsInit

DESCRIPTION
 	Initialise the Leds data; 
    
    LEDS_INITIAL_CFM message will return to App to indicate the status of initialisation
RETURNS
    void
    
*/
void LedsInit(Task leds_plugin, const void * config, Task clientTask) ;


/****************************************************************************
NAME 
    LedsIndicateEvent

DESCRIPTION
    displays event notification
    
    LEDS_EVENT_IND_CFM message will return to App when the Led display has finished
    
RETURNS
    void 
*/
void LedsIndicateEvent ( uint8 pEvent, bool can_queue ) ;

/****************************************************************************
NAME 
    LedsIndicateState

DESCRIPTION
    displays state indication information

RETURNS
    void
    
*/
void LedsIndicateState (uint8 pHfpState, uint8 pA2dpState ) ;


/****************************************************************************
NAME	
	LedsDisableLEDS

DESCRIPTION
    Disable LED indications and turn off all current LED Indications if not 
    overidden by state or filter 
    
RETURNS
	void
*/

void LedsDisableLEDS ( void );


/****************************************************************************
NAME	
	LedsEnableLEDS

DESCRIPTION
    Enable LED indications
RETURNS
	void
    
*/
void LedsEnableLEDS ( void );


/****************************************************************************
NAME	
	LedsResetLEDEventInd

DESCRIPTION
    Resets the LED event Indications and reverts to state indications
	Sets the Flag to allow the Next Event to interrupt the current LED Indication
    Used if you have a permanent LED event indication that you now want to interrupt.
    
    
RETURNS
	void
    
*/
void LedsResetLEDEventInd ( void ) ;


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

void LedsResetLEDTimeout( void );
	

#endif    /* _LEDS_MANAGER_H_ */

