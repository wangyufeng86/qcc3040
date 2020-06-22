/*!
\copyright  Copyright (c) 2019-2020 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Kymera module to handle VA Wake-Up-Word chain

*/

#ifndef KYMERA_VA_WUW_CHAIN_H_
#define KYMERA_VA_WUW_CHAIN_H_

#include "va_audio_types.h"

/*! \brief Parameters used to determine the chain to use/create */
typedef struct
{
    va_wuw_engine_t wuw_engine;
} va_wuw_chain_params_t;

/*! \brief Parameters used to configure the VA Wake-Up-Word chain operators */
typedef struct
{
    Task       wuw_detection_handler;
    FILE_INDEX wuw_model;
} va_wuw_chain_op_params_t;

/*! \brief Parameters used to create the VA Wake-Up-Word chain */
typedef struct
{
    va_wuw_chain_params_t    chain_params;
    va_wuw_chain_op_params_t operators_params;
} va_wuw_chain_create_params_t;

/*! \brief Create VA Wake-Up-Word chain.
           Must be called after VA mic chain is instantiated, since it will connect to it.
    \param params Parameters used to create/configure the chain.
*/
void Kymera_CreateVaWuwChain(const va_wuw_chain_create_params_t *params);
void Kymera_DestroyVaWuwChain(void);

void Kymera_StartVaWuwChain(void);
void Kymera_StopVaWuwChain(void);

void Kymera_VaWuwChainSleep(void);
void Kymera_VaWuwChainWake(void);

void Kymera_VaWuwChainStartGraphManagerDelegation(void);
void Kymera_VaWuwChainStopGraphManagerDelegation(void);

#endif /* KYMERA_VA_WUW_CHAIN_H_ */
