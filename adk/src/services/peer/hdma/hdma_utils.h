/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file
\brief      Hearder for Utility functions required for HDMA Core.
*/

#ifdef INCLUDE_HDMA

#ifndef HDMA_UTILS_H
#define HDMA_UTILS_H

#ifdef DEBUG_HDMA_UT
#include "types.h"
#include <stdlib.h>
#include <stdio.h>
#endif
#ifndef DEBUG_HDMA_UT
#include <macros.h>
#include <logging.h>
#endif

#ifdef DEBUG_HDMA_UT
    #define TRUE 1
    #define FALSE 0
    #define MAX( a, b ) ( ( a > b) ? a : b )
    #define MIN( a, b ) ( ( a < b) ? a : b )
#endif


#ifdef DEBUG_HDMA_UT
    #define HDMA_DEBUG_LOG(...) printf(__VA_ARGS__)
    #define HDMA_DEBUG_LOG_INFO(...) printf(__VA_ARGS__)
#else
	/*! Print Debug logs */
    #define HDMA_DEBUG_LOG(...) DEBUG_LOG(__VA_ARGS__)
	/*! Print Debug logs for level info */
    #define HDMA_DEBUG_LOG_INFO(...) DEBUG_LOG_INFO(__VA_ARGS__)
#endif

#ifdef DEBUG_HDMA_UT
    #define HDMA_MALLOC(T) (T*)malloc(sizeof(T))
#else
	/*! Allocate memory */
    #define HDMA_MALLOC(T) PanicUnlessMalloc(sizeof(T))
#endif

/*! Used to check when timestamp exceeds base time and 48KB */
#define FORTYEIGHTKB 49152
/*! Circular buffer length */
#define BUFFER_LEN 10
#define MIN_UPDATE_INT_MS 0
/*! Minimum time required to retry handover at low level */
#define MIN_HANDOVER_RETRY_TIME_LOW_MS 0
/*! Minimum time required to retry handover at high level */
#define MIN_HANDOVER_RETRY_TIME_HIGH_MS 0
/*! Minimum time required to retry handover at critical level */
#define MIN_HANDOVER_RETRY_TIME_CRITICAL_MS 0
#define OUT_OF_EAR_TIME_BEFORE_HANDOVER_MS 2100
/*! invalid timestamp */
#define INVALID_TIMESTAMP 0xFFFFFFF
/*! Unknown to HDMA */
#define HDMA_UNKNOWN 0xFF
/*! Unknown Quality */
#define HDMA_UNKNOWN_QUALITY 0xFF

#define IN_EAR_FALLBACK TRUE
/*! When handover is invalid */
#define HDMA_INVALID -1

/*! \brief Returns rounded value to nearest integer.

*/
#define ROUND(num,denom) ((num < 0)? ((((num << 1)/denom) - 1) >> 1): ((((num << 1)/denom) + 1)>> 1))

/*! Structure to hold threshold values for quality events */
typedef struct{
	/*! threshold critical */
    int16 critical;
	/*! threshold high */
    int16 high;
	/*! threshold low */
    int16 low;
}hdma_urgency_thresholds_t;

/*! Structure to hold all information for thresholds for quality event */
typedef struct {
	/*! type of event (rssi/mic) */
	uint8 type;
	/*! half life threshold for the event */
	hdma_urgency_thresholds_t halfLife_ms;
	/*! max age threshold for the event */
	hdma_urgency_thresholds_t maxAge_ms;
	/*! absolute threshold value for the event */
	hdma_urgency_thresholds_t absThreshold;
	/*! relative threshold value for the event */
	hdma_urgency_thresholds_t relThreshold;
}hdma_thresholds_t;

extern const hdma_thresholds_t rssi;
extern const hdma_thresholds_t mic;

#endif

#endif /* INCLUDE_HDMA */
