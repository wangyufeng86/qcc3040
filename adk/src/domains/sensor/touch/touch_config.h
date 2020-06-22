/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file       proximity_config.h
\brief      Configuration related definitions for proximity sensor support.
*/

#ifndef TOUCH_CONFIG_H_
#define TOUCH_CONFIG_H_


#include "touch.h"

/*! The touch sensor configuration */
extern const touchConfig touch_config;

/*! Returns the touch sensor configuration */
#define appConfigTouch() (&touch_config)

#endif /* TOUCH_CONFIG_H_ */
