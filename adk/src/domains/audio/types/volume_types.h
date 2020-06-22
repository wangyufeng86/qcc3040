/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Volume related data types
*/

#ifndef VOLUME_TYPES_H_
#define VOLUME_TYPES_H_

typedef enum
{
    event_origin_local,
    event_origin_external,
    event_origin_peer,
} event_origin_t;

typedef struct
{
    int min;
    int max;
} range_t;

typedef struct
{
    range_t range;
    int number_of_steps;
} volume_config_t;

typedef struct
{
    volume_config_t config;
    int value;
} volume_t;

typedef enum
{
    unmute = 0,
    mute,
} mute_state_t;

#define FULL_SCALE_VOLUME                   { .config = {.range = {.min = 0, .max = 1}, .number_of_steps = 1}, .value = 1 }
#define PERCENTAGE_VOLUME_CONFIG            { .range = { .min = 0, .max = 100 }, .number_of_steps = 101 }
#define PERCENTAGE_VOLUME(percentage_value) { .config = PERCENTAGE_VOLUME_CONFIG, .value = percentage_value }

#endif /* VOLUME_TYPES_H_ */
