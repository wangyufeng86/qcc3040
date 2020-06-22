/****************************************************************************
Copyright (c) 2016-2020 Qualcomm Technologies International, Ltd.

FILE NAME
    chain.c

DESCRIPTION
    Chain library implementation.
*/

#include "chain.h"
#include "chain_private.h"
#include "chain_list.h"
#include "chain_path.h"
#include "chain_connect.h"
#include "chain_config.h"

#include <vmal.h> 
#include <panic.h>
#include <stream.h>
#include <stdlib.h>
#include <operators.h>
#include <print.h>
#include <string.h>
#include <audio_processor.h>

typedef bool (*OperatorFunction)(operator_list_t operators);

static bool isInOperatorList(Operator op, const operator_list_t *list)
{
    unsigned i;

    if (list == NULL)
        return FALSE;

    for(i = 0; i < list->length; i++)
    {
        if (list->array[i] == op)
        {
            return TRUE;
        }
    }

    return FALSE;
}

/*! \brief Populate operator list with valid chain operators.
    \param operators List of operators to populate.
    \param chain Chain handle.
    \param operators_to_exclude Operators to exclude from the list.
    \return TRUE if successful, FALSE if the list provided was too short.
*/
static bool populateWithOperatorsInChain(operator_list_t *operators, const kymera_chain_t *chain, const operator_list_t *operators_to_exclude)
{
    unsigned i, num_of_operators = 0;
    bool status = TRUE;

    for(i = 0; i < chain->config->number_of_operators; i++)
    {
        if ((chain->operator_list[i] != INVALID_OPERATOR) &&
            (isInOperatorList(chain->operator_list[i], operators_to_exclude) == FALSE))
        {
            if (num_of_operators < operators->length)
            {
                operators->array[num_of_operators] = chain->operator_list[i];
                num_of_operators++;
            }
            else
            {
                status = FALSE;
                break;
            }
        }
    }

    operators->length = num_of_operators;

    return status;
}

static bool runFunctionOnMultipleOperators(OperatorFunction operatorFunction, const kymera_chain_t *chain,
                                           const operator_list_t *operators_to_exclude)
{
    operator_list_t operators;
    bool            result = TRUE;
    operators.length = chain->config->number_of_operators;
    operators.array = PanicUnlessMalloc(operators.length * sizeof(Operator));
    PanicFalse(populateWithOperatorsInChain(&operators, chain, operators_to_exclude));

    if (operators.length)
    {
        result = operatorFunction(operators);
    }

    free(operators.array);
    return result;
}

static bool isRoleExcluded(unsigned role, const unsigned *excluded_roles, unsigned excluded_roles_count)
{
    unsigned i;

    for(i = 0; i < excluded_roles_count; i++)
    {
        if(excluded_roles[i] == role)
        {
            return TRUE;
        }
    }

    return FALSE;
}

static bool preserveOperators(operator_list_t operators)
{
    return OperatorFrameworkPreserve((uint16)operators.length, operators.array, 0, NULL, 0, NULL);
}

static bool releaseOperators(operator_list_t operators)
{
    return OperatorFrameworkRelease((uint16)operators.length, operators.array, 0, NULL, 0, NULL);
}

static bool startOperators(operator_list_t operators)
{
    return OperatorStartMultiple((uint16)operators.length, operators.array, NULL);
}

static bool stopOperators(operator_list_t operators)
{
    return OperatorStopMultiple((uint16)operators.length, operators.array, NULL);
}

/* Free all memory associated with the chain */
static void chainFreeMemory(kymera_chain_t *chain)
{
    if (chain != NULL)
    {
        free(chain->operator_list);
        free(chain->filters.operator_filters);
        free(chain);
    }
}

/* Allocate all memory associated with the chain */
static kymera_chain_t* chainAllocateMemory(const chain_config_t *config, const operator_filters_t* filter)
{
    kymera_chain_t *chain;
    
    if(!config)
        return NULL;
    
    chain = calloc(1, sizeof(*chain));
    
    if (chain)
    {
        chain->operator_list = calloc(config->number_of_operators, sizeof(Operator));
        
        if(chain->operator_list)
        {
            if(!filter)
                return chain;
            
            chain->filters.operator_filters = calloc(filter->num_operator_filters, sizeof(operator_config_t));
            if(chain->filters.operator_filters)
                return chain;
        }
        
        chainFreeMemory(chain);
    }
    
    return NULL;
}

/* Create and add all non-filtered operators to the chain */
static void chainAddOperators(kymera_chain_t *chain)
{
    unsigned i;
    const chain_config_t *config = chain->config;
    
    for (i = 0; i < config->number_of_operators; i++)
    {
        const operator_config_t* op_config = chainConfigGetOperatorConfig(chain, i);
        if(op_config)
        {
            /* Operator role must be unique within the chain */
            PanicFalse(ChainGetOperatorByRole(chain, op_config->role) == INVALID_OPERATOR);
            chain->operator_list[i] = CustomOperatorCreate(op_config->capability_id, op_config->processor_id, op_config->priority, &op_config->setup);
        }
    }
}

/* Start any required DSP processors, main processor is always required to be on */
static void processorsEnable(kymera_chain_t *chain)
{
    PanicFalse(chain->chain_enabled == FALSE);

    PanicFalse(VmalOperatorFrameworkEnableMainProcessor(TRUE));

    if (chainConfigUsesSecondProcessor(chain))
    {
        PanicFalse(VmalOperatorFrameworkEnableSecondProcessor(TRUE));
    }

    chain->chain_enabled = TRUE;
}

/* Stop any required DSP processors */
static void processorsDisable(kymera_chain_t *chain)
{
    PanicFalse(chain->chain_enabled == TRUE);

    if (chainConfigUsesSecondProcessor(chain))
    {
        PanicFalse(VmalOperatorFrameworkEnableSecondProcessor(FALSE));
    }
    PanicFalse(VmalOperatorFrameworkEnableMainProcessor(FALSE));

    chain->chain_enabled = FALSE;
}

static void destroyOperators(const kymera_chain_t *chain)
{
    operator_list_t operators;
    operators.length = chain->config->number_of_operators;
    operators.array = PanicUnlessMalloc(operators.length * sizeof(Operator));
    PanicFalse(populateWithOperatorsInChain(&operators, chain, NULL));
    CustomOperatorDestroy(operators.array, operators.length);
    free(operators.array);
}

/******************************************************************************/
static unsigned ChainGetNumberOfRoles(kymera_chain_handle_t handle)
{
    kymera_chain_t *chain = handle;

    if(chainConfigIsStreamBased(chain))
    {
        return chainStreamGetNumberOfStreams(chain);
    }
    else
    {
        return chainConnectGetNumberOfOutputs(chain);
    }
}

static unsigned ChainGetRoleByNumber(kymera_chain_handle_t handle, unsigned number)
{
    kymera_chain_t *chain = handle;

    if(chainConfigIsStreamBased(chain))
    {
        return chainStreamGetRoleByStreamNumber(chain, number);
    }
    else
    {
        return chainConnectGetRoleByOutputEndpointNumber(chain, number);
    }
}

static void ChainConfigureSampleRateAndBufferSize(kymera_chain_handle_t handle, uint32 sample_rate, const unsigned *excluded_roles, unsigned excluded_roles_count, bool update_buffer_size)
{
    kymera_chain_t *chain = handle;
    unsigned i;

    if(chain)
    {
        for(i = 0; i < chain->config->number_of_operators; i++)
        {
            Operator op;
            const operator_config_t* op_config = chainConfigGetOperatorConfig(chain, i);

            if(!op_config)
                continue;

            if(excluded_roles && isRoleExcluded(op_config->role, excluded_roles, excluded_roles_count))
                continue;

            op = ChainGetOperatorByRole(handle, op_config->role);
            OperatorsStandardSetSampleRate(op, sample_rate);

            if(update_buffer_size)
            {
                OperatorsStandardSetBufferSizeFromSampleRate(op, sample_rate, &op_config->setup);
            }
        }
    }

}

/******************************************************************************/
kymera_chain_handle_t ChainCreate(const chain_config_t *config)
{
    return ChainCreateWithFilter(config, NULL);
}

/******************************************************************************/
kymera_chain_handle_t ChainCreateWithFilter(const chain_config_t *config, const operator_filters_t* filter)
{
    kymera_chain_t *chain;
    
    chain = chainAllocateMemory(config, filter);
    
    if(!chain)
        return NULL;
    
    chainConfigStore(chain, config, filter);
    
    if(chainConfigIsWholeChainFiltered(chain))
    {
        chainFreeMemory(chain);
        return NULL;
    }

    chain->chain_enabled = FALSE;

    processorsEnable(chain);
    chainListAdd(chain);
    chainAddOperators(chain);
    AudioProcessorAddUseCase(config->audio_ucid);

    PRINT(("ChainCreate() %p\n", chain));

    return chain;
}

/******************************************************************************/
void ChainDestroy(kymera_chain_handle_t handle)
{
    kymera_chain_t *chain = handle;
    PRINT(("ChainDestroy() %p\n", chain));

    if(chain != NULL)
    {
        destroyOperators(chain);
        AudioProcessorRemoveUseCase(chain->config->audio_ucid);
        chainListRemove(chain);
        processorsDisable(chain);
        chainFreeMemory(chain);
    }
}

/******************************************************************************/
Operator ChainGetOperatorByRole(kymera_chain_handle_t handle, unsigned operator_role)
{
    kymera_chain_t *chain = handle;
    if(chain)
    {
        const chain_config_t *config = chain->config;
        unsigned i;
        for(i = 0; i < config->number_of_operators; ++i)
        {
            if(config->operator_config[i].role == operator_role)
            {
                return chain->operator_list[i];
            }
        }
    }

    /* No match */
    return INVALID_OPERATOR;
}

/******************************************************************************/
Sink ChainGetInput(kymera_chain_handle_t handle, unsigned input_role)
{
    kymera_chain_t *chain = handle;
    Sink sink = NULL;

    PRINT(("Get Input %d\n", input_role));
    if(chain)
    {
        if(chainConfigIsStreamBased(chain))
        {
            sink = chainPathGetInput(chain, input_role);
        }
        else
        {
            sink = chainConnectGetInput(chain, input_role);
        }
    }
    return sink;
}

/******************************************************************************/
Source ChainGetOutput(kymera_chain_handle_t handle, unsigned output_role)
{
    kymera_chain_t *chain = handle;
    Source source = NULL;

    PRINT(("Get Output %d\n", output_role));
    if(chain)
    {
        if(chainConfigIsStreamBased(chain))
        {
            source = chainPathGetOutput(chain, output_role);
        }
        else
        {
            source = chainConnectGetOutput(chain, output_role);
        }
    }
    return source;
}

/******************************************************************************/
void ChainConnect(kymera_chain_handle_t handle)
{
    kymera_chain_t *chain = handle;
    PanicNull(chain);
    
    if(chainConfigIsStreamBased(chain))
    {
        chainPathConnect(chain);
    }
    else
    {
        chainConnectAllOperators(chain);
    }
}

/******************************************************************************/
void ChainConnectWithPath(kymera_chain_handle_t handle, unsigned path_role)
{
    kymera_chain_t *chain = handle;
    PanicNull(chain);

    if(chainConfigIsStreamBased(chain))
    {
        chainPathConnectPath(chain, path_role);
    }
    else
    {
        Panic();
    }
}

bool ChainConnectInput(kymera_chain_handle_t handle, Source source, unsigned input_role)
{
    return (StreamConnect(source, ChainGetInput(handle, input_role)) != NULL);
}

bool ChainConnectOutput(kymera_chain_handle_t handle, Sink sink, unsigned output_role)
{
    return (StreamConnect(ChainGetOutput(handle, output_role), sink) != NULL);
}

/******************************************************************************/
void ChainStart(kymera_chain_handle_t handle)
{
    kymera_chain_t *chain = handle;
    PanicNull(chain);
    
    PRINT(("ChainStart() %p\n", chain));

    PanicFalse(runFunctionOnMultipleOperators(startOperators, chain, NULL));
}

/******************************************************************************/
bool ChainStartAttempt(kymera_chain_handle_t handle)
{
    kymera_chain_t *chain = handle;
    PanicNull(chain);

    PRINT(("ChainStartAttempt() %p\n", chain));

    if (!runFunctionOnMultipleOperators(startOperators, chain, NULL))
    {
        PRINT(("ChainStartAttempt() was unable to start all operators. Stopping started operators.\n"));
        ChainStop(chain);
        return FALSE;
    }
    return TRUE;
}

/******************************************************************************/
void ChainStop(kymera_chain_handle_t handle)
{
    kymera_chain_t *chain = handle;

    PanicNull(chain);
    PRINT(("ChainStop() %p\n", chain));

    PanicFalse(runFunctionOnMultipleOperators(stopOperators, chain, NULL));
}

/******************************************************************************/
void ChainJoin(kymera_chain_handle_t source_chain, kymera_chain_handle_t sink_chain, unsigned count, const chain_join_roles_t *connect_list)
{
    unsigned i;
    Source output_src;
    Sink input_snk;

    PanicNull(source_chain);
    PanicNull(sink_chain);

    for (i = 0; i < count; i++)
    {
        output_src = ChainGetOutput(source_chain, connect_list[i].source_role);
        input_snk = ChainGetInput(sink_chain, connect_list[i].sink_role);
        PanicNull(StreamConnect(output_src, input_snk));
    }
}

/******************************************************************************/
unsigned ChainJoinMatchingRoles(kymera_chain_handle_t source_chain, kymera_chain_handle_t sink_chain, unsigned count)
{
    unsigned role_count;
    unsigned number_roles;
    Source output_source;
    unsigned role;
    unsigned connected=0;

    PanicNull(source_chain);
    PanicNull(sink_chain);

    number_roles = ChainGetNumberOfRoles(source_chain);

    for(role_count=0; ((role_count<number_roles) && (count > connected)); role_count++)
    {
        role = ChainGetRoleByNumber(source_chain, role_count);
        output_source = ChainGetOutput(source_chain, role);

        if(ChainConnectInput(sink_chain, output_source, role))
        {
            connected++;
        }
    }

    return connected;
}

/******************************************************************************/
void ChainConfigure(kymera_chain_handle_t handle, const chain_operator_message_t *messages,  unsigned number_of_messages)
{
    kymera_chain_t *chain = handle;
    if (chain && messages)
    {
        const chain_operator_message_t *msg;
        for (msg = messages; msg < messages + number_of_messages; msg++)
        {
            Operator op = ChainGetOperatorByRole(chain, msg->operator_role);
            PanicFalse(VmalOperatorMessage(op, msg->message, msg->message_length, NULL, 0));
        }
    }
}

/******************************************************************************/
void ChainConfigureSampleRate(kymera_chain_handle_t handle, uint32 sample_rate, const unsigned *excluded_roles, unsigned excluded_roles_count)
{
    ChainConfigureSampleRateAndBufferSize(handle,sample_rate,excluded_roles,excluded_roles_count,TRUE);
}

/******************************************************************************/
void ChainConfigureSampleRateWithoutAllocatingBuffer(kymera_chain_handle_t handle, uint32 sample_rate, const unsigned *excluded_roles, unsigned excluded_roles_count)
{
    ChainConfigureSampleRateAndBufferSize(handle,sample_rate,excluded_roles,excluded_roles_count,FALSE);
}

/******************************************************************************/
audio_ucid_t ChainGetUseCase(const chain_config_t *config)
{
    if(config)
    {
        return config->audio_ucid;
    }
    
    return audio_ucid_number_of_ucids;
}

/******************************************************************************/
void ChainWake(kymera_chain_handle_t chain, const operator_list_t *operators_to_exclude)
{
    PanicNull(chain);
    processorsEnable(chain);
    PanicFalse(runFunctionOnMultipleOperators(releaseOperators, chain, operators_to_exclude));
}

/******************************************************************************/
void ChainSleep(kymera_chain_handle_t chain, const operator_list_t *operators_to_exclude)
{
    PanicNull(chain);
    PanicFalse(runFunctionOnMultipleOperators(preserveOperators, chain, operators_to_exclude));
    processorsDisable(chain);
}
