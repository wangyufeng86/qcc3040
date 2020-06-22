/*!
\copyright  Copyright (c) 2018 - 2019 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file       proximity.h
\brief      Header file for proximity sensor support
*/

#ifndef TOUCH_H
#define TOUCH_H

#include <task_list.h>
#include "domain_message.h"

/*! Enumeration of touch actions that can send to its clients */
typedef enum
{
    SINGLE_PRESS,
    DOUBLE_PRESS,
    DOUBLE_PRESS_HOLD,
    TRIPLE_PRESS,
    TRIPLE_PRESS_HOLD,
    FOUR_PRESS,
    FOUR_PRESS_HOLD,
    FIVE_PRESS,
    FIVE_PRESS_HOLD,
    SIX_PRESS,
    SEVEN_PRESS,
    EIGHT_PRESS,
    NINE_PRESS,
    LONG_PRESS,
    VERY_LONG_PRESS,
    VERY_VERY_LONG_PRESS,
    VERY_VERY_VERY_LONG_PRESS,
    SLIDE_UP,
    SLIDE_DOWN,
    SLIDE_LEFT,
    SLIDE_RIGHT,
    HAND_COVER,
    HAND_COVER_RELEASE,
    MAX_ACTION,
} touchAction;

typedef struct
{
    touchAction   action;
    MessageId     message;

} touchEventConfig;

/*! touch config type */
typedef struct __touch_config touchConfig;

/*! @brief Touchpad module state. */
typedef struct
{
    /*! Touch State module message task. */
    TaskData task;
    /*! Handle to the bitserial instance. */
    bitserial_handle handle;
    /*! List of registered client tasks */
    task_list_t *clients;
    /*! The config */
    const touchConfig *config;
    /*! Input action table */
    const touchEventConfig *action_table;
    /*! Input action table size */
    uint32 action_table_size;
    /*! to set first data is true for calibration */
    bool first_data;
} touchTaskData;

/*!< Task information for proximity sensor */
extern touchTaskData app_touch;
/*! Get pointer to the proximity sensor data structure */
#define TouchGetTaskData()   (&app_touch)


#if defined(HAVE_TOUCHPAD_PSOC4000S)
extern bool touchSensorClientRegister(Task task, uint32 size_action_table, const touchEventConfig *action_table);
#endif

/*! \brief Unregister with proximity.
    \param task The task to unregister.
    The sensor will be disabled when the final client unregisters. */
#if defined(HAVE_TOUCHPAD_PSOC4000S)
extern void touchSensorClientUnRegister(Task task);
#else
#define touchSensorClientUnRegister(task) ((void)task)
#endif

#endif /* PROXIMITY_H */
