/****************************************************************************
Copyright (c) 2004 - 2015 Qualcomm Technologies International, Ltd.

FILE NAME
    leds_rom.h
    
DESCRIPTION
	Header file for the leds_rom manager interface.

    The parameters / enums here define the configuration of led patterns used for the 
    leds_rom manager plugin.
*/

#ifndef _LEDS_ROM_H_
#define _LEDS_ROM_H_

#include <message.h>
#include "leds_manager.h"

#define NO_STATE_OR_EVENT           (0xff)

#define LED_MAX_NUM_STATE_PATTERN   (20)
#define LED_MAX_NUM_EVENT_PATTERN   (20)
#define LED_MAX_NUM_EVENT_FILTERS   (16)

/*LED filter for changing flash speed*/
typedef enum 
{
    LEDS_SPEED_MULTIPLY = 0,
    LEDS_SPEED_DIVIDE  
}leds_speed_action ;


/*the tricolour led information, PIOs are configurable   */
/*Only LED_RED and LED_BLUE are used for two-led system */
typedef enum 
{
	LED_RED  ,              /* PIO for RED   Led */
	LED_BLUE ,              /* PIO for BLUE  Led */
	LED_GREEN               /* PIO for GREEN Led */
}ledsPIO_t ;

/*LED display colour pattern*/
typedef enum 
{
    LED_COL_EITHER ,
    LED_COL_LED_A ,
    LED_COL_LED_B ,
    LED_COL_LED_A_B_ALT,       /*Alternate the colours */
    LED_COL_LED_A_B_BOTH,      /*Use Both LEDS         */
    LED_COL_LED_C ,            /*The following patterns are used for Tri-Colour LEDs only */
    LED_COL_LED_A_C_ALT,
    LED_COL_LED_B_C_ALT,
    LED_COL_LED_A_C_BOTH,
    LED_COL_LED_B_C_BOTH,
    LED_COL_LED_AB_C_ALT,      /*Original Tri-colour covers */
    LED_COL_LED_AC_B_ALT,      /*Original Tri-colour covers */
    LED_COL_LED_A_BC_ALT,      /*Original Tri-colour covers */
    LED_COL_LED_A_B_C_ALT,
    LED_COL_LED_A_B_C_ALL
}leds_colour ;

#define MAX_LEDS_PATTERN (LED_COL_LED_A_B_C_ALL + 1)

typedef struct 
{
    unsigned            Event:8 ;      /*The event to action the filter upon*/
    unsigned            Speed:8 ;      /*speed multiple o apply - 0 =  no speed multiple*/
    
	unsigned            IsFilterActive:1 ;
    unsigned            Dummy:1;
    unsigned            SpeedAction:2 ;/*which action to perform on the multiple  multiply or divide */
    unsigned            Colour:4 ;     /*Force LED to this colour pattern no matter what is defined in the state pattern*/    
    unsigned            FilterToCancel:4 ;
    /* TODO */
    unsigned            Dummy1:2;
    ledsPIO_t           OverideLED:2;    
    
    unsigned            OverideLEDActive:1 ;
    unsigned            Dummy2:2;
    unsigned            FollowerLEDActive:1 ;/*whether this filter defines a follower led*/
	unsigned            FollowerLEDDelay:4 ; /*the Delay before the following pattern starts*/ /*50ms (0 - 750ms)*/
    unsigned            OverideDisable:1 ; /* overide LED disable flag when filter active */
    unsigned            Dummy3:7;
}ledsFilter_t ;


/*the led pattern type */
typedef struct 
{
    unsigned          EventOrState:8;
    unsigned          OnTime:8     ;    /* ms */
    
    unsigned          OffTime:8    ;    /* ms */
    unsigned          RepeatTime:8 ;    /* ms */  
    
    unsigned          DimTime:8     ;   /* Time to Dim this LED*/       
    unsigned          TimeOut:8     ;   /* number of repeats*/
    
    unsigned          NumFlashes:4 ;    /* how many flashes in the pattern*/          
    ledsPIO_t         LED_A:2      ;    /* default first LED to use*/
    ledsPIO_t         LED_B:2      ;    /* second LED to use*/

    ledsPIO_t         LED_C:2      ;    /* third LED, it is not used if only two leds (decided by colour) are support*/
    unsigned          OverideDisable:1; /* overide LED disable flag for this pattern */
    unsigned          unused:1     ;
    unsigned          Colour:4     ;    /* which of the LEDS pattern is to be used   */     
}ledsPattern_t ;

/* leds event queue messages */
typedef struct 
{
    unsigned            Event1:8 ;
    unsigned            Event2:8 ;
    unsigned            Event3:8 ;
    unsigned            Event4:8 ;    
} ledsEventQueue_t;


/*the tricolour led information, PIOs are configurable*/
typedef struct 
{
	unsigned            TriCol_R:4;  /* PIO Pin Num for RED   Led */
	unsigned            TriCol_B:4;  /* PIO Pin Num for BLUE  Led */
	unsigned            TriCol_G:4;  /* PIO Pin Num for GREEN Led */
	unsigned            Unused1 :4;
}ledsTriColLeds_t ;


/* The led indication types */
typedef enum 
{
    LEDS_IND_UNDEFINED = 0 ,
    LEDS_IND_STATE,                        /* State indication */
    LEDS_IND_EVENT                         /* event indication */
}leds_ind_type ;


/*the information required for a LED to be updated*/
typedef struct 
{  
    unsigned         Index:8;                 /*what this led is displaying*/
    unsigned         NumFlashesComplete:4 ;   /*how far through the pattern we currently are*/        
    unsigned         FilterIndex:4 ;          /*the filter curently attached to this LED (0-15)*/    
    
    /*dimming*/
    unsigned         unused1:2;
    leds_ind_type    Type:2 ;  
    unsigned         DimState:4  ;            /*how far through the dim pattern we are*/
    unsigned         DimTime:7   ;
    unsigned         OnOrOff:1 ;
    
    /*what this LED is displaying*/
    unsigned         unused:3 ;
    unsigned         DimDir:1 ;
    unsigned         LED_PIO:4 ;
    unsigned         NumRepeatsComplete:8;
}leds_activity ;


/*The LED configure type*/
typedef struct
{
    ledsPattern_t       *gLedsStatePatterns;            /*pointer to the array of state leds patterns */
    ledsPattern_t       *gLedsEventPatterns;            /*pointer to the array of event leds patterns  */
    ledsFilter_t        *gLedsEventFilters ;            /*pointer to the array of LED Filter patterns */
    leds_activity       *gActiveLEDS;                   /*pointer to the array of LED Activities */
    
    uint16               gNumStatePatternsUsed;         /*number of state pattern used */
    uint16               gNumEventPatternsUsed;         /*number of event pattern used */
    uint16               gNumEventFiltersUsed;          /*number of leds event filter used */

    ledsTriColLeds_t     gTriColLeds ;                  /*PIO pins for tri-colour LED display*/ 
    ledsEventQueue_t     gQueue ;                       /*Event Queue Data, Can put into library local ctrl variables */
    
    unsigned             gLedsQueueLEDEvents:1;
    unsigned             gLedsTimerMultiplier:2;
    unsigned             gLedsOverideFilterPermanentOn:1;
    unsigned             gLedEnablePIO:4;
    
    unsigned             gLedsRomEnabled:1 ;             /*Can put into library local ctrl variables*/
    unsigned             gNumActiveLeds:5 ;              /*number of active leds (2 or 3), maximum 16 */
    unsigned             gLEDSStateTimeout:1;
}leds_config_type;  


/* leds rom manager plugin */
extern const TaskData leds_rom_manager;


#endif  /*_LEDS_ROM_H_*/

