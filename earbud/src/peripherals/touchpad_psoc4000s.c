/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file       touchpad_psoc4000s.c
\brief      Support for the Cypress Touchpad PSoc 4000S
*/
#ifdef INCLUDE_CAPSENSE
#ifdef HAVE_TOUCHPAD_PSOC4000S
#include <bitserial_api.h>
#include <panic.h>
#include <pio.h>
#include <pio_monitor.h>
#include <stdlib.h>

#include "adk_log.h"
#include "touchpad_psoc4000s.h"
#include "touch.h"
#include "touch_config.h"
#include "proximity.h"

/*! \brief Returns the PIOs bank number.
    \param pio The pio.
*/
#define PIO2BANK(pio) ((uint16)((pio) / 32))
/*! \brief Returns the PIO bit position mask within a bank.
    \param pio The pio.
*/
#define PIO2MASK(pio) (1UL << ((pio) % 32))

const struct __touch_config touch_config = {
    .i2c_clock_khz = 100,
    .pios = {
        /* The touch PIO definitions are defined in the platform x2p file */
        .xres = RDP_PIO_XRES,
        .i2c_scl = RDP_PIO_I2C_SCL,
        .i2c_sda = RDP_PIO_I2C_SDA,
        .interrupt = RDP_PIO_INT_TOUCH,
        .ldo1v8 = RDP_PIO_LDO1V8
    },
};

const touch_data_to_action_t touch_action_map[] =
{
    {
        SINGLE_PRESS,
        TOUCH_DATA_SINGLE_PRESS
    },
    {
        DOUBLE_PRESS,
        TOUCH_DATA_DOUBLE_PRESS
    },    
    {
        DOUBLE_PRESS_HOLD,
        TOUCH_DATA_DOUBLE_PRESS_HOLD
    },
    {
        TRIPLE_PRESS,
        TOUCH_DATA_TRIPLE_PRESS
    },    
    {
        TRIPLE_PRESS_HOLD,
        TOUCH_DATA_TRIPLE_PRESS_HOLD
    },
    {
        FOUR_PRESS,
        TOUCH_DATA_FOUR_PRESS
    },    
    {
        FOUR_PRESS_HOLD,
        TOUCH_DATA_FOUR_PRESS_HOLD
    },
    {
        FIVE_PRESS,
        TOUCH_DATA_FIVE_PRESS
    },
    {
        FIVE_PRESS_HOLD,
        TOUCH_DATA_FIVE_PRESS_HOLD
    },
    {
        SIX_PRESS,
        TOUCH_DATA_SIX_PRESS
    },
    {
        SEVEN_PRESS,
        TOUCH_DATA_SEVEN_PRESS
    },
    {
        EIGHT_PRESS,
        TOUCH_DATA_EIGHT_PRESS
    },    
    {
        NINE_PRESS,
        TOUCH_DATA_NINE_PRESS
    },
    {
        LONG_PRESS,
        TOUCH_DATA_LONG_PRESS
    },
    {
        VERY_LONG_PRESS,
        TOUCH_DATA_VERY_LONG_PRESS
    },
	{
		VERY_VERY_LONG_PRESS,
		TOUCH_DATA_VERY_VERY_LONG_PRESS
	},
    {
        VERY_VERY_VERY_LONG_PRESS,
        TOUCH_DATA_VERY_VERY_VERY_LONG_PRESS
    },
    {
        SLIDE_UP,
        TOUCH_DATA_SLIDER_UP
    },
    {
        SLIDE_DOWN,
        TOUCH_DATA_SLIDER_DOWN
    },
	{
		SLIDE_LEFT,
		TOUCH_DATA_SLIDER_LEFT
	},
	{
		SLIDE_RIGHT,
		TOUCH_DATA_SLIDER_RIGHT
	},
	{
		HAND_COVER,
		TOUCH_DATA_HANDCOVER
	},
	{
		HAND_COVER_RELEASE,
		TOUCH_DATA_HANDCOVER_RELEASE
	},
};

uint8 touchData [5];
/*!< Task information for touch pad */
touchTaskData app_touch;

/*! \brief Read touch event */
static bool touchPsoc4000s_ReadEvent(bitserial_handle handle, uint8 *value)
{
    bitserial_result result = BITSERIAL_RESULT_INVAL;

    /* First set the parameters */
    result = BitserialChangeParam(handle,
                                  BITSERIAL_PARAMS_I2C_DEVICE_ADDRESS, 
                                  TOUCHPAD_I2C_ADDRESS,
                                  BITSERIAL_FLAG_BLOCK);
    if (BITSERIAL_RESULT_SUCCESS != result)
        return FALSE;
    
    /* First write the register address to be read */
    result = BitserialTransfer(handle,
                            NULL,
                            NULL,
                            0,
                            value,
                            5);

    return (result == BITSERIAL_RESULT_SUCCESS);
}

/*! \brief Decode touch data */
static void touchPsoc4000s_HandleTouchData(uint8 *data)
{
    touchTaskData *touch = &app_touch;
    
    /* verify valid data received*/
    if ((data[0] != TOUCH_DATA_FIRST_BYTE) || (data[1] != TOUCH_DATA_SECOND_BYTE)
         || (data[2] != TOUCH_DATA_THIRD_BYTE) || (data[3] != TOUCH_DATA_FOURTH_BYTE))
    {
        DEBUG_LOG_WARN("touchPsoc4000s_HandleTouchData Wrong Event Data received");
        return;
    }

    touchAction touch_ui_input = MAX_ACTION;
    uint8 actionTableSize = ARRAY_DIM(touch_action_map);
    uint8 i = 0;

    DEBUG_LOG_VERBOSE("Touch Event %02x", data[4]);

    for(i=0; i < actionTableSize; i++)
    {
        if (touch_action_map[i].touch_data == data[4])
        {
            touch_ui_input = touch_action_map[i].action;
            break;
        }
    }
    
    if (touch_ui_input != MAX_ACTION)
    {
        /* try to match input action with UI message to be broadcasted*/
        for (i=0; i<touch->action_table_size; i++)
        {
            if (touch_ui_input == touch->action_table[i].action)
            {
                /* Inform clients */
                DEBUG_LOG_VERBOSE("Send UI event %x", touch->action_table[i].message);
                TaskList_MessageSendId(touch->clients, touch->action_table[i].message);
                break;
            }
        }
    }
}

/*! \brief Handle the touch interrupt */
static void touchPsoc4000s_InterruptHandler(Task task, MessageId id, Message msg)
{
    touchTaskData *touch = (touchTaskData *) task;
    switch(id)
    {
        case MESSAGE_PIO_CHANGED:
        {
            const MessagePioChanged *mpc = (const MessagePioChanged *)msg;
            uint32 state = ((uint32)mpc->state16to31 << 16) + mpc->state;
    
            if (mpc->bank == PIO2BANK(touch->config->pios.interrupt))
            {
                if (~state & PIO2MASK(touch->config->pios.interrupt))
                {
                    memset(&touchData[0], 0, sizeof(touchData));
                    PanicFalse(touchPsoc4000s_ReadEvent(touch->handle, touchData));
                    touchPsoc4000s_HandleTouchData(touchData);
                }
            }
        }
        break;
        default:
            break;
    }
}

/*! \brief Enable the proximity sensor */
static bitserial_handle touchPsoc4000s_Enable(const touchConfig *config)
{
    bitserial_handle ret;
    uint32 i;
    uint16 bank;
    uint32 mask;
    struct
    {
        uint16 pio;
        pin_function_id func;
    } i2c_pios[] = {{config->pios.i2c_scl, BITSERIAL_1_CLOCK_OUT},
                    {config->pios.i2c_scl, BITSERIAL_1_CLOCK_IN},
                    {config->pios.i2c_sda, BITSERIAL_1_DATA_OUT},
                    {config->pios.i2c_sda, BITSERIAL_1_DATA_IN}};

    DEBUG_LOG_VERBOSE("touchPsoc4000sEnable");

    /* Setup Interrupt as input with weak pull up */
    bank = PIO2BANK(config->pios.interrupt);
    mask = PIO2MASK(config->pios.interrupt);
    PanicNotZero(PioSetMapPins32Bank(bank, mask, mask));
    PanicNotZero(PioSetDir32Bank(bank, mask, 0));
    PanicNotZero(PioSet32Bank(bank, mask, mask));
    bank = PIO2BANK(config->pios.ldo1v8);
    mask = PIO2MASK(config->pios.ldo1v8);

    PanicNotZero(PioSetMapPins32Bank(bank, mask, mask));
    PanicNotZero(PioSetDir32Bank(bank, mask, mask));
    PanicNotZero(PioSet32Bank(bank, mask, mask));

    /* Setup Reset PIN to Output High*/
    bank = PIO2BANK(config->pios.xres);
    mask = PIO2MASK(config->pios.xres);
    PanicNotZero(PioSetMapPins32Bank(bank, mask, mask));
    PanicNotZero(PioSetDir32Bank(bank, mask, mask));
    PanicNotZero(PioSet32Bank(bank, mask, mask));

    for (i = 0; i < ARRAY_DIM(i2c_pios); i++)
    {
        uint16 pio = i2c_pios[i].pio;
        bank = PIO2BANK(pio);
        mask = PIO2MASK(pio);

        /* Setup I2C PIOs with strong pull-up */
        PanicNotZero(PioSetMapPins32Bank(bank, mask, 0));
        PanicFalse(PioSetFunction(pio, i2c_pios[i].func));
        PanicNotZero(PioSetDir32Bank(bank, mask, 0));
        PanicNotZero(PioSet32Bank(bank, mask, mask));
        PanicNotZero(PioSetStrongBias32Bank(bank, mask, mask));
    }

#ifndef INCLUDE_PROXIMITY
    bitserial_config bsconfig;

    /* Configure Bitserial to work with touchpad */
    memset(&bsconfig, 0, sizeof(bsconfig));
    bsconfig.mode = BITSERIAL_MODE_I2C_MASTER;
    bsconfig.clock_frequency_khz = config->i2c_clock_khz;
    bsconfig.u.i2c_cfg.i2c_address = TOUCHPAD_I2C_ADDRESS;
    ret = BitserialOpen((bitserial_block_index)BITSERIAL_BLOCK_1, &bsconfig);
#else
    //share I2C with proximity sensor
    proximityTaskData *prox = ProximityGetTaskData();
    ret = prox->handle;
#endif
    return ret;
}

/*! \brief Disable the proximity sensor */
static void touchPsoc4000s_Disable(bitserial_handle handle, const touchConfig *config)
{
    uint16 bank;
    uint32 mask;
    DEBUG_LOG_VERBOSE("touchPsoc4000sDisable");

    /* Disable interrupt and set weak pull down */
    bank = PIO2BANK(config->pios.interrupt);
    mask = PIO2MASK(config->pios.interrupt);
    PanicNotZero(PioSet32Bank(bank, mask, 0));

    /* Release bitserial instance */
    BitserialClose(handle);
    handle = BITSERIAL_HANDLE_ERROR;

}

bool touchSensorClientRegister(Task task, uint32 size_action_table, const touchEventConfig *action_table)
{
    touchTaskData *touch = &app_touch;

    if (NULL == touch->clients)
    {
        const touchConfig *config = &touch_config;
        touch->config = config;
        touch->clients = TaskList_Create();
        touch->first_data = TRUE;
        touch->handle = touchPsoc4000s_Enable(config);
        PanicFalse(touch->handle != BITSERIAL_HANDLE_ERROR);

        /* Register for interrupt events */
        touch->task.handler = touchPsoc4000s_InterruptHandler;
        touch->action_table = action_table;
        touch->action_table_size = size_action_table;
        PioMonitorRegisterTask(&touch->task, config->pios.interrupt);

    }

    return TaskList_AddTask(touch->clients, task);
}

void touchSensorClientUnRegister(Task task)
{
    touchTaskData *touch = &app_touch;
    TaskList_RemoveTask(touch->clients, task);
    if (0 == TaskList_Size(touch->clients))
    {
        TaskList_Destroy(touch->clients);
        touch->clients = NULL;

        PanicFalse(touch->handle != BITSERIAL_HANDLE_ERROR);

        /* Unregister for interrupt events */
        PioMonitorUnregisterTask(&touch->task, touch->config->pios.interrupt);

        touchPsoc4000s_Disable(touch->handle, touch->config);
        touch->handle = BITSERIAL_HANDLE_ERROR;
    }
}

#endif /* HAVE_TOUCHPAD_PSOC4000S */
#endif /* INCLUDE_CAPSENSE*/
