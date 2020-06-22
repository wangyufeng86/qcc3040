/****************************************************************************
Copyright (c) 2004 - 2015 Qualcomm Technologies International, Ltd.

FILE NAME
    leds_rom_manager_common.c
    
DESCRIPTION
	Function implementation file for the leds_rom manager plugin, 
    defining all of the functions for LED ROM play.
*/

#include <string.h>
#include <pio.h>
#include <led.h>

#include <print.h>

#include "leds_rom.h"
#include "leds_rom_manager_common.h"
#include "leds_manager_if.h"                      /*the messaging interface*/

#define LED_SCALE_ON_OFF_TIME(x) (uint16)((x * 10) << (leds_config->gLedsTimerMultiplier) )
#define LED_SCALE_REPEAT_TIME(x) (uint16)((x * 50) << (leds_config->gLedsTimerMultiplier) )
#define FILTER_SCALE_DELAY_TIME(x) (uint16)(x << (leds_config->gLedsTimerMultiplier) )

static void PioSetLed ( uint16 pPIO , bool pOnOrOff );
static void PioSetPio ( uint16 pPIO , bool pOnOrOff );

static uint16 LedsApplyFilterToTime     ( uint16 pTime )  ;
static leds_colour LedsGetPatternColour ( const ledsPattern_t * pPattern ) ;

 /*helper functions to change the state of LED pairs depending on the pattern being played*/
static void LedsTurnOffLEDPair ( ledsPattern_t * pPattern, bool pUseOveride  ) ;
static void LedsTurnOnLEDPair  ( ledsPattern_t * pPattern, leds_activity * pLED );
static bool isOverideFilterActive ( uint8 Led );

/*method to complete an event*/
static void LedsEventComplete ( leds_activity * pPrimaryLed , leds_activity * pSecondaryLed, leds_activity * pThirdLed ) ;

/*method to indicate that an event has been completed*/
static void LedsSendEventComplete ( uint8 pEvent , bool pPatternCompleted ) ;
static void LedsSetLedActivity ( leds_activity * pLed , leds_ind_type pType , uint16 pIndex , uint16 pDimTime);

/*filter enable - check methods*/
static bool LedsIsFilterEnabled ( uint16 pFilter ) ;
static void LedsEnableFilter ( uint16 pFilter , bool pEnable) ;
static void LedsHandleOverideLED ( bool pOnOrOff ) ;

/*Follower LED helper functions*/
static bool LedsCheckFiltersForLEDFollower( void ) ;
static uint16 LedsGetLedFollowerRepeatTimeLeft( ledsPattern_t * pPattern) ;
static uint16 LedsGetLedFollowerStartDelay( void ) ;

static void LedsSetEnablePin ( bool pOnOrOff ) ;
static void LedsTurnOnOffLeds(uint16 ledspattern);

/* define the pattern constant */
static const uint16 LedsOnOffPattern[MAX_LEDS_PATTERN][3] = {
    {0x0108, 0x0000, 0x0000},    /* LED_COL_EITHER */
    {0x0908, 0x0000, 0x0000},    /* LED_COL_LED_A  */
    {0x2808, 0x0000, 0x0000},    /* LED_COL_LED_B */
    {0x0908, 0x2808, 0x0000},    /* LED_COL_LED_A_B_ALT */
    {0x0948, 0x0000, 0x0000},    /* LED_COL_LED_A_B_BOTH */
    {0x4804, 0x0000, 0x0000},    /* LED_COL_LED_C */
    {0x0908, 0x4804, 0x0000},    /* LED_COL_LED_A_C_ALT */
    {0x2808, 0x4804, 0x0000},    /* LED_COL_LED_B_C_ALT */
    {0x0A44, 0x0000, 0x0000},    /* LED_COL_LED_A_C_BOTH */
    {0x2A40, 0x0000, 0x0000},    /* LED_COL_LED_B_C_BOTH */
    {0x0948, 0x4804, 0x0000},    /* LED_COL_LED_AB_C_ALT */
    {0x0A44, 0x2808, 0x0000},    /* LED_COL_LED_AC_B_ALT */
    {0x0908, 0x2A40, 0x0000},    /* LED_COL_LED_A_BC_ALT */
    {0x0908, 0x2808, 0x4804},    /* LED_COL_LED_A_B_C_ALT */
    {0x0949, 0x0000, 0x0000}     /* LED_COL_LED_A_B_C_ALL */
};


/* Define the ALT flag constant for LED colour pattern */
static const bool LedsColorAltPattern[MAX_LEDS_PATTERN] = {
    FALSE, FALSE, FALSE, TRUE, FALSE, FALSE, TRUE, TRUE, FALSE, FALSE, TRUE, TRUE, TRUE, TRUE, FALSE
};

/* Global Variables */
leds_ctrl_info leds_rom_ctl;
leds_config_type * leds_config = NULL;

/****************************************************************************
  LOCAL FUNCTIONS
****************************************************************************/

/****************************************************************************
NAME	
	PioSetPio

DESCRIPTION
    Fn to change a PIO
    
RETURNS
	void
*/
static void PioSetPio ( uint16 pPIO , bool pOnOrOff  ) 
{
    uint16 lPinVals = 0 ;
    uint16 lWhichPin  = (1<< pPIO) ;
    
    if ( pOnOrOff )    
    {
        lPinVals = lWhichPin  ;
    }
    else
    {
        lPinVals = 0x0000;/*clr the corresponding bit*/
    }

        /*(mask,bits) setting bit to a '1' sets the corresponding port as an output*/
    PioSetDir32( lWhichPin , lWhichPin );   
    	/*set the value of the pin*/         
    PioSet32( lWhichPin , lPinVals ) ;     
}


/****************************************************************************
NAME	
	PioSetLed

DESCRIPTION
   Internal fn to change set an LED attached to a PIO or a special LED pin 
    
RETURNS
	void
*/
static void PioSetLed ( uint16 pPIO , bool pOnOrOff ) 
{	
    leds_activity *gActiveLED = &(leds_config->gActiveLEDS[pPIO]);

   /* LED pins are special cases*/
    if (pPIO == 14)        
    {
        if ( gActiveLED->DimTime > 0 ) /*if this is a dimming led / pattern*/ 
        {
            if (leds_rom_ctl.gLED_0_STATE != pOnOrOff) /*if the request is to do the same as what we are doing then ignore*/
            {
                    /*set led to max or min depending on whether we think the led is on or off*/
                gActiveLED->DimState = (DIM_NUM_STEPS * !pOnOrOff) ; 
                gActiveLED->DimDir   = pOnOrOff ; /*1=go up , 0 = go down**/ 

                LedConfigure(LED_0, LED_DUTY_CYCLE, (gActiveLED->DimState * (DIM_STEP_SIZE)));
                LedConfigure(LED_0, LED_PERIOD, DIM_PERIOD );
                LedConfigure(LED_0, LED_ENABLE, TRUE);
                
                /*send the first message*/
                MessageCancelAll ( (TaskData *)&leds_rom_manager, (DIM_MSG_BASE + pPIO) ) ;                
                MessageSendLater ( (TaskData *)&leds_rom_manager, (DIM_MSG_BASE + pPIO) ,0 ,gActiveLED->DimTime ) ;

                leds_rom_ctl.gLED_0_STATE = pOnOrOff ;
            }
        }
        else
        {              
		    LedConfigure(LED_0, LED_ENABLE, pOnOrOff ) ;
            LedConfigure(LED_0, LED_DUTY_CYCLE, (0xfff));
            LedConfigure(LED_0, LED_PERIOD, DIM_PERIOD );
            
            leds_rom_ctl.gLED_0_STATE = pOnOrOff ;
        }
    }
    else if (pPIO == 15 )
    {
        if ( gActiveLED->DimTime > 0 ) /*if this is a dimming led / pattern*/ 
        {
            if (leds_rom_ctl.gLED_1_STATE != pOnOrOff) /*if the request is to do the same as what we are doing then ignore*/
            {
                   /*set led to max or min depending on whether we think the led is on or off*/
                gActiveLED->DimState = (DIM_NUM_STEPS * !pOnOrOff) ; 
                gActiveLED->DimDir   = pOnOrOff ; /*1=go up , 0 = go down**/ 
                                    
                LedConfigure(LED_1, LED_DUTY_CYCLE, (gActiveLED->DimState * (DIM_STEP_SIZE)));
                LedConfigure(LED_1, LED_PERIOD, DIM_PERIOD );
                LedConfigure(LED_1, LED_ENABLE, TRUE);
                   
                /*send the first message*/
                MessageCancelAll ( (TaskData *)&leds_rom_manager, (DIM_MSG_BASE + pPIO) ) ;                
                MessageSendLater ( (TaskData *)&leds_rom_manager, (DIM_MSG_BASE + pPIO) ,0 ,gActiveLED->DimTime ) ;

                leds_rom_ctl.gLED_1_STATE = pOnOrOff ;                                  
            }
        }
        else
        {
            LedConfigure(LED_1, LED_ENABLE, pOnOrOff ) ;
            LedConfigure(LED_1, LED_DUTY_CYCLE, (0xfff));
            LedConfigure(LED_1, LED_PERIOD, DIM_PERIOD );
            
            leds_rom_ctl.gLED_1_STATE = pOnOrOff ;
        }
    }
    else
    {
        PioSetPio (pPIO , pOnOrOff) ;
    }
}


/****************************************************************************
NAME	
	PioSetDimState  
	
DESCRIPTION
    Update funtion for a led that is currently dimming
    
RETURNS
	void
*/
static void PioSetDimState ( uint16 pPIO )
{
    uint16 lDim = 0x0000 ;
    leds_activity *gActiveLED = &(leds_config->gActiveLEDS[pPIO]);
    
    if (gActiveLED->DimDir && gActiveLED->DimState >= DIM_NUM_STEPS )
    {      
        lDim = 0xFFF;
    }
    else if ( !gActiveLED->DimDir && gActiveLED->DimState == 0x0 )
    {
        lDim = 0 ;
    }
    else
    {
        if(gActiveLED->DimDir)
            gActiveLED->DimState++ ;
        else
            gActiveLED->DimState-- ;
        
        lDim = (gActiveLED->DimState * (DIM_STEP_SIZE) ) ;
        
        MessageCancelAll ( (TaskData *)&leds_rom_manager, (DIM_MSG_BASE + pPIO) ) ;                
        MessageSendLater ( (TaskData *)&leds_rom_manager, (DIM_MSG_BASE + pPIO) , 0 , gActiveLED->DimTime ) ;    
    }
  
    if (pPIO == 14)
    {
        LedConfigure(LED_0, LED_DUTY_CYCLE, lDim);
        LedConfigure(LED_0, LED_PERIOD, DIM_PERIOD );  
        LedConfigure(LED_0, LED_ENABLE, TRUE ) ;
    }
    else if (pPIO == 15)
    {
        LedConfigure(LED_1, LED_DUTY_CYCLE, lDim);
        LedConfigure(LED_1, LED_PERIOD, DIM_PERIOD );  
        LedConfigure(LED_1, LED_ENABLE, TRUE ) ;
    }
}


/****************************************************************************
NAME 
 LedsTurnOnOffLeds

DESCRIPTION
    Fn to turn On/Off the LEDs with LED colour pattern
    
RETURNS
 void
*/
static void LedsTurnOnOffLeds(uint16 ledsPattern)
{
    uint8 LedA = LedsToPioPin((ledsPattern & LEDSA_MASK) >> LEDSA_SHIFT); 
    uint8 LedB = LedsToPioPin((ledsPattern & LEDSB_MASK) >> LEDSB_SHIFT);
    uint8 LedC = LedsToPioPin((ledsPattern & LEDSC_MASK) >> LEDSC_SHIFT);
    
    bool LedAOnorOff = ((ledsPattern & LEDSA_ONOFF_MASK) >> LEDSA_ONOFF_SHIFT);
    bool LedBOnorOff = ((ledsPattern & LEDSB_ONOFF_MASK) >> LEDSB_ONOFF_SHIFT);
    bool LedCOnorOff = ((ledsPattern & LEDSC_ONOFF_MASK) >> LEDSC_ONOFF_SHIFT);
    
    PRINT(("LED_ROM_COMMON: A [%x] ON[%s] B [%x] ON[%s] C [%x] ON[%s]\n", LedA ,(LedAOnorOff)?"Y":"N", LedB, (LedBOnorOff)?"Y":"N", LedC, (LedCOnorOff)?"Y":"N")) ;
    
    /* For LedA, if any pattern has some LED_ON, put them as LedA */
    if(LedAOnorOff)
        PioSetLed ( LedA , LED_ON );
    else if(!isOverideFilterActive(LedA))
    {
        PioSetLed ( LedA , LED_OFF )  ;
    }
    /* For LedB */
    if(LedB != LedA)
    {
        if(LedBOnorOff)
            PioSetLed ( LedB , LED_ON );
        else if(!isOverideFilterActive(LedB))
        {
            PioSetLed ( LedB , LED_OFF )  ;
        }
    }
    /* For LedC */    
    if(LedC != LedA && LedC != LedB)
    {
        if(LedCOnorOff)
            PioSetLed ( LedC , LED_ON );
        else if(!isOverideFilterActive(LedC))
        {
            PioSetLed ( LedC , LED_OFF )  ;
        }
    }
}

/****************************************************************************
NAME 
 LMTurnOnLEDPair

DESCRIPTION
    Fn to turn on the LED associated with the pattern / LEDs depending upon the 
    colour 
    
RETURNS
 void
*/
static void LedsTurnOnLEDPair ( ledsPattern_t * pPattern , leds_activity * pLED )
{
    leds_colour lColour = LedsGetPatternColour ( pPattern ) ;   
    uint8 LedA          = LedsToPioPin(pPattern->LED_A); 
    
    /* to prevent excessive stack usage when compiled in native mode only perform one read and convert of these
       4 bit parameters */
    if ( leds_rom_ctl.gFollowing )
    {	 
        /*turn of the pair of leds (and dont use an overide LED */
        /* obtain the PIO to drive */
        uint16 lLED = leds_rom_ctl.gFollowPin; 
                
        LedsTurnOffLEDPair ( pPattern , FALSE) ;
        
        /* check to ensure it was possible to retrieve PIO, the filter may have been cancelled */
        if(lLED <= leds_config->gNumActiveLeds)
        {
            /* set the LED specified in the follower filter */
            PioSetLed ( lLED , LED_ON );
        }
    }
    else
    {   /*we are not following*/
            /*Turn on the LED enable pin*/    
        LedsSetEnablePin ( LED_ON )  ;
        
        PRINT(("LED_ROM_COMMON: Color [%x]\n", lColour));
        
        switch (lColour )
        {
        case LED_COL_LED_A:
        case LED_COL_LED_B:
        case LED_COL_LED_A_B_BOTH:
        case LED_COL_LED_C:
        case LED_COL_LED_A_C_BOTH:
        case LED_COL_LED_B_C_BOTH:
        case LED_COL_LED_A_B_C_ALL:
            LedsTurnOnOffLeds(LedsOnOffPattern[lColour][0]);
        break; 
        case LED_COL_LED_A_B_ALT:
        case LED_COL_LED_A_C_ALT:
        case LED_COL_LED_B_C_ALT:
        case LED_COL_LED_AB_C_ALT:
        case LED_COL_LED_AC_B_ALT:
        case LED_COL_LED_A_BC_ALT:
            LedsTurnOnOffLeds(LedsOnOffPattern[lColour][pLED->NumFlashesComplete % 2]);
        break;
        case LED_COL_LED_A_B_C_ALT:
            LedsTurnOnOffLeds(LedsOnOffPattern[lColour][pLED->NumFlashesComplete % 3]);
        break;
        default:
            PRINT(("LED_ROM_COMMON: ?Col\n")) ;
        break;

        }
    }
 
    /* only process the overide filter if not an alternating pattern or a permanently on pattern otherwise
       led will be turned off */
    if((!LedsColorAltPattern[lColour]) && (pPattern->NumFlashes))
    {
        /*handle an overide LED if there is one will also dealit is different to one of the pattern LEDS)*/
        if(!isOverideFilterActive(LedA))
        {
            PRINT(("LED_ROM_COMMON: TurnOnPair - Handle Overide\n" )) ;
            LedsHandleOverideLED ( LED_OFF ) ;
        }
    }
    
    pLED->OnOrOff = TRUE ;
        
}


/****************************************************************************
NAME 
 LMTurnOffLEDPair

DESCRIPTION
    Fn to turn OFF the LEDs associated with the pattern
    
RETURNS
 void
*/
static void LedsTurnOffLEDPair ( ledsPattern_t * pPattern  , bool pUseOveride ) 
{
    uint8 LedA = LedsToPioPin(pPattern->LED_A);

    /*turn off both LEDS*/
    LedsTurnOnOffLeds(LedsOnOffPattern[0][0]);
    
    /*handle an overide LED if we want to use one*/
    if ( pUseOveride )
    {
        LedsHandleOverideLED ( LED_ON ) ;
    }
    
    leds_config->gActiveLEDS [ LedA ].OnOrOff  = FALSE ;        
    
    /*Turn off the LED enable pin*/  
    LedsSetEnablePin ( LED_OFF )  ;
}


/****************************************************************************
NAME 
 LedsHandleOverideLED

DESCRIPTION
    Enables / diables any overide LEDS if there are some    
RETURNS
*/
static void LedsHandleOverideLED ( bool pOnOrOff ) 
{   
    uint16 lFilterIndex = 0 ;
    
    for (lFilterIndex = 0 ; lFilterIndex< leds_config->gNumEventFiltersUsed; lFilterIndex++ )
    {
        if ( LedsIsFilterEnabled(lFilterIndex) )
        {
             if ( leds_config->gLedsEventFilters[lFilterIndex].OverideLEDActive )
             {
                 uint8 lLED = LedsToPioPin(leds_config->gLedsEventFilters[lFilterIndex].OverideLED);
                 /*Overide the Off LED with the Overide LED*/
                 PRINT(("LED_ROM_COMMON: LEDOveride [%d] [%d]\n" , lLED , pOnOrOff)) ;    
                 PioSetLed (lLED, pOnOrOff) ;   
             }    
        }
    }  
}


/****************************************************************************
NAME 
   LedsGetPatternColour

DESCRIPTION
    Fn to determine the LEDColour_t of the LED pair we are currently playing
    takes into account whether or not a filter is currently active
    
RETURNS
  leds_colour
*/
static leds_colour LedsGetPatternColour ( const  ledsPattern_t * pPattern )
{
    uint16 lFilterIndex = 0 ;
        /*sort out the colour of the LED we are interested in*/
    leds_colour lColour = pPattern->Colour ;
   
    for (lFilterIndex = 0 ; lFilterIndex< leds_config->gNumEventFiltersUsed ; lFilterIndex++ )
    {
        if ( LedsIsFilterEnabled(lFilterIndex) )
        {
            if ( leds_config->gLedsEventFilters[lFilterIndex].Colour != LED_COL_EITHER )
            {
                    /*Overide the Off LED with the Overide LED*/
                lColour = leds_config->gLedsEventFilters[lFilterIndex].Colour;   
            } 
        }
    }
    return lColour ;
}


/****************************************************************************
NAME 
 LMApplyFilterToTime

DESCRIPTION
    Fn to change the callback time if a filter has been applied - if no filter is applied
    just returns the original time
    
RETURNS
 uint16 the callback time
*/
static uint16 LedsApplyFilterToTime ( uint16 pTime ) 
{
    uint16 lFilterIndex = 0 ;
    uint16 lTime = pTime ; 
    
    for (lFilterIndex = 0 ; lFilterIndex< leds_config->gNumEventFiltersUsed ; lFilterIndex++ )
    {
        if ( LedsIsFilterEnabled(lFilterIndex) )
        {
            ledsFilter_t *lEventFilter = &(leds_config->gLedsEventFilters[lFilterIndex]);
            
            if ( lEventFilter->Speed )
            {
                if (lEventFilter->SpeedAction == LEDS_SPEED_MULTIPLY)
                {
                    PRINT(("LED_ROM_COMMON: FIL_MULT[%d]\n" , lEventFilter->Speed )) ;
                    lTime *= lEventFilter->Speed ;
                }
                else /*we want to divide*/
                {
                    if (lTime)
                    {
                       PRINT(("LED_ROM_COMMON: FIL_DIV[%d]\n" , lEventFilter->Speed )) ;
                       lTime /= lEventFilter->Speed ;
                    }
                }
            }
        }
    }

    return lTime ;
}




/****************************************************************************
NAME 
 LEDManagerSendEventComplete

DESCRIPTION
    Sends a message to the main task thread to say that an event indication has been completed
    
    
RETURNS
 void
*/
static void LedsSendEventComplete ( uint8 pEvent , bool pPatternCompleted )
{
    MAKE_LEDS_MESSAGE(LEDS_EVENT_IND_CFM) ;

    /*need to add the message containing the EventType here*/
    message->pEvent = pEvent  ;
    message->status = pPatternCompleted ;
            
    MessageSend(leds_rom_ctl.clientTask, LEDS_EVENT_IND_CFM, message) ;
    
    /* Check the Queue Events */
    if (leds_config->gLedsQueueLEDEvents )
    {
        /*if there is a queueud event*/
		if (leds_config->gQueue.Event1)
        {
            PRINT(("LED_ROM_COMMON: Queue [%x][%x][%x][%x]\n", leds_config->gQueue.Event1,
                                                         leds_config->gQueue.Event2,
                                                         leds_config->gQueue.Event3,
                                                         leds_config->gQueue.Event4 ));
                
            LedsIndicateEvent (leds_config->gQueue.Event1, TRUE) ;
    
            /*shuffle the queue*/
            leds_config->gQueue.Event1 = leds_config->gQueue.Event2 ;
            leds_config->gQueue.Event2 = leds_config->gQueue.Event3 ;
            leds_config->gQueue.Event3 = leds_config->gQueue.Event4 ;
            leds_config->gQueue.Event4 = 0x00 ;
        }	
    }
}

/****************************************************************************
NAME 
 LedsEventComplete

DESCRIPTION
    signal that a given event indicatio has completed
RETURNS
 	void
*/
static void LedsEventComplete ( leds_activity * pPrimaryLed , leds_activity * pSecondaryLed, leds_activity * pThirdLed ) 
{       
    pPrimaryLed->Type   = LEDS_IND_UNDEFINED ;
    pSecondaryLed->Type = LEDS_IND_UNDEFINED ;
    pThirdLed->Type     = LEDS_IND_UNDEFINED ;
    
    leds_rom_ctl.gCurrentlyIndicatingEvent = FALSE ;
    
    /* Switch to indicate State */
    MessageSend(leds_rom_ctl.clientTask, LEDS_STATE_IND_REQ, 0) ;
}        
/****************************************************************************
NAME 
 LedsEnableFilter

DESCRIPTION
    enable / disable a given filter ID
RETURNS
 	void
*/
static void LedsEnableFilter ( uint16 pFilter , bool pEnable)
{
	uint16 lOldMask = leds_rom_ctl.gTheActiveFilters ;
	    
	if (pEnable)
    {
        /*to set*/
        leds_rom_ctl.gTheActiveFilters |= (  0x1 << pFilter ) ;
    }
    else
    {
        /*to unset*/
        leds_rom_ctl.gTheActiveFilters &= (0xFFFF - (  0x1 << pFilter ) ) ;
    }
    
    /* Check if we should indicate state */
    if ((leds_config->gLedsEventFilters[pFilter].OverideDisable) && (lOldMask != leds_rom_ctl.gTheActiveFilters))
    {
        MessageSend(leds_rom_ctl.clientTask, LEDS_STATE_IND_REQ, 0); 
    }
}

/****************************************************************************
NAME 
 LedsIsFilterEnabled

DESCRIPTION
    determine if a filter is enabled
RETURNS
 	bool - enabled or not
*/
static bool LedsIsFilterEnabled ( uint16 pFilter )
{
    bool lResult = FALSE ;
	
	if ( leds_rom_ctl.gTheActiveFilters & (0x1 << pFilter ) )
    {
        lResult = TRUE ;
    }
 
    return lResult ;
}

/****************************************************************************
NAME 
 LedsSetLedActivity

DESCRIPTION
    Sets a Led Activity to a known state
RETURNS
   void
*/
static void LedsSetLedActivity ( leds_activity * pLed , leds_ind_type pType , uint16 pIndex , uint16 pDimTime)
{
    memset(pLed, 0, sizeof(leds_activity));
    
    pLed->Type      = pType ;
    pLed->Index     = pIndex ;
    pLed->DimTime   = pDimTime ;   
}

/****************************************************************************
NAME 
	LedsCheckFiltersForLEDFollower
DESCRIPTION
    determine if a follower is currently active
RETURNS
 	bool - active or not
*/
static bool LedsCheckFiltersForLEDFollower( void )
{
    uint16 lResult = FALSE ;    
    uint16 lFilterIndex = 0 ;
    
    for (lFilterIndex = 0 ; lFilterIndex< leds_config->gNumEventFiltersUsed ; lFilterIndex++ )
    {
        if ( LedsIsFilterEnabled(lFilterIndex) )
        {
            ledsFilter_t *lEventFilter = &(leds_config->gLedsEventFilters[lFilterIndex]);
                
            /*if this filter defines a lefd follower*/
            if ( lEventFilter->FollowerLEDActive)
            {
                lResult = TRUE ;
            }    
        }
    }
    return lResult ;
}
/****************************************************************************
NAME 
	LedsGetLedFollowerRepeatTimeLeft
DESCRIPTION
    calculate the new repeat time based upon the follower led delay and the normal repeat time
RETURNS
 	uint16 - updated repeat time to use
*/
static uint16 LedsGetLedFollowerRepeatTimeLeft( ledsPattern_t * pPattern) 
{
    uint16 lTime = LED_SCALE_REPEAT_TIME(pPattern->RepeatTime) ;
    uint16 lPatternTime = ( ( LED_SCALE_ON_OFF_TIME(pPattern->OnTime)  *  pPattern->NumFlashes) + 
                            ( LED_SCALE_ON_OFF_TIME( pPattern->OffTime) * (pPattern->NumFlashes - 1 ) )   +
                            ( LedsGetLedFollowerStartDelay() ) ) ;
                            
    if(lPatternTime < lTime )
    {
        lTime = lTime - lPatternTime ;
        PRINT(("LED_ROM_COMMON: FOllower Rpt [%d] = [%d] - [%d]\n " , lTime , LED_SCALE_REPEAT_TIME(pPattern->RepeatTime) , lPatternTime)) ;
    }
    
    return lTime ;        
}
/****************************************************************************
NAME 
	LedsGetLedFollowerStartDelay
DESCRIPTION
    get the delay associated with a follower led pin
RETURNS
 	uint16 - delay to use for the follower
*/            
static uint16 LedsGetLedFollowerStartDelay( void )
{
    uint16 lDelay = 0 ;
    uint16 lFilterIndex =0 ;    
    
    for (lFilterIndex = 0 ; lFilterIndex< leds_config->gNumEventFiltersUsed ; lFilterIndex++ )
    {
        if ( LedsIsFilterEnabled(lFilterIndex) )
        {
            ledsFilter_t *lEventFilter = &(leds_config->gLedsEventFilters[ lFilterIndex ]);
            
            if (lEventFilter->FollowerLEDActive)
            {		 
                PRINT(("LED_ROM_COMMON: LEDFollower Led[%d] Delay[%d]\n" , LedsToPioPin(lEventFilter->OverideLED) ,
                                                               FILTER_SCALE_DELAY_TIME(lEventFilter->FollowerLEDDelay))) ;    
                lDelay = FILTER_SCALE_DELAY_TIME(lEventFilter->FollowerLEDDelay) * 50 ;
            }    
        }
    }

    return lDelay ;
}


/****************************************************************************
NAME 
 LedsSetEnablePin

DESCRIPTION
    if configured sets a pio as a led enable pin
RETURNS
 void
*/
static void LedsSetEnablePin ( bool pOnOrOff ) 
{
    if ( leds_config->gLedEnablePIO < 0xF ) 
    {
        PioSetLed ( leds_config->gLedEnablePIO , pOnOrOff ) ;
    }
}



/****************************************************************************
NAME 
 isOverideFilterActive

DESCRIPTION
    determine if an overide filter is currently active and driving one of the
    leds in which case return TRUE to prevent it being turned off to display 
    another pattern, allows solid red with flashing blue with no interuption in
    red for example.
RETURNS
    true or false
*/
static bool isOverideFilterActive ( uint8 Led ) 
{  
    uint16 lFilterIndex = 0 ;
 
    /* determine whether feature to make an overide filter drive the led permanently regardless of 
       any intended flash pattern for that led is enabled */
    if( leds_config->gLedsOverideFilterPermanentOn )
    {
        /* permanent overide filter led indication is enabled, this means that an active override
           filter will drive its configured led regardless of any other patterns configured for that
           led */
        for (lFilterIndex = 0 ; lFilterIndex < leds_config->gNumEventFiltersUsed; lFilterIndex++ )
        {
            /* look for any active filters */
            if ( LedsIsFilterEnabled(lFilterIndex) )
            {
                /* if this is an overide filter driving an led check to see if the passed in LED
                   requires that this led be turned off, if it does then stop the led being turned off
                   otherwise allow it to be turned off as usual */
                ledsFilter_t *lEventFilter = &(leds_config->gLedsEventFilters[ lFilterIndex ]);
                
                if ( lEventFilter->OverideLEDActive )
                {
                    uint8 lLedA = LedsToPioPin(lEventFilter->OverideLED);
                    /* if overide led is active and is driving the passed in led stop this led being turned off */
                    if ((lLedA) && (lLedA == Led))
                    {
                        return TRUE;                    
                    }
                }    
            }
        }  
    }
    /* permanent overide filter led drive is diabled so allow led pattern to be indicated */
    else
    {
        return FALSE;
    }
    
    /* default case whereby led can be driven normally */
    return FALSE;    
}


/****************************************************************************
NAME 
    LedsToPio

DESCRIPTION
    To get the PIO Pin number based on the configured leds value
    
RETURN
    uint8
*/ 
uint8 LedsToPioPin(uint8 leds)
{
    switch(leds)
    {
        case LED_RED:
            return leds_config->gTriColLeds.TriCol_R;
        break;
        case LED_BLUE:
            return leds_config->gTriColLeds.TriCol_B;
        break;
        case LED_GREEN:
            return leds_config->gTriColLeds.TriCol_G;
        break; 
        default:
            return leds_config->gTriColLeds.TriCol_R;
        break;
    }    
}

/****************************************************************************
NAME 
    ledsIndicateLedsPattern

DESCRIPTION
 	Given the indication type and leds pattern, Play the LED Pattern
RETURNS
    void
*/
void ledsIndicateLedsPattern(ledsPattern_t *lPattern, uint8 lIndex, leds_ind_type Ind_type)
{
    uint8 lPrimaryLED     = LedsToPioPin(lPattern->LED_A);
    uint8 lSecondaryLED   = LedsToPioPin(lPattern->LED_B);
    uint8 lThirdLED       = LedsToPioPin(lPattern->LED_C);
    
#ifdef DEBUG_PRINT_ENABLED
    {
        if(lPattern)
        {
            char * lColStrings[ 5 ] =   {"LED_E ","LED_A","LED_B","ALT","Both"} ;
    
            PRINT(("LED_A:[%d], LED_B:[%d], OnTime:[%d], OffTime:[%d], RepeatTime:[%d], ", LedsToPioPin(lPattern->LED_A) , LedsToPioPin(lPattern->LED_B), lPattern->OnTime ,lPattern->OffTime ,lPattern->RepeatTime)) ;  
            PRINT(("NumFlashes:[%d], Timeout:[%d], color:[%s], ",       lPattern->NumFlashes, lPattern->TimeOut, lColStrings[lPattern->Colour])) ;    
            PRINT(("overidedisable:[%d]\n",       lPattern->OverideDisable)) ;    
        }
        else
        {
            PRINT(("LMPrintPattern = NULL \n")) ;  
        }
    }
#endif
    
    if(Ind_type == LEDS_IND_EVENT)
    {
        /*if the PIO we want to use is currently indicating an event then do interrupt the event*/
        MessageCancelAll ((TaskData *)&leds_rom_manager, lPrimaryLED ) ;
        MessageCancelAll ((TaskData *)&leds_rom_manager, lSecondaryLED ) ;
        MessageCancelAll ((TaskData *)&leds_rom_manager, lThirdLED ) ;
    }
        
    /*cancel all led state indications*/
    /*Find the LEDS that are set to indicate states and Cancel the messages,
      -do not want to indicate more than one state at a time */
    LedsIndicateNoState ( ) ;

    /*now set up and start the event indication*/
    LedsSetLedActivity ( &leds_config->gActiveLEDS[lPrimaryLED] , Ind_type , lIndex  , lPattern->DimTime ) ;
    /*Set the Alternative LED up with the same info*/
    LedsSetLedActivity ( &leds_config->gActiveLEDS[lSecondaryLED] , Ind_type , lIndex , lPattern->DimTime ) ;
    /*Set the Alternative LED up with the same info*/
    LedsSetLedActivity ( &leds_config->gActiveLEDS[lThirdLED] , Ind_type , lIndex , lPattern->DimTime ) ;

    /* - need to set the LEDS to a known state before starting the pattern*/
    LedsTurnOffLEDPair (lPattern , TRUE) ;
    
    /*Handle permanent output leds*/
    if ( lPattern->NumFlashes == 0 )
    {
        /*set the pins on or off as required*/
        if ( LED_SCALE_ON_OFF_TIME(lPattern->OnTime) > 0 )
        {
            LedsTurnOnLEDPair ( lPattern , &leds_config->gActiveLEDS[lPrimaryLED] ) ;
        }
        else if ( LED_SCALE_ON_OFF_TIME(lPattern->OffTime) > 0 )
        {
            LedsTurnOffLEDPair ( lPattern , TRUE) ;
            
            if( Ind_type == LEDS_IND_EVENT )
            {
                /*If we are turning a pin off the revert to state indication*/
                LedsEventComplete ( &leds_config->gActiveLEDS[lPrimaryLED] , &leds_config->gActiveLEDS[lSecondaryLED] , &leds_config->gActiveLEDS[lThirdLED] ) ;
            }
        }   
    }
    else
    {
        if(Ind_type == LEDS_IND_EVENT)
        {
            MessageSend( (TaskData *)&leds_rom_manager, lPrimaryLED, 0) ;
            leds_rom_ctl.gCurrentlyIndicatingEvent = TRUE ;
        }
        else
        {
            /*send the first message for this state LED indication*/ 
            MessageSendLater ( (TaskData *)&leds_rom_manager, lPrimaryLED, 0, LEDS_STATE_START_DELAY_MS ) ;
        }
    }
}

/****************************************************************************
NAME 
    ledsCheckEventForActiveFilter

DESCRIPTION
 	check the event filter and set/remove the active bit of filters
RETURNS
    void
*/
void ledsCheckEventForActiveFilter( uint8 pEvent )
{
    uint16 lFilterIndex = 0 ;

    for (lFilterIndex = 0 ; lFilterIndex < leds_config->gNumEventFiltersUsed ; lFilterIndex ++ )
	{ 
        ledsFilter_t *lEventFilter = &(leds_config->gLedsEventFilters[ lFilterIndex ]);
        
        if ( (uint16)(lEventFilter->Event) == pEvent )
        {
            if (lEventFilter->IsFilterActive)
            {
                /* Check filter isn't already enabled */
                if (!LedsIsFilterEnabled(lFilterIndex))
                {
                    /* Enable filter */
                    LedsEnableFilter (lFilterIndex , TRUE) ;
            
                    /* If it is an overide fLED filter and the currently playing pattern is OFF then turn on the overide led immediately*/
                    if ( lEventFilter->OverideLEDActive )
                    {
                        uint16 lOverideLEDIndex = LedsToPioPin(lEventFilter->OverideLED) ;                    
                        
                        /* this should only happen if the led in question is currently off*/
                        if ( leds_config->gActiveLEDS[lOverideLEDIndex].OnOrOff == LED_OFF)
                        {
                             PRINT(("LED_ROM_COMMON: FilEnable Turn on[%d][%d] \n",lFilterIndex + 1 , lOverideLEDIndex  )) ;
                             PioSetLed ( lOverideLEDIndex , LED_ON) ;
                        }
                    }
                }
            }
            else
            {
                 uint16 lFilterToCancel = lEventFilter->FilterToCancel ;
                /*disable the according filter*/
                 if ( lFilterToCancel != 0 )
                 {
                     uint16 lFilterToCancelIndex = lFilterToCancel - 1 ;
                     ledsFilter_t *lEventFilter1 = &(leds_config->gLedsEventFilters[ lFilterToCancelIndex ]);
                     uint16 lOverideLEDIndex     = LedsToPioPin(lEventFilter1->OverideLED);
                    
                     PRINT(("LED_ROM_COMMON: FilCancel[%d][%d] [%d]\n",lFilterIndex + 1 , lFilterToCancel , lFilterToCancelIndex )) ;
                     
                        /*lFilter To cancel = 1-n, LedsEbnable filter requires 0-n */
                     LedsEnableFilter (lFilterToCancelIndex , FALSE ) ;
                     
                     if ( leds_config->gActiveLEDS[lOverideLEDIndex].OnOrOff == LED_OFF)
                     {   /*  LedsHandleOverideLED ( theHeadset.theLEDTask , LED_OFF) ;*/
                         if ( lEventFilter1->OverideLEDActive )
                         {
	          	             PioSetLed ( lOverideLEDIndex , LED_OFF) ;                
                             
                             /* it is possible for the cancel filter to turn off leds used in a solid led
                                state indication such as a solid blue pairing indication, should the charger be
                                removed and then reinserted the solid blue state is turned off, this call will reset
                                the state indication and turn it back on again */
                             MessageSend(leds_rom_ctl.clientTask, LEDS_STATE_IND_REQ, 0);
                         }    
                     }                           
                 }
                 else
                 {
                    PRINT(("LED_ROM_COMMON: Fil !\n")) ;
                 }
            }
        }
    }
}



/****************************************************************************
NAME 
 LedActiveFiltersCanOverideDisable

DESCRIPTION
    Check if active filters disable the global LED disable flag.
RETURNS
 	bool
*/
bool LedActiveFiltersCanOverideDisable(void)
{
    uint16 lFilterIndex ;
    
    for (lFilterIndex = 0 ; lFilterIndex< leds_config->gNumEventFiltersUsed ; lFilterIndex++ )
    {
        if ( LedsIsFilterEnabled(lFilterIndex) )
        {
            /* check if this filter overides LED disable flag */
            if (leds_config->gLedsEventFilters[lFilterIndex].OverideDisable)
                return TRUE;
        }
    }
    return FALSE;
}


/****************************************************************************
NAME 
 LedsIndicateNoState

DESCRIPTION
	remove any state indications as there are currently none to be displayed
RETURNS
 void
    
*/
void LedsIndicateNoState( void ) 
{
    /*Find the LEDS that are set to indicate states and Cancel the messages,
        -do not want to indicate more than one state at a time*/
    uint16 lLoop = 0;

    for ( lLoop = 0 ; lLoop < leds_config->gNumActiveLeds ; lLoop ++ )
    {
        leds_activity *lActiveLeds = &leds_config->gActiveLEDS[lLoop];
        
        if (lActiveLeds->Type == LEDS_IND_STATE)
        {
            MessageCancelAll ( (TaskData *)&leds_rom_manager, lLoop ) ; 
            
            lActiveLeds->Type =  LEDS_IND_UNDEFINED ;
            
            LedsTurnOffLEDPair ( &leds_config->gLedsStatePatterns[lActiveLeds->Index] ,TRUE) ;                    
        }
    }
}


/****************************************************************************
NAME 
 LedsResetAllLeds

DESCRIPTION
    resets all led pins to off and cancels all led and state indications
RETURNS
 void
*/
void LedsResetAllLeds( void )
{
    /*Cancel all event indications*/ 
    uint16 lLoop = 0;
    
    for ( lLoop = 0 ; lLoop < leds_config->gNumActiveLeds; lLoop ++ )
    {
        if (leds_config->gActiveLEDS[lLoop].Type == LEDS_IND_EVENT)
        {
            MessageCancelAll ( (TaskData *)&leds_rom_manager, lLoop ) ; 
            
            leds_config->gActiveLEDS[lLoop].Type =  LEDS_IND_UNDEFINED ;

            LedsTurnOffLEDPair( &leds_config->gLedsStatePatterns[leds_config->gActiveLEDS[lLoop].Index] ,TRUE) ;
        }
    }
    	
    /*cancel all state indications*/
    LedsIndicateNoState ( )  ;   
    
}

/****************************************************************************
NAME 
    LedsIsStateMatched

DESCRIPTION
    To check whether the combined state is matched the state in the configured state
    
RETURN
    bool
*/ 
bool LedsIsStateMatched(uint8 config_state, uint8 state)
{
    bool state_matched = FALSE;
    
    PRINT(("LED_ROM_COMMON: HFP state [%x], A2DP state [%x]\n", HFP_STATE(state), A2DP_STATE(state)));
    PRINT(("LED_ROM_COMMON: HFP config state [%x], A2DP config state [%x]\n", HFP_STATE(config_state), A2DP_STATE(config_state)));
    
    if(HFP_STATE(config_state) == HFP_STATE(state))
    {
        if( ( A2DP_FLAG(config_state) &&  A2DP_STATE(config_state) != A2DP_STATE(state)) || 
            (!A2DP_FLAG(config_state) &&  A2DP_STATE(config_state) == A2DP_STATE(state))   )
        {
            state_matched = TRUE;
        }
    }
    return state_matched;
}

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
void LedsResetStateIndNumRepeatsComplete( void ) 
{
    ledsPattern_t * lPattern = NULL;
	
    /* Get the LED state pattern currently is playing */
    if(leds_rom_ctl.pStatePatternIndex != NO_STATE_OR_EVENT)
    {
        lPattern = &leds_config->gLedsStatePatterns[leds_rom_ctl.pStatePatternIndex] ;
    }
    
    /* does pattern exist for this state */
    if (lPattern)
    {
        uint8 LedA             = LedsToPioPin(lPattern->LED_A);
        leds_activity * lLED   = &leds_config->gActiveLEDS[LedA] ;
        if (lLED)
        {
            /*reset num repeats complete to 0*/
            lLED->NumRepeatsComplete = 0 ;
        }    
    }    
}


/****************************************************************************
NAME 
    handleLedsInternalMessage

DESCRIPTION
    The main message handler for the LED task. Controls the PIO in question, then 
    queues a message back to itself for the next LED update.
*/ 
void handleLedsInternalMessage ( Task task , MessageId id, Message message ) 	
{   
    bool lOldState = LED_OFF ;
    uint16 lTime   = 0 ;
    leds_colour lColour ;    
    bool lPatternComplete    = FALSE ;
    
    if (id < DIM_MSG_BASE )
    {
        leds_activity * lLED     = &leds_config->gActiveLEDS[id] ;
        ledsPattern_t * lPattern = NULL ;
        uint8 LedA = 0;

        /*which pattern are we currently indicating for this LED pair*/
        if ( lLED->Type == LEDS_IND_STATE)
        {
            lPattern = &leds_config->gLedsStatePatterns[ lLED->Index] ;
        }
        else
        {   /*is an event indication*/
            lPattern = &leds_config->gLedsEventPatterns[ lLED->Index ] ;
        }
            /*get which of the LEDs we are interested in for the pattern we are dealing with*/
        lColour   = LedsGetPatternColour ( lPattern ) ;
        LedA      = LedsToPioPin( lPattern->LED_A ); 
        
            /*get the state of the LED we are dealing with*/
        lOldState = leds_config->gActiveLEDS[ LedA ].OnOrOff ;
                     
            /*The actual LED handling*/
        if (lOldState == LED_OFF)
        {
            lTime = LED_SCALE_ON_OFF_TIME(lPattern->OnTime) ;
                
            LedsTurnOnLEDPair ( lPattern , lLED ) ;
            
            /*Increment the number of flashes*/
            lLED->NumFlashesComplete++ ;
        }
        else
        {    /*restart the pattern if we have palayed all of the required flashes*/
            if ( lLED->NumFlashesComplete >= lPattern->NumFlashes )
            {
                lTime = LED_SCALE_REPEAT_TIME(lPattern->RepeatTime) ;
                lLED->NumFlashesComplete = 0 ;       
                
                /*inc the Num times the pattern has been played*/
                lLED->NumRepeatsComplete ++ ;
         
                /*if a single pattern has completed*/
                if ( LED_SCALE_REPEAT_TIME(lPattern->RepeatTime) == 0 ) 
                {
                    lPatternComplete = TRUE ;
                }
                   /*a pattern timeout has occured*/
                if ( ( lPattern->TimeOut !=0 )  && ( lLED->NumRepeatsComplete >= lPattern->TimeOut) )
                {
                    lPatternComplete = TRUE ;
                }              
                
                /*if we have reached the end of the pattern and are using a follower then revert to the orig pattern*/
                if (leds_rom_ctl.gFollowing)
                {
                    leds_rom_ctl.gFollowing = FALSE ;
                    lTime = LedsGetLedFollowerRepeatTimeLeft( lPattern ) ;    
                }
                else
                {
                    /*do we have a led follower filter and are we indicating a state, if so use these parameters*/
                    if (lLED->Type == LEDS_IND_STATE)
                    {
                        if( LedsCheckFiltersForLEDFollower( ) )
                        {
                            lTime = LedsGetLedFollowerStartDelay( ) ;       
                            leds_rom_ctl.gFollowing = TRUE ;
                        }
                    }    
                }            
            } 
            else /*otherwise set up for the next flash*/
            {
                lTime = LED_SCALE_ON_OFF_TIME(lPattern->OffTime) ;
            } 
                /*turn off both LEDS*/
        						
			{
				leds_colour lColour = LedsGetPatternColour ( lPattern ) ;
				
				if ( !LedsColorAltPattern[lColour] )
				{
	        	    if ( (lTime == 0 ) && ( lPatternComplete == FALSE ) )
    		        {
    	    	            /*ie we are switching off for 0 time - do not use the overide led as this results in a tiny blip*/
	            	    LedsTurnOffLEDPair ( lPattern , FALSE) ;
	        	    }
    		        else
    	    	    {
	            	    LedsTurnOffLEDPair ( lPattern , TRUE) ;
	            	}
				}
				else
				{
					/*signal that we are off even though we are not*/
				    leds_config->gActiveLEDS[ LedA ].OnOrOff  = FALSE ;                     
				}
			}
        }
        
        /*handle the completion of the pattern or send the next update message*/
        if (lPatternComplete)
        {
            /*set the type of indication for both LEDs as undefined as we are now indicating nothing*/
            if ( leds_config->gActiveLEDS[id].Type == LEDS_IND_EVENT )
            {
                uint8 LedB      = LedsToPioPin( lPattern->LED_B );
                uint8 LedC      = LedsToPioPin( lPattern->LED_C );
                
                /*signal the completion of an event*/
                LedsSendEventComplete ( lPattern->EventOrState, TRUE ) ;
             
                /*now complete the event, and indicate a new state if required*/        
                LedsEventComplete ( lLED , &leds_config->gActiveLEDS[LedB], &leds_config->gActiveLEDS[LedC] ) ;
            }  
            else if (leds_config->gActiveLEDS[id].Type == LEDS_IND_STATE )
            {
               /* Tell App that the LEDS state Ind has been completed */
               MAKE_LEDS_MESSAGE(LEDS_STATE_IND_TIMEOUT);
               message->status = TRUE;
               MessageSend( leds_rom_ctl.clientTask, LEDS_STATE_IND_TIMEOUT, message);
               
               /*then we have completed a state indication and the led pattern is now off*/    
               /*Indicate that we are now with LEDS disabled*/
               leds_config->gLEDSStateTimeout = TRUE ;
            }
            
            /* ensure leds are turned off when pattern completes as when using an alternating pattern
               leds are now left on to provide a better smoother transistion between colours */
            if ( LedsColorAltPattern[lColour] )
            {
                LedsTurnOffLEDPair ( lPattern , TRUE) ;
            }
        }
        else
        {       /*apply the filter in there is one  and schedule the next message to handle for this led pair*/
            lTime = LedsApplyFilterToTime ( lTime ) ;
            
            MessageSendLater( (TaskData *)&leds_rom_manager, id , 0 , lTime ) ;
        } 
    }
    else
    {
        /*DIMMING LED Update message */       
        PioSetDimState ( (id - DIM_MSG_BASE) );
    }
}


