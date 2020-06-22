/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file       av_handover.c
\brief      AV Handover related interfaces

*/

/* Only compile if AV defined */
#ifdef INCLUDE_AV
#ifdef INCLUDE_MIRRORING
#include "av.h"
#include "a2dp_profile.h"
#include "a2dp_profile_sync.h"
#include "avrcp_profile.h"
#include "domain_marshal_types.h"
#include "app_handover_if.h"
#include "adk_log.h"
#include <panic.h>
#include <stdlib.h>

/******************************************************************************
 * Local Function Prototypes
 ******************************************************************************/
static bool av_Veto(void);

static bool av_Marshal(const bdaddr *bd_addr, 
                       marshal_type_t type,
                       void **marshal_obj);

static app_unmarshal_status_t av_Unmarshal(const bdaddr *bd_addr, 
                         marshal_type_t type,
                         void *unmarshal_obj);

static void av_Commit(bool is_primary);

/******************************************************************************
 * Global Declarations
 ******************************************************************************/
const marshal_type_t av_marshal_types[] = {
    MARSHAL_TYPE(avTaskData),
    MARSHAL_TYPE(avInstanceTaskData),
    MARSHAL_TYPE(avTaskDataBitfields)
};

const marshal_type_list_t av_marshal_types_list = {av_marshal_types, sizeof(av_marshal_types)/sizeof(marshal_type_t)};

REGISTER_HANDOVER_INTERFACE(AV, &av_marshal_types_list, av_Veto, av_Marshal, av_Unmarshal, av_Commit);


/******************************************************************************
 * Local Function Definitions
 ******************************************************************************/

/*! 
    \brief Handle Veto check during handover
    \return TRUE to veto handover.
*/
static bool av_Veto(void)
{
    bool veto = FALSE;
    avInstanceTaskData *the_inst = NULL;
    avTaskData *the_av = AvGetTaskData();
    int instance;

    /* Handle any pending volume store requests immediately */
    if (MessageCancelFirst(appGetAvPlayerTask(), AV_INTERNAL_VOLUME_STORE_REQ))
    {
        appAvVolumeAttributeStore(the_av);
    }

    /* Check for pending messages of AV task */
    if (MessagesPendingForTask(appGetAvPlayerTask(), NULL))
    {
        DEBUG_LOG("av_Veto, Messages pending for AV task");
        veto = TRUE;
    }
    else
    {
        /*Check for all instances of AV */
        for (instance = 0; instance < AV_MAX_NUM_INSTANCES; instance++)
        {
            the_inst = the_av->av_inst[instance];

            if (the_inst)
            {
                if ((the_inst->a2dp.state != A2DP_STATE_DISCONNECTED) && the_inst->detach_pending)
                {
                    /* We are not yet disconnected, but we have a "detach pending", i.e. ACL has been disconnected
                     * and now we wait for profile disconnection event from Stack. Veto untill the state
                     * is A2DP_STATE_DISCONNECTED.
                     */
                    DEBUG_LOG("av_Veto, detach_pending");
                    veto =  TRUE;
                    break;
                }
                else if (A2dpProfile_Veto(the_inst))
                {
                    veto =  TRUE;
                    break;
                }
                else if (AvrcpProfile_Veto(the_inst))
                {
                    veto =  TRUE;
                    break;
                }
            }
        }
    }
       
    return veto;
}

/*!
    \brief The function shall set marshal_obj to the address of the object to 
           be marshalled.

    \param[in] bd_addr      Bluetooth address of the link to be marshalled
                            \ref bdaddr
    \param[in] type         Type of the data to be marshalled \ref marshal_type_t
    \param[out] marshal_obj Holds address of data to be marshalled.
    \return TRUE: Required data has been copied to the marshal_obj.
            FALSE: No data is required to be marshalled. marshal_obj is set to NULL.

*/
static bool av_Marshal(const bdaddr *bd_addr, 
                       marshal_type_t type, 
                       void **marshal_obj)
{
    bool status = FALSE;
    *marshal_obj = NULL;
    avTaskData *the_av = AvGetTaskData();

    UNUSED(bd_addr);

    switch (type)
    {
        case MARSHAL_TYPE(avTaskData):
            *marshal_obj = the_av;
            status = TRUE;
            break;

        case MARSHAL_TYPE(avInstanceTaskData):
            /* Get the AV instance of the handset */
            *marshal_obj = the_av->av_inst[0];
            status = (*marshal_obj != NULL);
            break;

        case MARSHAL_TYPE(avTaskDataBitfields):
        default:
            break;
    }

    return status;
}

/*! 
    \brief The function shall copy the unmarshal_obj associated to specific 
            marshal type \ref marshal_type_t

    \param[in] bd_addr      Bluetooth address of the link to be unmarshalled
                            \ref bdaddr
    \param[in] type         Type of the unmarshalled data \ref marshal_type_t
    \param[in] unmarshal_obj Address of the unmarshalled object.
    \return unmarshalling result. Based on this, caller decides whether to free
            the marshalling object or not.
*/
static app_unmarshal_status_t av_Unmarshal(const bdaddr *bd_addr, 
                         marshal_type_t type, 
                         void *unmarshal_obj)
{
    avTaskData *the_av = AvGetTaskData();
    app_unmarshal_status_t result = UNMARSHAL_FAILURE;

    UNUSED(bd_addr);
    
    switch (type)
    {
        case MARSHAL_TYPE(avTaskData):
            {
                avTaskData *recAv = unmarshal_obj;
                the_av->suspend_state = recAv->suspend_state;
                result = UNMARSHAL_SUCCESS_FREE_OBJECT;
            }
            break;

        case MARSHAL_TYPE(avInstanceTaskData):
            {
               avInstanceTaskData *av_inst = (avInstanceTaskData *)unmarshal_obj;

                /* index 0 is used as only one a2dp connection is expected in TWM */
                PanicFalse(the_av->av_inst[0] == NULL);
                the_av->av_inst[0] = av_inst;

                av_inst->av_task.handler = appAvInstanceHandleMessage;

                if(av_inst->avrcp.state == AVRCP_STATE_CONNECTED)
                {
                    /* The AV task is added automatically to the instance's AVRCP
                    task list, other components will need to register when committing
                    to the new role as primary */
                    av_inst->avrcp.client_list = TaskList_Create();
                    av_inst->avrcp.client_lock = 1;
                    TaskList_AddTask(av_inst->avrcp.client_list, &av_inst->av_task);
                }

                /* Initialize Sync parameters */
                appA2dpSyncInitialise(av_inst);
                result = UNMARSHAL_SUCCESS_DONT_FREE_OBJECT;
            }
            break;

        case MARSHAL_TYPE(avTaskDataBitfields):
        default:
            /* Do Nothing */
            break;
    }

    return result;
}

/*!
    \brief Component commits to the specified role

    The component should take any actions necessary to commit to the
    new role.

    \param[in] is_primary   TRUE if device role is primary, else secondary

*/
static void av_Commit(bool is_primary)
{
    avTaskData *the_inst = AvGetTaskData();
    avInstanceTaskData *av_task_inst = the_inst->av_inst[0];

    if (av_task_inst)
    {
        /*Perform commit operation for A2DP-Profile */
        A2dpProfile_Commit(av_task_inst, is_primary);

        /*Perform commit operation for AVRCP-Profile */
        AvrcpProfile_Commit(av_task_inst, is_primary);
    }

    if (!is_primary)
    {
        /* Clear suspend reason, so if secondary becomes primary (without
           handover) no invalid suspend reason is inherited when reconnecting
           to a handset */
        AvGetTaskData()->suspend_state = 0;
    }
}

#endif /* INCLUDE_MIRRORING */
#endif /* INCLUDE_AV */

