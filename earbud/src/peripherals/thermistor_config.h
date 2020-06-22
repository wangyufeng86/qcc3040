/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file       thermistor_config.h
\brief      Configuration related definitions for thermistor.
*/

#ifndef THERMISTOR_CONFIG_H_
#define THERMISTOR_CONFIG_H_

#ifdef HAVE_THERMISTOR

#include "thermistor.h"

/*! The thermistor configuration */
extern const thermistorConfig thermistor_config;

/*! Returns the thermistor configration */
#define appConfigThermistor() (&thermistor_config)

#endif /* HAVE_THERMISTOR */
#endif /* THERMISTOR_CONFIG_H_ */
