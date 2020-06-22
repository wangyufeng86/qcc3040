/*****************************************************************************
Copyright (c) 2018 Qualcomm Technologies International, Ltd.

FILE NAME
    ama_debug.h

DESCRIPTION
    AMA debug macros
*/

#ifndef AMA_DEBUG_H_
#define AMA_DEBUG_H_

#include <panic.h>


#ifndef DEBUG_PRINT_ENABLED
#define DEBUG_PRINT_ENABLED
#endif
#include <print.h>

#define AMA_DEBUG(x) {PRINT(x);}
#define AMA_DEBUG_INFO(x) {PRINT(("%s:%d - ", __FILE__, __LINE__)); PRINT(x);}
#define AMA_DEBUG_PANIC(x) {AMA_DEBUG_INFO(x); Panic();}

#endif /* AMA_DEBUG_H_ */

