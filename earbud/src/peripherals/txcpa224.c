/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file       txcpa224.c
\brief      Support for the txcpa224 proximity sensor
*/

#ifdef INCLUDE_PROXIMITY
#ifdef HAVE_TXCPA224
#include <bitserial_api.h>
#include <panic.h>
#include <pio.h>
#include <pio_monitor.h>
#include <stdlib.h>

#include "adk_log.h"
#include "proximity.h"
#include "proximity_config.h"
#include "txcpa224.h"
#include "touch.h"

/*! \brief Returns the PIOs bank number.
    \param pio The pio.
*/
#define PIO2BANK(pio) ((uint16)((pio) / 32))
/*! \brief Returns the PIO bit position mask within a bank.
    \param pio The pio.
*/
#define PIO2MASK(pio) (1UL << ((pio) % 32))

const struct __proximity_config proximity_config = {
    .threshold_low = TXCPA224_PS_OFFSET_MIN,
    .threshold_high = TXCPA224_PS_OFFSET_MAX,
    .threshold_counts = 0,
    .rate = 0,
    .i2c_clock_khz = 100,
    .pios = {
        /* The PROXIMITY_PIO definitions are defined in the platform x2p file */
        .on = RDP_PIO_ON_PROX,
        .i2c_scl = RDP_PIO_I2C_SCL,
        .i2c_sda = RDP_PIO_I2C_SDA,
        .interrupt = RDP_PIO_INT_PROX,
    },
};

/*!< Task information for proximity sensor */
proximityTaskData app_proximity;

/*! \brief Read a register from the proximity sensor */
static bool txcpa224_ReadRegister(bitserial_handle handle, uint8 reg,  uint8 *value)
{
    bitserial_result result;
    /* First change I2C address to be read */
    result = BitserialChangeParam(handle,
                                  BITSERIAL_PARAMS_I2C_DEVICE_ADDRESS, 
                                  TXCPA224_I2C_ADDRESS,
                                  BITSERIAL_FLAG_BLOCK);
    if (BITSERIAL_RESULT_SUCCESS != result)
        return FALSE;
     
    result = BitserialWrite(handle,
                            BITSERIAL_NO_MSG,
                            &reg, 1,
                            BITSERIAL_FLAG_BLOCK);
    if (result == BITSERIAL_RESULT_SUCCESS)
    {
        /* Now read the actual data in the register */
        result = BitserialRead(handle,
                                BITSERIAL_NO_MSG,
                                value, 1,
                                BITSERIAL_FLAG_BLOCK);
    }

    return (result == BITSERIAL_RESULT_SUCCESS);
}

/*! \brief Write to a proximity sensor register */
static bool txcpa224_WriteRegister(bitserial_handle handle, uint8 reg, uint8 value)
{
    bitserial_result result;
    uint8 command[2] = {reg, value};

    /* First change I2C address to be read */
    result = BitserialChangeParam(handle,
                                  BITSERIAL_PARAMS_I2C_DEVICE_ADDRESS, 
                                  TXCPA224_I2C_ADDRESS,
                                  BITSERIAL_FLAG_BLOCK);
    if (BITSERIAL_RESULT_SUCCESS != result)
        return FALSE;

    /* Write the write command and register */
    result = BitserialWrite(handle,
                            BITSERIAL_NO_MSG,
                            command, 2,
                            BITSERIAL_FLAG_BLOCK);

    return (result == BITSERIAL_RESULT_SUCCESS);
}


static bool txcpa224_ReadInterruptStatus(bitserial_handle handle, uint8 *reg)
{
    return txcpa224_ReadRegister(handle, TXCPA224_CFG2_REG, reg);
}

/*! \brief  Retrieve proximity readings */
static bool txcpa224_PSReading(bitserial_handle handle, uint8 *value)
{
    return txcpa224_ReadRegister(handle, TXCPA224_PDH_REG, value);

}

/*! \brief Enable proximity readings */
static bool txcpa224_EnableProximity(bitserial_handle handle, uint8 enable)
{
    bool ret = FALSE;
    uint8 value = 0;
    value |= (enable<<1);
    if(txcpa224_WriteRegister(handle, TXCPA224_CFG0_REG, value))
    {
        ret = txcpa224_ReadRegister(handle, TXCPA224_CFG0_REG, &value);
    }
    return ret;
}

/*! \brief Read the proximity sensor device ID */
static bool txcpa224_ReadingDeviceID(bitserial_handle handle, uint8 *data)
{
    return txcpa224_ReadRegister(handle, TXCPA224_CHIP_ID_REG, data);
}

static bool txcpa224_SetCfg3Reg(bitserial_handle handle, uint8 type, uint8 sleep_cycle)
{
    bool ret = FALSE;
    uint8 value = 0;
    value |= (type << 6);
    value |= (sleep_cycle << 3);

    if(txcpa224_WriteRegister(handle, TXCPA224_CFG3_REG, value))
    {
        ret = txcpa224_ReadRegister(handle, TXCPA224_CFG3_REG, &value);
    }
    return ret;
}

static bool txcpa224_SetCfg1Reg(bitserial_handle handle, uint8 lesc, uint8 psprst)
{
    bool ret = FALSE;
    uint8 value = 0;
    
    value |= (lesc << 4);
    value |= (psprst << 2);
    if( txcpa224_WriteRegister(handle, TXCPA224_CFG1_REG, value))
    {
        ret = txcpa224_ReadRegister(handle, TXCPA224_CFG1_REG, &value);
    }
    return ret;
}

static bool txcpa224_SetCfg2Reg(bitserial_handle handle, uint8 ps_mode, uint8 intsrc)
{
    uint8 value = 0;
    bool ret = FALSE;

    value |= (ps_mode << 6);
    value |= (intsrc << 2);
    if(txcpa224_WriteRegister(handle, TXCPA224_CFG2_REG, value))
    {
        ret = txcpa224_ReadRegister(handle, TXCPA224_CFG2_REG, &value);
    }
    return ret;
}

static bool txcpa224_SetHighThreshold(bitserial_handle handle, uint8 threshold)
{
    bool ret = FALSE;
    uint8 value;
    if( txcpa224_WriteRegister(handle, TXCPA224_PTH_REG, threshold))
    {
        ret = txcpa224_ReadRegister(handle, TXCPA224_PTH_REG, &value);
    }
    return ret;
}

static bool txcpa224_SetLowThreshold(bitserial_handle handle, uint8 threshold)
{
    bool ret = FALSE;
    uint8 value;
    if( txcpa224_WriteRegister(handle, TXCPA224_PTL_REG, threshold))
    {
        ret = txcpa224_ReadRegister(handle, TXCPA224_PTL_REG, &value);
    }
    return ret;
}

static bool txcpa224_SetPSOffset(bitserial_handle handle, uint8 offset)
{
    bool ret = FALSE;
    uint8 value;
    if( txcpa224_WriteRegister(handle, TXCPA224_POFS1_REG, offset))
    {
        ret = txcpa224_ReadRegister(handle, TXCPA224_POFS1_REG, &value);
    }
    return ret;
}
static bool txcpa224_SetCfg4Reg(bitserial_handle handle)
{
    bool ret = FALSE;
    uint8 value;
    if( txcpa224_WriteRegister(handle, TXCPA224_CFG4_REG, 0x0C))
    {
        ret = txcpa224_ReadRegister(handle, TXCPA224_CFG4_REG, &value);
    }
    return ret;
}

static bool txcpa224_SetPOFS2PSReg(bitserial_handle handle)
{

    bool ret = FALSE;
    uint8 value;
    if( txcpa224_WriteRegister(handle, TXCPA224_POFS2_REG, 0x82))
    {
        ret = txcpa224_ReadRegister(handle, TXCPA224_POFS2_REG, &value);
    }
    return ret;
}

/*! \brief Handle the proximity interrupt */
static void txcpa224_InterruptHandler(Task task, MessageId id, Message msg)
{
    proximityTaskData *proximity = (proximityTaskData *) task;
    switch(id)
    {
        case MESSAGE_PIO_CHANGED:
        {
            const MessagePioChanged *mpc = (const MessagePioChanged *)msg;
            uint32 state = ((uint32)mpc->state16to31 << 16) + mpc->state;
            const proximityConfig *config = proximity->config;

            if (mpc->bank == PIO2BANK(config->pios.interrupt))
            {
                if (~state & PIO2MASK(config->pios.interrupt))
                {
                    uint8 isr;
                    uint8 ps_data;
                    uint8 is_interrupted;
                    PanicFalse(txcpa224_ReadInterruptStatus(proximity->handle, &isr));
                    PanicFalse(txcpa224_PSReading(proximity->handle, &ps_data));
                    /* check if interrupt bit is set */
                    is_interrupted = isr&0x02;
                    if (is_interrupted)
                    {
                        if (ps_data >= proximity->config->threshold_high)
                        {
                            DEBUG_LOG_VERBOSE("txcpa224InterruptHandler in proximity");
                            /* update thresholds so no more interrupt generated for the same state */
                            PanicFalse(txcpa224_SetHighThreshold(proximity->handle, TXCPA224_PS_OFFSET_FULL_RANGE));
                            PanicFalse(txcpa224_SetLowThreshold(proximity->handle, TXCPA224_PS_OFFSET_MIN));
                            proximity->state->proximity = proximity_state_in_proximity;
                            /* Inform clients */
                            TaskList_MessageSendId(proximity->clients, PROXIMITY_MESSAGE_IN_PROXIMITY);
                        }
                        else if (ps_data < proximity->config->threshold_low)
                        {
                            DEBUG_LOG_VERBOSE("txcpa224InterruptHandler not in proximity");
                            // update thresholds so no more interrupt generated for the same state
                            PanicFalse(txcpa224_SetLowThreshold(proximity->handle, TXCPA224_PS_OFFSET_LOWEST));
                            PanicFalse(txcpa224_SetHighThreshold(proximity->handle, TXCPA224_PS_OFFSET_MAX));
                            proximity->state->proximity = proximity_state_not_in_proximity;
                            /* Inform clients */
                            TaskList_MessageSendId(proximity->clients, PROXIMITY_MESSAGE_NOT_IN_PROXIMITY);
                        }
                    }
                    /* clear interrupt flag */
                    isr &= 0xFD;
                    PanicFalse(txcpa224_WriteRegister(proximity->handle, TXCPA224_CFG2_REG, isr));
                    PanicFalse(txcpa224_ReadInterruptStatus(proximity->handle, &isr));
                }
            }
        }
        break;
        default:
        break;
    }
}

/*! \brief Enable the proximity sensor */
static bitserial_handle txcpa224_Enable(const proximityConfig *config)
{
    uint16 bank;
    uint32 mask;
    bitserial_handle ret;
    uint32 i;
    bitserial_config bsconfig;

    struct
    {
        uint16 pio;
        pin_function_id func;
    } i2c_pios[] = {{config->pios.i2c_scl, BITSERIAL_1_CLOCK_OUT},
                    {config->pios.i2c_scl, BITSERIAL_1_CLOCK_IN},
                    {config->pios.i2c_sda, BITSERIAL_1_DATA_OUT},
                    {config->pios.i2c_sda, BITSERIAL_1_DATA_IN}};

    if (config->pios.on != TXCPA224_ON_PIO_UNUSED)
    {
        /* Setup power PIO then power-on the sensor */
        bank = PIO2BANK(config->pios.on);
        mask = PIO2MASK(config->pios.on);
        PanicNotZero(PioSetMapPins32Bank(bank, mask, mask));
        PanicNotZero(PioSetDir32Bank(bank, mask, mask));
        PanicNotZero(PioSet32Bank(bank, mask, mask));
    }

    /* Setup Interrupt as input with weak pull up */
    bank = PIO2BANK(config->pios.interrupt);
    mask = PIO2MASK(config->pios.interrupt);
    PanicNotZero(PioSetMapPins32Bank(bank, mask, mask));
    PanicNotZero(PioSetDir32Bank(bank, mask, 0));
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

    /* Configure Bitserial to work with txcpa224 proximity sensor */
    memset(&bsconfig, 0, sizeof(bsconfig));
    bsconfig.mode = BITSERIAL_MODE_I2C_MASTER;
    bsconfig.clock_frequency_khz = config->i2c_clock_khz;
    bsconfig.u.i2c_cfg.i2c_address = TXCPA224_I2C_ADDRESS;
    ret = BitserialOpen((bitserial_block_index)BITSERIAL_BLOCK_1, &bsconfig);

    if (ret != BITSERIAL_HANDLE_ERROR)
    {    
        DEBUG_LOG_INFO("txcpa224Enable Successful");
    }

    return ret;
}

/*! \brief Disable the proximity sensor txcpa224Disable*/
static void txcpa224_Disable(bitserial_handle handle, const proximityConfig *config)
{
    uint16 bank;
    uint32 mask;
    DEBUG_LOG_INFO("txcpa224_Disable");

    /* Disable interrupt and set weak pull down */
    bank = PIO2BANK(config->pios.interrupt);
    mask = PIO2MASK(config->pios.interrupt);
    PanicNotZero(PioSet32Bank(bank, mask, 0));

    /* Release bitserial instance */
    BitserialClose(handle);
    handle = BITSERIAL_HANDLE_ERROR;

    if (config->pios.on != TXCPA224_ON_PIO_UNUSED)
    {
        /* Power off the proximity sensor */
        PanicNotZero(PioSet32Bank(PIO2BANK(config->pios.on),
                                  PIO2MASK(config->pios.on),
                                  0));
    }
}

bool appProximityClientRegister(Task task)
{
    proximityTaskData *prox = ProximityGetTaskData();

    if (NULL == prox->clients)
    {
        uint8 first_data = 0xFF;
        const proximityConfig *config = appConfigProximity();
        prox->config = config;
        prox->state = PanicUnlessNew(proximityState);
        prox->state->proximity = proximity_state_in_proximity;
        prox->clients = TaskList_Create();

        prox->handle = txcpa224_Enable(config);
        PanicFalse(prox->handle != BITSERIAL_HANDLE_ERROR);
        PanicFalse(txcpa224_ReadingDeviceID(prox->handle, &first_data));
        DEBUG_LOG_VERBOSE("Sensor ID : %02x", first_data);

        /* config the registers */
        PanicFalse(txcpa224_SetCfg1Reg(prox->handle, TXCPA224_PS_CURRENT_10MA, TXCPA224_PS_PRST_2PTS));
        PanicFalse(txcpa224_SetCfg3Reg(prox->handle, TXCPA224_PS_ISR_WINDOW_TYPE, TXCPA224_PS_SLEEP_PERIOD_625));
        PanicFalse(txcpa224_SetPOFS2PSReg(prox->handle));
        PanicFalse(txcpa224_SetCfg4Reg(prox->handle));
        PanicFalse(txcpa224_SetPSOffset(prox->handle, TXCPA224_PS_DEFAULT_CROSSTALK));

        /* set high/low to 255/0 */
        PanicFalse(txcpa224_SetHighThreshold(prox->handle, TXCPA224_PS_OFFSET_FULL_RANGE));
        PanicFalse(txcpa224_SetLowThreshold(prox->handle, TXCPA224_PS_OFFSET_LOWEST));

        PanicFalse(txcpa224_SetCfg1Reg(prox->handle, TXCPA224_PS_CURRENT_10MA, TXCPA224_PS_PRST_2PTS));
        PanicFalse(txcpa224_SetCfg2Reg(prox->handle, TXCPA224_PS_MODE_OFFSET, TXCPA224_PS_INT_SELECT_MODE));

        PanicFalse(txcpa224_EnableProximity(prox->handle, TXCPA224_PS_MODE_ENABLE));
        /* add a delay to wait for PS sensor to be stable */
        int i;
        for (i=0; i<1000; i++);

        /* read first set of data */
        PanicFalse(txcpa224_PSReading(prox->handle, &first_data));

        if (first_data <= (TXCPA224_PS_OFFSET_MIN))
        {
            prox->state->proximity = proximity_state_not_in_proximity;
            /* update thresholds so no more interrupt generated for the same state */
            PanicFalse(txcpa224_SetLowThreshold(prox->handle, TXCPA224_PS_OFFSET_LOWEST));
            PanicFalse(txcpa224_SetHighThreshold(prox->handle, TXCPA224_PS_OFFSET_MAX));
        
        }
        else if (first_data >= (TXCPA224_PS_OFFSET_MAX))
        {
            prox->state->proximity = proximity_state_in_proximity;
            /* update thresholds so no more interrupt generated for the same state */
            PanicFalse(txcpa224_SetHighThreshold(prox->handle, TXCPA224_PS_OFFSET_FULL_RANGE));
            PanicFalse(txcpa224_SetLowThreshold(prox->handle, TXCPA224_PS_OFFSET_MIN));
        }
        /* Register for interrupt events */
        prox->task.handler = txcpa224_InterruptHandler;
        PioMonitorRegisterTask(&prox->task, prox->config->pios.interrupt);
        
    }

    /* Send initial message to client */
    switch (prox->state->proximity)
    {
        case proximity_state_in_proximity:
            MessageSend(task, PROXIMITY_MESSAGE_IN_PROXIMITY, NULL);
            break;
        case proximity_state_not_in_proximity:
            MessageSend(task, PROXIMITY_MESSAGE_NOT_IN_PROXIMITY, NULL);
            break;
        case proximity_state_unknown:
        default:
            /* The client will be informed after the first interrupt */
            break;
    }
    DEBUG_LOG_VERBOSE("Proximity Client registered");

    return TaskList_AddTask(prox->clients, task);
}

void appProximityClientUnregister(Task task)
{
    proximityTaskData *prox = ProximityGetTaskData();
    TaskList_RemoveTask(prox->clients, task);
    if (0 == TaskList_Size(prox->clients))
    {
        TaskList_Destroy(prox->clients);
        prox->clients = NULL;
        free(prox->state);
        prox->state = NULL;

        PanicFalse(prox->handle != BITSERIAL_HANDLE_ERROR);

        /* Unregister for interrupt events */
        PioMonitorUnregisterTask(&prox->task, prox->config->pios.interrupt);

        /* Reset into lowest power mode in case the sensor is not powered off. */
        txcpa224_Disable(prox->handle, prox->config);
        prox->handle = BITSERIAL_HANDLE_ERROR;
    }
}

#endif /* HAVE_TXCPA224 */
#endif /* INCLUDE_PROXIMITY */
