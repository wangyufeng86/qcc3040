/*!
\copyright  Copyright (c) 2019-2020 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Kymera module with kymera VA common definitions

*/

#include "kymera_va_common.h"
#include <panic.h>

void Kymera_ConfigureChain(kymera_chain_handle_t chain, const operator_config_map_t *op_config_map, uint8 maps_length, const void *params)
{
    Operator operator;
    unsigned i;

    PanicFalse(chain != NULL);
    PanicFalse(op_config_map != NULL);
    PanicFalse(params != NULL);

    for(i = 0; i < maps_length; i++)
    {
        operator = ChainGetOperatorByRole(chain, op_config_map[i].operator_role);
        if (operator)
        {
            op_config_map[i].ConfigureOperator(operator, params);
        }
    }
}
