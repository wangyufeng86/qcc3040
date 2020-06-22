/****************************************************************************
Copyright (c) 2005 - 2015 Qualcomm Technologies International, Ltd.

FILE NAME
    leds_rom_manager.c
DESCRIPTION
    an leds_rom manager for leds display
NOTES
*/

#include <stdlib.h>
#include <print.h>

#include "leds_manager.h"
#include "leds_manager_if.h"                /*the messaging interface*/
#include "leds_rom.h"
#include "leds_rom_manager_common.h"

/*the task message handler*/
static void rom_message_handler (Task task, MessageId id, Message message);

/*the local message handling functions*/
static void handleLedsManagerMessage ( Task task , MessageId id, Message message );

static void LedsRomInit(Task clientTask, const void * params);
static void LedsRomIndicateEvent(uint8 pEvent, bool can_queue);
static void LedsRomIndicateState(uint8 pState);
static void LedsRomEnableLEDS(void);
static void LedsRomDisableLEDS(void);
static void LedsRomResetLEDEventInds(void);
static void LedsRomCheckTimeoutState(void);

/*the led_rom manager task*/
const TaskData leds_rom_manager = { rom_message_handler };

/****************************************************************************
DESCRIPTION
	The main task message handler
*/
static void rom_message_handler ( Task task, MessageId id, Message message ) 
{
	if ( (id >= LEDS_PLUGIN_MESSAGE_BASE ) && (id <= LEDS_PLUGIN_MESSAGE_TOP) )
	{
		handleLedsManagerMessage (task , id, message ) ;
	}
	else
	{
        PRINT(("LM: The id is [%x] from internal rom library\n", id));
		handleLedsInternalMessage (task , id , message ) ;
	}
}	

/****************************************************************************
DESCRIPTION

	messages from the leds manager library are received here. 
	and converted into function calls to be implemented in the 
	leds rom manager module
*/ 
static void handleLedsManagerMessage ( Task task , MessageId id, Message message ) 	
{
	switch (id)
	{
		case (LEDS_INITIAL_MSG):
		{
            LEDS_INITIAL_MSG_T * msg = (LEDS_INITIAL_MSG_T *)message ;
            PRINT(("LM: LEDS_ROM_MANAGER: LEDS_INITIAL_MSG\n"));
            LedsRomInit(msg->clientTask, msg->params);
		}	
		break ;
		
		case (LEDS_INDICATE_EVENT_MSG):
		{
            LEDS_INDICATE_EVENT_MSG_T * msg = (LEDS_INDICATE_EVENT_MSG_T *)message ;
            PRINT(("LEDS_ROM_MANAGER: LEDS_INDICATE_EVENT_MSG \n"));
            LedsRomIndicateEvent(msg->pEvent, msg->can_queue);
        }	
		break ;
		
		case (LEDS_INDICATE_STATE_MSG):
		{
            LEDS_INDICATE_STATE_MSG_T * msg = (LEDS_INDICATE_STATE_MSG_T *)message ;
            PRINT(("LEDS_ROM_MANAGER: LEDS_INDICATE_STATE_MSG \n"));
            LedsRomIndicateState(msg->pState);
		}
		break ;
		
		case (LEDS_ENABLE_LEDS_MSG): 
		{
            PRINT(("LEDS_ROM_MANAGER: LEDS_ENABLE_LEDS_MSG \n"));
            LedsRomEnableLEDS();
		}		
		break ;
		
        case (LEDS_DISABLE_LEDS_MSG):
		{
            PRINT(("LEDS_ROM_MANAGER: LEDS_DISABLE_LEDS_MSG \n"));
            LedsRomDisableLEDS();
		}		
		break ;
		
        case (LEDS_RESET_EVENT_IND_MSG):
		{
            PRINT(("LEDS_ROM_MANAGER: LEDS_RESET_EVENT_IND_MSG \n"));
            LedsRomResetLEDEventInds( );
		}
		break ;
		
		case (LEDS_RESET_LEDS_TIMEOUT_MSG):
		{
            PRINT(("LEDS_ROM_MANAGER: LEDS_RESET_LEDS_TIMEOUT_MSG \n"));
            LedsRomCheckTimeoutState( );
		}
		break ;		
		default:
		{
		}
		break ;
	}
}

/****************************************************************************
NAME 
    LedsRomInit

DESCRIPTION
 	Initialise the Leds ROM play system
RETURNS
    void
    
*/
static void LedsRomInit(Task clientTask, const void * params)
{
    leds_config = (leds_config_type *)params;
    
    leds_rom_ctl.clientTask = clientTask;    
}

/****************************************************************************
NAME 
    LedsRomIndicateEvent

DESCRIPTION
    displays event notification
    This function also enables / disables the event filter actions - if a normal event indication is not
    associated with the event, it checks to see if a filer is set up for the event 
    
RETURNS
    void 
*/
static void LedsRomIndicateEvent(uint8 pEvent, bool can_queue)
{
    uint8 i,lPatternIndex;
    ledsPattern_t *lPattern = NULL;
 
    lPatternIndex = NO_STATE_OR_EVENT;
        
    /* search for a matching event */
	for(i=0;i< leds_config->gNumEventPatternsUsed;i++)
    {
        PRINT(("LM: The config event:[%x], curr event [%x]\n", leds_config->gLedsEventPatterns[i].EventOrState, pEvent));
            
        if(leds_config->gLedsEventPatterns[i].EventOrState == pEvent)
        {
            lPattern = &leds_config->gLedsEventPatterns[i];
            lPatternIndex = i;
            break;
        }
    }
        
    /*if there is an event configured*/
    if ( lPatternIndex != NO_STATE_OR_EVENT )
    {        
        /*only indicate if LEDs are enabled*/
        if ((leds_config->gLedsRomEnabled ) ||
            (lPattern->OverideDisable )     ||
            LedActiveFiltersCanOverideDisable( ))
        {
            /*only update if wer are not currently indicating an event*/
            if ( !leds_rom_ctl.gCurrentlyIndicatingEvent )
            {
                /*Indicate the LED Pattern of Event/State*/
                ledsIndicateLedsPattern(lPattern, lPatternIndex, LEDS_IND_EVENT);
            }    
            else
            {
                if(leds_config->gLedsQueueLEDEvents && can_queue)
                {
                    PRINT(("LM: Queue LED Event [%x]\n" , pEvent )) ;
					
					if( leds_config->gQueue.Event1 == 0)
                    {
                        leds_config->gQueue.Event1 = pEvent;
                    }
                    else if( leds_config->gQueue.Event2 == 0)
                    {
                        leds_config->gQueue.Event2 = pEvent;
                    }
                    else if( leds_config->gQueue.Event3 == 0)
                    {
                        leds_config->gQueue.Event3 = pEvent;
                    }
                    else if( leds_config->gQueue.Event4 == 0)
                    {
                        leds_config->gQueue.Event4 = pEvent;
                    }
                    else
                    {
                        PRINT(("LM: Err Queue Full!!\n")) ;
                    }
                }    
            }
        }
        else
        {
            PRINT(("LM : No IE[%x] disabled\n",pEvent )) ;
        }
    }
    else
    {
        PRINT(("LM: NoEvPatCfg\n")) ;
    }
   
    /*indicate a filter if there is one present*/
    ledsCheckEventForActiveFilter( pEvent ) ;
}

/****************************************************************************
NAME 
    LedsRomIndicateState

DESCRIPTION
    displays state indication information

RETURNS
    void
*/
static void LedsRomIndicateState(uint8 pState)
{
    uint8 i,lPatternIndex;   
    ledsPattern_t *lPattern = NULL;
    
    lPatternIndex = NO_STATE_OR_EVENT;
    
    /* search for a matching state */
	for(i=0;i<leds_config->gNumStatePatternsUsed;i++)
    {
        if(LedsIsStateMatched(leds_config->gLedsStatePatterns[i].EventOrState, pState))
        {
            lPattern = &leds_config->gLedsStatePatterns[i];
            lPatternIndex = i;
            break;
        }
    }

    /* force indicated state to that of Low Battery configured pattern */ 
    leds_rom_ctl.pStatePatternIndex = NO_STATE_OR_EVENT;
            
    if(lPatternIndex != NO_STATE_OR_EVENT)
    {
        /*if there is a pattern associated with the state and not disabled, indicate it*/
        leds_rom_ctl.pStatePatternIndex      = lPatternIndex;
        leds_rom_ctl.gStateCanOverideDisable = lPattern->OverideDisable;
            
        /* only indicate if LEDs are enabled*/
        if ((leds_config->gLedsRomEnabled ) ||
            (lPattern->OverideDisable )     ||
            LedActiveFiltersCanOverideDisable( ))
        {
            uint8 LED_A_PIO = LedsToPioPin(lPattern->LED_A);
            uint8 LED_B_PIO = LedsToPioPin(lPattern->LED_B);
            uint8 LED_C_PIO = LedsToPioPin(lPattern->LED_C);
            
            if (    ( leds_config->gActiveLEDS[LED_A_PIO].Type != LEDS_IND_EVENT  )
                 && ( leds_config->gActiveLEDS[LED_B_PIO].Type != LEDS_IND_EVENT  ) 
                 && ( leds_config->gActiveLEDS[LED_C_PIO].Type != LEDS_IND_EVENT  ) )
            {            
                ledsIndicateLedsPattern(lPattern, lPatternIndex, LEDS_IND_STATE);
            }
        }
        else
        {
            LedsIndicateNoState ( ) ; 
        }
    }
    else
    {
        PRINT(("LM : DIS NoStCfg[%x]\n", pState)) ;
        LedsIndicateNoState ( );
    }    
}

/****************************************************************************
NAME	
	LedsRomEnableLEDS

DESCRIPTION
    Enable LED indications
RETURNS
	void
    
*/
static void LedsRomEnableLEDS(void)
{
    leds_config->gLedsRomEnabled = TRUE ;
      
    MessageSend(leds_rom_ctl.clientTask, LEDS_ENABLE_LEDS_CFM, 0);
}

/****************************************************************************
NAME	
	LedsRomDisableLEDS

DESCRIPTION
    Disable LED indications and turn off all current LED Indications if not 
    overidden by state or filter 
    
RETURNS
	void
*/
static void LedsRomDisableLEDS(void)
{    
    /*Turn off all current LED Indications if not overidden by state or filter */
    if (!leds_rom_ctl.gStateCanOverideDisable && !LedActiveFiltersCanOverideDisable())
    {
        LedsIndicateNoState ( ) ;
    }    
    
    leds_config->gLedsRomEnabled = FALSE ;
    
    MessageSend(leds_rom_ctl.clientTask, LEDS_DISABLE_LEDS_CFM, 0);
}

/****************************************************************************
NAME	
	LedsRomResetLEDEventInds

DESCRIPTION
    Resets the LED event Indications and reverts to state indications
	Sets the Flag to allow the Next Event to interrupt the current LED Indication
    Used if you have a permanent LED event indication that you now want to interrupt.
    
    
RETURNS
	void
*/
static void LedsRomResetLEDEventInds(void)
{
    LedsResetAllLeds ( ) ;
    
    leds_rom_ctl.gCurrentlyIndicatingEvent = FALSE ;
    
    MessageSend(leds_rom_ctl.clientTask, LEDS_STATE_IND_REQ, 0);
}

/****************************************************************************
NAME	
	LedsRomCheckTimeoutState

DESCRIPTION
    checks the led timeout state and resets it if required, this function is called from
    an event or volume button press to re-enable led indications as and when required
    to do so 
RETURNS
	void
    
*/
static void LedsRomCheckTimeoutState(void)
{
    /*handles the LED event timeouts - restarts state indications if we have had a user generated event only*/
    if (leds_config->gLEDSStateTimeout)
    {   
        MAKE_LEDS_MESSAGE(LEDS_STATE_IND_TIMEOUT);
        message->status = FALSE;
        /* send message that can be used to show an led pattern when led's are re-enabled following a timeout */
        MessageSend( leds_rom_ctl.clientTask, LEDS_STATE_IND_TIMEOUT, message);
        
        leds_config->gLEDSStateTimeout = FALSE;
    }
    else
    {
        /*reset the current number of repeats complete - i.e restart the timer so that the leds will disable after
          the correct time*/
        LedsResetStateIndNumRepeatsComplete( ) ;
    }
}


