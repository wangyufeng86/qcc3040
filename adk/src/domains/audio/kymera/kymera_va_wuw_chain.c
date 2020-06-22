/*!
\copyright  Copyright (c) 2019-2020 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Kymera module to handle VA Wake-Up-Word chain

*/

#include "kymera_va_wuw_chain.h"
#include "kymera_private.h"
#include "kymera_va_mic_chain.h"
#include "kymera_va_common.h"
#include "kymera_chain_roles.h"

static void kymera_ConfigureGraphManager(Operator graph_manager, const void *params);
static void kymera_ConfigureWuwEngine(Operator wuw, const void *params);
static const operator_config_map_t operator_config_map[] =
{
    {OPR_VA_GRAPH_MANAGER, kymera_ConfigureGraphManager},
    {OPR_WUW, kymera_ConfigureWuwEngine}
};

static const appKymeraVaWuwChainTable *chain_config_map = NULL;

static kymera_chain_handle_t va_wuw_chain = NULL;
static DataFileID wuw_model_handle = DATA_FILE_ID_INVALID;

static void kymera_ConfigureGraphManager(Operator graph_manager, const void *params)
{
    const va_wuw_chain_op_params_t *op_params = params;
    MessageOperatorTask(graph_manager, op_params->wuw_detection_handler);
}

static void kymera_ConfigureWuwEngine(Operator wuw, const void *params)
{
    const va_wuw_chain_op_params_t *op_params = params;

    if (wuw_model_handle == DATA_FILE_ID_INVALID)
    {
        wuw_model_handle = PanicZero(OperatorDataLoadEx(op_params->wuw_model, DATAFILE_BIN, STORAGE_INTERNAL, FALSE));
    }

    OperatorsStandardSetSampleRate(wuw, Kymera_GetVaMicChainOutputSampleRate());
    OperatorsConfigureQvaTriggerPhrase(wuw, wuw_model_handle);
}

static const chain_config_t * kymera_GetChainConfig(const va_wuw_chain_params_t *params)
{
    unsigned i;
    for(i = 0; i < chain_config_map->table_length; i++)
    {
        if (chain_config_map->chain_table[i].wuw_engine == params->wuw_engine)
        {
            return chain_config_map->chain_table[i].chain_config;
        }
    }

    PANIC("kymera_GetChainConfig: Wake-Up-Word engine not supported!");
    return NULL;
}

static Operator kymera_GetChainOperator(unsigned operator_role)
{
    PanicNull(va_wuw_chain);
    return ChainGetOperatorByRole(va_wuw_chain, operator_role);
}

static Sink kymera_GetChainInput(unsigned input_role)
{
    PanicNull(va_wuw_chain);
    return ChainGetInput(va_wuw_chain, input_role);
}

static void kymera_CreateChain(const va_wuw_chain_params_t *params)
{
    PanicNotNull(va_wuw_chain);
    va_wuw_chain = PanicNull(ChainCreate(kymera_GetChainConfig(params)));
}

static void kymera_ConfigureChain(const va_wuw_chain_op_params_t *params)
{
    Kymera_ConfigureChain(va_wuw_chain, operator_config_map, ARRAY_DIM(operator_config_map), params);
}

static void kymera_ConnectChain(void)
{
    ChainConnect(va_wuw_chain);
    PanicNull(StreamConnect(Kymera_GetVaMicChainWuwOutput(), kymera_GetChainInput(EPR_VA_WUW_IN)));
}

static void kymera_DisconnectChain(void)
{
    StreamDisconnect(NULL, kymera_GetChainInput(EPR_VA_WUW_IN));
}

static void kymera_RunUsingOperatorsNotToPreserve(OperatorFunction function)
{
    Operator ops[] = {kymera_GetChainOperator(OPR_VA_GRAPH_MANAGER), kymera_GetChainOperator(OPR_WUW)};
    function(ops, ARRAY_DIM(ops));
}

static void kymera_ChainSleep(Operator *array, unsigned length_of_array)
{
    operator_list_t operators_to_exclude = {array, length_of_array};
    ChainSleep(va_wuw_chain, &operators_to_exclude);
}

static void kymera_ChainWake(Operator *array, unsigned length_of_array)
{
    operator_list_t operators_to_exclude = {array, length_of_array};
    ChainWake(va_wuw_chain, &operators_to_exclude);
}

void Kymera_CreateVaWuwChain(const va_wuw_chain_create_params_t *params)
{
    PanicFalse(params != NULL);
    kymera_CreateChain(&params->chain_params);
    kymera_ConfigureChain(&params->operators_params);
    kymera_ConnectChain();
}

void Kymera_DestroyVaWuwChain(void)
{
    kymera_DisconnectChain();
    if (wuw_model_handle != DATA_FILE_ID_INVALID)
    {
        PanicFalse(OperatorDataUnloadEx(wuw_model_handle));
        wuw_model_handle = DATA_FILE_ID_INVALID;
    }
    ChainDestroy(va_wuw_chain);
    va_wuw_chain = NULL;
}

void Kymera_StartVaWuwChain(void)
{
    ChainStart(va_wuw_chain);
}

void Kymera_StopVaWuwChain(void)
{
    ChainStop(va_wuw_chain);
}

void Kymera_VaWuwChainSleep(void)
{
    PanicFalse(OperatorFrameworkTriggerNotificationStart(TRIGGER_ON_GM, kymera_GetChainOperator(OPR_VA_GRAPH_MANAGER)));
    kymera_RunUsingOperatorsNotToPreserve(kymera_ChainSleep);
}

void Kymera_VaWuwChainWake(void)
{
    kymera_RunUsingOperatorsNotToPreserve(kymera_ChainWake);
    PanicFalse(OperatorFrameworkTriggerNotificationStop());
}

void Kymera_VaWuwChainStartGraphManagerDelegation(void)
{
    Kymera_VaMicChainStartGraphManagerDelegation(kymera_GetChainOperator(OPR_VA_GRAPH_MANAGER), kymera_GetChainOperator(OPR_WUW));
}

void Kymera_VaWuwChainStopGraphManagerDelegation(void)
{
    Kymera_VaMicChainStopGraphManagerDelegation(kymera_GetChainOperator(OPR_VA_GRAPH_MANAGER), kymera_GetChainOperator(OPR_WUW));
}

void Kymera_SetVaWuwChainTable(const appKymeraVaWuwChainTable *chain_table)
{
    chain_config_map = chain_table;
}
