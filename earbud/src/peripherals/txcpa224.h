/*!
\copyright  Copyright (c) 2017 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file       txcpa224.h
\brief      Header file for txcpa224 Accelerometer
*/

#ifndef TXCPA224_PRIVATE_H
#define TXCPA224_PRIVATE_H

#ifdef HAVE_TXCPA224

#include <types.h>

/*! I2C communication addresses */
#define TXCPA224_I2C_ADDRESS                (0x1E)

/*! @name txcpa224 I2C registers. */
//!@{
// all registers need to ship 3 bits as 
#define TXCPA224_CFG0_REG                (0x00)
#define TXCPA224_CFG1_REG                (0x01)
#define TXCPA224_CFG2_REG                (0x02)
#define TXCPA224_CFG3_REG                (0x03)
#define TXCPA224_ATLL_REG                (0x04)
#define TXCPA224_ATLH_REG                (0x05)
#define TXCPA224_ATHL_REG                (0x06)
#define TXCPA224_ATHH_REG                (0x07)
#define TXCPA224_PTL_REG                 (0x08)
#define TXCPA224_PTM_REG                 (0x09)
#define TXCPA224_PTH_REG                 (0x0A)
#define TXCPA224_ADL_REG                 (0x0B)
#define TXCPA224_ADH_REG                 (0x0C)
#define TXCPA224_PDH_REG                 (0x0E)
#define TXCPA224_POFS1_REG               (0x10)
#define TXCPA224_POFS2_REG               (0x11)
#define TXCPA224_CFG4_REG                (0x12)
#define TXCPA224_CHIP_ID_REG             (0x7F)

#define TXCPA224_PS_ISR_WINDOW_TYPE      (0x00)
#define TXCPA224_PS_ISR_HISTERESIS_TYPE  (0x01)

#define TXCPA224_PS_CURRENT_15MA         (0x04)
#define TXCPA224_PS_CURRENT_12MA         (0x05)
#define TXCPA224_PS_CURRENT_10MA         (0x06)

// 2points preset
#define TXCPA224_PS_PRST_1PTS	         (0x00)
#define TXCPA224_PS_PRST_2PTS	         (0x01)
#define TXCPA224_PS_PRST_4PTS	         (0x02)

#define TXCPA224_PS_MODE_OFFSET          (0x00)
#define TXCPA224_PS_MODE_NORMAL          (0x01)

#define TXCPA224_PS_SLEEP_PERIOD_625     (0x00)
#define TXCPA224_PS_SLEEP_PERIOD_1250    (0x01)
#define TXCPA224_PS_SLEEP_PERIOD_2500    (0x02)

#define TXCPA224_PS_OFFSET_MAX           (150)
#define TXCPA224_PS_OFFSET_MIN           (100)
#define TXCPA224_PS_OFFSET_FULL_RANGE    (0xFF)
#define TXCPA224_PS_OFFSET_LOWEST        (0x00)

#define TXCPA224_PS_MODE_ENABLE          (0x01)
#define TXCPA224_PS_MODE_DISABLE         (0x00)

#define TXCPA224_PS_INT_SELECT_MODE      (0x01)

#define TXCPA224_PS_DEFAULT_CROSSTALK    (0x05)

//!@}

/*! Important note, this code assumes the compiler fills bitfields from the
    least significant bit, as KCC does, such that the following register
    bitfield definitions correctly map onto the physical bits */
#ifndef __KCC__
#warning "Not compiling with KCC. The bitfield register definitions may be incorrect."
#endif

/*! This register is for starting proximity measurements */
typedef union txcpa224_cfg0_register
{
    /*! The register bit fields */
    struct
    {
        /*! Function of this bit is not defined */
        unsigned _unused : 6;
        /*! R/W bit. Enables periodic proximity measurement */
        unsigned prox_en : 1;
        /*! Function of this bit is not defined */
        unsigned __unused : 1;
    } bits;
    /*! The entire register as a byte */
    uint8 reg;
} txcpa224_cfg0_register_t;


/*! This register is for starting proximity measurements */
typedef union txcpa224_cfg1_register
{
    /*! The register bit fields */
    struct
    {
        /*! Function of this bit is not defined */
        unsigned _unused : 1;
        /*! R/W bit. Light Emitting source current setting */
        unsigned LESC : 3;
        /*! R/W bit. PS interrupt persistent */
        unsigned PSPRST : 2;
        /*! Function of this bit is not defined */
        unsigned __unused : 2;
    } bits;
    /*! The entire register as a byte */
    uint8 reg;
} txcpa224_cfg1_register_t;

/*! This register is for starting proximity measurements */
typedef union txcpa224_cfg2_register
{
    /*! The register bit fields */
    struct
    {
        /*! Function of this bit is not defined */
        unsigned _unused : 1;
        /*! R/W bit. PS offset mode selection */
        unsigned PSMODESEL : 1;
        /*! Function of this bit is not defined */
        unsigned __unused : 1;
        /*! R/W bit. Reset command */
        unsigned CMR : 1;
        /*! R/W bit. Interrupt source selection */
        unsigned INTSEL : 2;
        /*! R/W bit. PS Interrupt Flag */
        unsigned PSINTF : 1;
        /*! Function of this bit is not defined */
        unsigned ___unused : 1;
    } bits;
    /*! The entire register as a byte */
    uint8 reg;
} txcpa224_cfg2_register_t;

/*! This register is for starting proximity measurements */
typedef union txcpa224_cfg3_register
{
    /*! The register bit fields */
    struct
    {
        /*! Function of this bit is not defined */
        unsigned _unused : 1;
        /*! R/W bit. Interrupt type for PS */
        unsigned PITYPE : 1;
        /*! R/W bit. PS Sleep Time */
        unsigned PSSLP : 3;
        /*! Function of this bit is not defined */
        unsigned __unused : 3;
    } bits;
    /*! The entire register as a byte */
    uint8 reg;
} txcpa224_cfg3_register_t;

/*! The 'on' PIO will not be controlled when __proximity_config.pio.on is set to
    this value */
#define TXCPA224_ON_PIO_UNUSED 255

/*! The high level configuration for taking measurement */
struct __proximity_config
{
    /*! Measurements higher than this value will result in the sensor considering
        it is in-proximity of an object */
    uint8 threshold_high;
    /*! Measurements lower than this value will result in the sensor considering
        it is not in-proximity of an object */
    uint8 threshold_low;
    /*! The I2C clock frequency */
    uint16 i2c_clock_khz;
    /*! The number of measurements above/below the threshold before the sensor
        generates an interrupt */
    uint8 threshold_counts;
    /*! The number of measurements per second */
    uint8 rate;
    /*! The PIOs used to control/communicate with the sensor */
    struct
    {
        /*! PIO used to power-on the sensor, or #VNCL3020_ON_PIO_UNUSED */
        uint8 on;
        /*! Interrupt PIO driven by the sensor */
        uint8 interrupt;
        /*! I2C serial data PIO */
        uint8 i2c_sda;
        /*! I2C serial clock PIO */
        uint8 i2c_scl;
    } pios;
};

/*! Internal representation of proximity state */
enum proximity_states
{
    proximity_state_unknown,
    proximity_state_in_proximity,
    proximity_state_not_in_proximity
};

/*! Trivial state for storing in-proximity state */
struct __proximity_state
{
    /*! The sensor proximity state */
    enum proximity_states proximity;
};

#endif /* HAVE_TXCPA224 */
#endif /* TXCPA224_PRIVATE_H */
