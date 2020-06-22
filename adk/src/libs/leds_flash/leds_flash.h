/****************************************************************************
Copyright (c) 2004 - 2015 Qualcomm Technologies International, Ltd.

FILE NAME
    leds_flash.h
    
DESCRIPTION
	Header file for the leds_flash plugin interface.

    The parameters / enums here define the configuration of led patterns used for the 
    leds_flash plugin.
*/

#ifndef _LEDS_FLASH_H_
#define _LEDS_FLASH_H_

#include <message.h>
#include "leds_manager.h"
#include "leds_manager_if.h"

/* Define the Active Event Filter*/
typedef enum
{
    LEDS_FLASH_NORMAL,
    LEDS_FLASH_LOWBATTERY,
    LEDS_FLASH_FULLBATTERY,
    LEDS_FLASH_MUTE
}ledsFilterIndex;

/*! A led event filter pattern type */
typedef struct
{
	unsigned        event:8;
    unsigned        IsFilterActive:1;
	ledsFilterIndex index:7;
}ledsEventFilter_t ;


/*! A led event filter pattern type, including the number of configured event filter */
typedef struct
{
   uint16            num_filters;
   ledsEventFilter_t *filter;
}ledsEventFilterConfig;


/*! A led state pattern type */
typedef struct
{
    unsigned state: 8;
	unsigned normal:8;  
	unsigned low_battery:8 ;
	unsigned full_battery:8 ;
	unsigned mute:8 ;
}ledsStatePattern_t ;
 

/*! A led state pattern type, including the number of configured events */
typedef struct
{
   uint8 num_states;
   ledsStatePattern_t *pattern;
}ledsStatePatternConfig;

/*! A led event pattern type */
typedef struct 
{
	unsigned event:8 ;
	unsigned pattern:8 ;
} ledsEventPattern_t ;


/*! A led event pattern type, including the number of configured events */
typedef struct
{
   uint8 num_events;
   ledsEventPattern_t *pattern;
}ledsEventPatternConfig;


/*! configuration type of leds flash */
typedef struct
{
   ledsEventFilterConfig   *event_filters;    
   ledsEventPatternConfig  *event_pattern;
   ledsStatePatternConfig  *state_pattern;
}leds_flash_config_type;


/* leds flash manager plugin */
extern const TaskData leds_flash_manager;

#endif  /*_LEDS_FLASH_H_*/

