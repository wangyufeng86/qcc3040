/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file       hfp_profile_handover.c
\brief      HFP Profile Handover related interfaces

*/
#ifdef INCLUDE_MIRRORING

#include "domain_marshal_types.h"
#include "app_handover_if.h"
#include "hfp_profile.h"
#include "mirror_profile.h"
#include <logging.h>
#include <stdlib.h>
#include <panic.h>

/******************************************************************************
 * Local Function Prototypes
 ******************************************************************************/
static bool hfpProfile_Veto(void);

static bool hfpProfile_Marshal(const bdaddr *bd_addr, 
                               marshal_type_t type,
                               void **marshal_obj);

static app_unmarshal_status_t hfpProfile_Unmarshal(const bdaddr *bd_addr, 
                                 marshal_type_t type,
                                 void *unmarshal_obj);

static void hfpProfile_Commit(bool is_primary);

/******************************************************************************
 * Global Declarations
 ******************************************************************************/
const marshal_type_t hfp_profile_marshal_types[] = {
    MARSHAL_TYPE(hfpTaskData),
};

const marshal_type_list_t hfp_profile_marshal_types_list = {hfp_profile_marshal_types, ARRAY_DIM(hfp_profile_marshal_types)};
REGISTER_HANDOVER_INTERFACE(HFP_PROFILE, &hfp_profile_marshal_types_list, hfpProfile_Veto, hfpProfile_Marshal, hfpProfile_Unmarshal, hfpProfile_Commit);


/******************************************************************************
 * Local Function Definitions
 ******************************************************************************/
/*! 
    \brief Handle Veto check during handover
    \return TRUE to veto handover.
*/
static bool hfpProfile_Veto(void)
{
    bool veto = FALSE;

    /* Veto if any one of the following conditions is TRUE
       a) In transient state (lock is held). 
       b) Detach is pending.
       c) Pending messages for the task.
     */

    /* Handle any pending config write immediately */
    if (MessageCancelAll(appGetHfpTask(), HFP_INTERNAL_CONFIG_WRITE_REQ))
    {
        appHfpHandleInternalConfigWriteRequest();
    }

    if (MessagesPendingForTask(appGetHfpTask(), NULL))
    {
        DEBUG_LOG("hfpProfile_Veto, Messages pending for HFP task");
        veto = TRUE;
    }
    else 
    {
        if (appHfpGetLock())
        {
            DEBUG_LOG("hfpProfile_Veto, hfp_lock");
            veto = TRUE;
        }
        else if (!appHfpIsDisconnected() && appGetHfp()->bitfields.detach_pending)
        {
            /* We are not yet disconnected, but we have a "detach pending", i.e. ACL has been disconnected
             * and now we wait for profile disconnection event from Stack. Veto untill the profile is A2DP_STATE_DISCONNECTED.
             */
            DEBUG_LOG("hfpProfile_Veto, detach_pending");
            veto = TRUE;
        }
    }

    return veto;
}

/*!
    \brief The function shall set marshal_obj to the address of the object to 
           be marshalled.

    \param[in] bd_addr      Bluetooth address of the link to be marshalled.
    \param[in] type         Type of the data to be marshalled.
    \param[out] marshal_obj Holds address of data to be marshalled.
    \return TRUE: Required data has been copied to the marshal_obj.
            FALSE: No data is required to be marshalled. marshal_obj is set to NULL.

*/
static bool hfpProfile_Marshal(const bdaddr *bd_addr, 
                               marshal_type_t type, 
                               void **marshal_obj)
{
    bool status = FALSE;
    DEBUG_LOG("hfpProfile_Marshal");
    *marshal_obj = NULL;
    
    if (BdaddrIsSame(bd_addr, appHfpGetAgBdAddr()))
    {
        switch (type)
        {
            case MARSHAL_TYPE(hfpTaskData):
                *marshal_obj = appGetHfp();
                status = TRUE;
                break;
     
            default:
                break;
        }
    }
    else
    {
        DEBUG_LOG("hfpProfile_Marshal:Bluetooth Address Mismatch");
    }

    return status;
}

/*! 
    \brief The function shall copy the unmarshal_obj associated to specific 
            marshal type

    \param[in] bd_addr      Bluetooth address of the link to be unmarshalled.
    \param[in] type         Type of the unmarshalled data.
    \param[in] unmarshal_obj Address of the unmarshalled object.
    \return unmarshalling result. Based on this, caller decides whether to free
            the marshalling object or not.
*/
static app_unmarshal_status_t hfpProfile_Unmarshal(const bdaddr *bd_addr, 
                                 marshal_type_t type, 
                                 void *unmarshal_obj)
{
    DEBUG_LOG("hfpProfile_Unmarshal");
    app_unmarshal_status_t result = UNMARSHAL_FAILURE;

    switch (type)
    {
        case MARSHAL_TYPE(hfpTaskData):
            {
                hfpTaskData *hfpInst = (hfpTaskData*)unmarshal_obj;
                appGetHfp()->state = hfpInst->state;
                appGetHfp()->profile = hfpInst->profile;
                appGetHfp()->ag_bd_addr = *bd_addr;
                appGetHfp()->bitfields = hfpInst->bitfields;
                appGetHfp()->sco_supported_packets = hfpInst->sco_supported_packets;
                appGetHfp()->codec = hfpInst->codec;
                appGetHfp()->wesco = hfpInst->wesco;
                appGetHfp()->tesco = hfpInst->tesco;
                appGetHfp()->qce_codec_mode_id = hfpInst->qce_codec_mode_id;

                if (appGetHfp()->bitfields.hf_indicator_assigned_num == hf_battery_level)
                {
                    appBatteryUnregister(appGetHfpTask());
                    PanicFalse(appBatteryRegister(&appGetHfp()->battery_form));
                }                
                result = UNMARSHAL_SUCCESS_FREE_OBJECT;
            }
            break;

        default:
            /* Do nothing */
            break;
    }

    return result;
}

/*!
    \brief Component commits to the specified role

    The component should take any actions necessary to commit to the
    new role.

    \param[in] is_primary   TRUE if new role is primary, else secondary

*/
static void hfpProfile_Commit(bool is_primary)
{
    DEBUG_LOG("hfpProfile_Commit");
    
    if (is_primary)
    {
        Sink sink;

        DEBUG_LOG("hfpProfile_Commit:: New Role Primary");

        /* If the mirror profile has a valid eSCO sink, then always transfer
           it to the HFP profile. */
        appGetHfp()->sco_sink = MirrorProfile_GetScoSink();
        if (appGetHfp()->sco_sink)
        {
            DEBUG_LOG("hfpProfile_Commit:: Override SCO sink");
            /* Set the HFP Sink in the HFP profile library for the handset connection */
            PanicFalse(HfpOverideSinkBdaddr(&appGetHfp()->ag_bd_addr, appGetHfp()->sco_sink));
        }

        /* Derive slc_sink */
        if (HfpLinkGetSlcSink(hfp_primary_link, &sink))
        {
            appGetHfp()->slc_sink = sink;
        }
        else
        {
            DEBUG_LOG("hfpProfile_Commit:: Deriving slc_link failed");
        }
    }
    else
    {
        DEBUG_LOG("hfpProfile_Commit:: New Role Secondary");
        /* Silently move to disconnected state post commit in the new-secondary device */
        appGetHfp()->state = HFP_STATE_DISCONNECTED;
        appGetHfp()->slc_sink = 0;
        appGetHfp()->sco_sink = 0;

        appBatteryUnregister(appGetHfpTask());
        appGetHfp()->bitfields.hf_indicator_assigned_num = hf_indicators_invalid;        
    }
}

#endif /* INCLUDE_MIRRORING */
