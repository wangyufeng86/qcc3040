/*!
\copyright  Copyright (c) 2018 - 2019 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Implementation of internal LE advertising manager common utilities.
*/

#include "le_advertising_manager_utils.h"

/*! \brief Checks whether two sets of input parameters are matched
    \params params1, first set of parameters
            params2, second set of parameters
    
    \return TRUE, if parameters matched, FALSE if not matched.    
 */
bool LeAdvertisingManager_ParametersMatch(const le_adv_data_params_t * params1, const le_adv_data_params_t * params2)
{
    if(params1->data_set != params2->data_set)
        return FALSE;

    if(params1->placement != params2->placement)
        return FALSE;
    
    if(params1->completeness != params2->completeness)
        return FALSE;
    
    return TRUE;
}