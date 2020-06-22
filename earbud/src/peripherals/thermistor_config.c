/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file       thermistor_config.c
\brief      Configuration related structure for thermistor.
*/

#include "thermistor_config.h"
#ifdef HAVE_THERMISTOR

/* Requrired for THERMISTOR_ADC */
#include <app/adc/adc_if.h>

const thermistorConfig thermistor_config = {
    .on = THERMISTOR_ON,
    .adc = THERMISTOR_ADC,
};

#endif /* HAVE_THERMISTOR */
