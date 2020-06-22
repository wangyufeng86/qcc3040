/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file       acceleration_config.h
\brief      Configuration related definitions for accelerometer support.
*/

#ifndef ACCELERATION_CONFIG_H_
#define ACCELERATION_CONFIG_H_

#if defined(INCLUDE_ACCELEROMETER)
#include "acceleration.h"

/*! The accelerometer configuration */
extern const accelerometerConfig accelerometer_config;

/*! Returns the accelerometer configuration */
#define appConfigAccelerometer() (&accelerometer_config)
#endif

#endif /* ACCELERATION_CONFIG_H_ */
