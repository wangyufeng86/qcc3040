/*!
\copyright  Copyright (c) 2019-2020 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Common kymera VA definitions

*/

#ifndef KYMERA_VA_COMMON_H_
#define KYMERA_VA_COMMON_H_

#include <chain.h>
#include <logging.h>
#include <panic.h>

#define PANIC(...) do{DEBUG_LOG_ERROR(__VA_ARGS__); Panic();}while(0);

typedef void (* OperatorFunction) (Operator *array, unsigned length_of_array);

typedef struct
{
    unsigned operator_role;
    void   (*ConfigureOperator)(Operator operator, const void *chain_configure_params);
} operator_config_map_t;

/*! \brief Configure operators in chain based on a map between the operator role and the function used to configure such an operator.
    \param chain Chain handle of the chain to be configured.
    \param op_config_map Table that maps each operator role to a function call used to configure it.
    \param maps_length Length of the op_config_map array.
    \param params Argument passed to each of the functions used in the op_config_map table (chain_configure_params).
*/
void Kymera_ConfigureChain(kymera_chain_handle_t chain, const operator_config_map_t *op_config_map, uint8 maps_length, const void *params);

#endif /* KYMERA_VA_COMMON_H_ */
