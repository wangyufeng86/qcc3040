/****************************************************************************
Copyright (c) 2005 - 2015 Qualcomm Technologies International, Ltd.

FILE NAME
    leds_flash_manager.c
DESCRIPTION
    an leds_flash manager for leds display
NOTES
*/
#include <stdlib.h>
#include <print.h>

#include "leds_manager.h"
#include "leds_manager_if.h"                /*the messaging interface*/
#include "leds_flash.h"
#include "leds_flash_manager_common.h"

/*the task message handler*/
static void flash_message_handler (Task task, MessageId id, Message message);

/*the leds_flash manager task*/
const TaskData leds_flash_manager = { flash_message_handler };


/****************************************************************************
DESCRIPTION
	The main task message handler
*/
static void flash_message_handler ( Task task, MessageId id, Message message ) 
{
	if ( (id >= LEDS_PLUGIN_MESSAGE_BASE ) && (id <= LEDS_PLUGIN_MESSAGE_TOP) )
	{
		switch (id)
	    {
		    case LEDS_INITIAL_MSG:
		    {
                LEDS_INITIAL_MSG_T * msg = (LEDS_INITIAL_MSG_T *)message ;
                LedsFlashInit(msg->clientTask, msg->params);
            }	
		    break ;
		    case LEDS_INDICATE_EVENT_MSG:
		    {
                LEDS_INDICATE_EVENT_MSG_T * msg = (LEDS_INDICATE_EVENT_MSG_T *)message ;
                LedsFlashIndicateEvent(msg->pEvent, msg->can_queue);
            }	
		    break ;
		    case LEDS_INDICATE_STATE_MSG:
		    {
                LEDS_INDICATE_STATE_MSG_T * msg = (LEDS_INDICATE_STATE_MSG_T *)message ;
                LedsFlashIndicateState(msg->pState);
            }
		    break ;
		    case LEDS_ENABLE_LEDS_MSG:
            {
                LedsFlashEnableLEDS();
            }
		    break ;
            case LEDS_DISABLE_LEDS_MSG:
            {
                LedsFlashDisableLEDS();
            }
		    break ;
		    case LEDS_RESET_EVENT_IND_MSG:
		    break ;
		    case LEDS_RESET_LEDS_TIMEOUT_MSG:
		    break ;		
		
		    default:
		    break ;
	    }
	}
}	




