/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file       
\brief      Pairing plugin internal interface
*/

#ifndef PAIRING_PLUGIN_H_
#define PAIRING_PLUGIN_H_

#include "pairing.h"

/*! 
    \brief Initialise pairing_plugin 
 */
void Pairing_PluginInit(void);

/*! 
    \brief Call pairing plugin to handle remote IO capability if registered

    \param[in] ind The remote IO capability indication message
    
    \returns TRUE if pairing plugin handled the message, otherwise FALSE
 */
bool Pairing_PluginHandleRemoteIoCapability(const CL_SM_REMOTE_IO_CAPABILITY_IND_T *ind);

/*! \brief Call pairing plugin to handle IO capability request if registered

    \param[in] ind The IO capability request indication message
    \param[out] response Pointer which will be populated with the plugin's response 
    
    \returns TRUE if pairing plugin handled the message and populated the response, 
             otherwise FALSE
 */
bool Pairing_PluginHandleIoCapabilityRequest(const CL_SM_IO_CAPABILITY_REQ_IND_T *ind, pairing_io_capability_rsp_t* response);

/*! \brief Call pairing plugin to handle IO capability request if registered

    \param[in] ind The user confirmation request indication message
    \param[out] response Pointer which will be populated with the plugin's response 
    
    \returns TRUE if pairing plugin handled the message and populated the response, 
             otherwise FALSE
 */
bool Pairing_PluginHandleUserConfirmationRequest(const CL_SM_USER_CONFIRMATION_REQ_IND_T *ind, pairing_user_confirmation_rsp_t* response);

/*! 
    \brief Called when pairing completes to clear any state from pairing_plugin
    
    \note This does not notify the plugin directly
 */
void Pairing_PluginPairingComplete(void);

/*! 
    \brief Determine if any plugin is registered with the Pairing component
    
    \returns TRUE if a plugin is registered, FALSE if no plugin is registered
 */
bool Pairing_PluginIsRegistered(void);

#endif

