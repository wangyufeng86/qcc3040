/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file       handset_service_handover.c
\brief      Handset Service handover related interfaces

*/

#include <bdaddr.h>
#include <logging.h>
#include <panic.h>
#include <app_handover_if.h>
#include <device_properties.h>
#include <device_list.h>
#include "handset_service_sm.h"
#include "handset_service_protected.h"
/******************************************************************************
 * Local Function Prototypes
 ******************************************************************************/
static bool handsetService_Veto(void);
static void handsetService_Commit(bool is_primary);

/******************************************************************************
 * Global Declarations
 ******************************************************************************/
REGISTER_HANDOVER_INTERFACE_NO_MARSHALLING(HANDSET_SERVICE, handsetService_Veto, handsetService_Commit);

/******************************************************************************
 * Local Function Definitions
 ******************************************************************************/
/*! 
    \brief Handle Veto check during handover
           TRUE is returned if handset service is not in CONNECTED state.
    \return bool
*/
static bool handsetService_Veto(void)
{
    bool veto = TRUE;
    handset_service_state_machine_t *sm = HandsetService_GetSm();
    uint16 index;
    uint8 connected_handset_count = 0;

    for(index = 0; index < HANDSET_SERVICE_MAX_SM; index++)
    {
        if(sm && (sm->state == HANDSET_SERVICE_STATE_CONNECTED_BREDR))
        {
            connected_handset_count++;
        }

        sm++;
    }

    if(connected_handset_count == 1)
    {
        veto = FALSE;
    }
    else
    {
        DEBUG_LOG("handsetService_Veto, Number of handsets in connected state: %d", connected_handset_count);
    }

    return veto;
}

/*!
    \brief Component commits to the specified role

    The component should take any actions necessary to commit to the
    new role.

    \param[in] is_primary   TRUE if device role is primary, else secondary

*/
static void handsetService_Commit(bool is_primary)
{
    handset_service_state_machine_t *sm = HandsetService_GetSm();
    uint16 index;

    PanicNull(sm);

    if(is_primary)
    {
        /*Handover of only one device is supported. Get the MRU device and populate the first slot for 
          Handset statemachine.*/
        bool mru = TRUE;
        HandsetServiceSm_Init(sm);
        device_t mru_device = DeviceList_GetFirstDeviceWithPropertyValue(device_property_mru, &mru, sizeof(uint8));
        /* Panic if there is no device. This could happen if BT Device list is not restored on secondary. */
        PanicNull(mru_device);
        HandsetServiceSm_SetDevice(sm, mru_device);
        sm->state = HANDSET_SERVICE_STATE_CONNECTED_BREDR;
    }
    else
    {
        for (index = 0; index < HANDSET_SERVICE_MAX_SM; index++)
        {
            HandsetServiceSm_DeInit(sm);
            sm++;
        }
    }
}

