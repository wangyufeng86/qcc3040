/****************************************************************************
Copyright (c) 2016 - 2020 Qualcomm Technologies International, Ltd.


FILE NAME
    operators_data.c

DESCRIPTION
    Implementation of Kymera operator helpers.
*/

#include <panic.h>
#include <string.h>

#include "operators.h"

set_params_data_t* OperatorsCreateSetParamsData(unsigned number_of_params)
{
    unsigned memory_size = (unsigned) sizeof(set_params_data_t) + number_of_params * (unsigned) sizeof(standard_param_t);
    set_params_data_t* set_params_data = PanicUnlessMalloc(memory_size);
    memset(set_params_data, 0, memory_size);
    set_params_data->number_of_params = number_of_params;
    return set_params_data;
}

adaptive_anc_coefficients_t* OperatorsCreateAdaptiveAncCoefficientsData(uint16 num_coefficients)
{
    unsigned memory_size = (unsigned) sizeof(adaptive_anc_coefficients_t) + ((unsigned) sizeof(standard_param_value_t) * num_coefficients);
    adaptive_anc_coefficients_t* set_params_data = PanicUnlessMalloc(memory_size);
    memset(set_params_data, 0, memory_size);
    set_params_data->num_coefficients = num_coefficients;
    return set_params_data;
}


get_params_data_t* OperatorsCreateGetParamsData(unsigned number_of_params)
{
    unsigned memory_size = (unsigned) sizeof(get_params_data_t) + number_of_params * (unsigned) sizeof(standard_param_t);
    get_params_data_t* get_params_data = PanicUnlessMalloc(memory_size);
    memset(get_params_data, 0, memory_size);
    get_params_data->number_of_params = number_of_params;
    return get_params_data;
}

