/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file       earbud_sm_handover.c
\brief      Earbud application SM handover support.

*/

#ifdef INCLUDE_MIRRORING

#include "earbud_sm.h"
#include "earbud_sm_private.h"
#include "earbud_sm_handover.h"
#include "earbud_handover_typedef.h"
#include "earbud_handover_marshal_typedef.h"

#include <app_handover_if.h>

#include <logging.h>
#include <system_clock.h>
#include <rtime.h>

#include <marshal.h>

/******************************************************************************
 * Prototypes for the marshal inteface
 ******************************************************************************/
static bool appSm_HandoverVeto(void);
static bool appSm_HandoverMarshal(const bdaddr *bd_addr, 
                              marshal_type_t type,
                              void **marshal_obj);
static app_unmarshal_status_t appSm_HandoverUnmarshal(const bdaddr *bd_addr, 
                                                  marshal_type_t type,
                                                  void *unmarshal_obj);
static void appSm_HandoverCommit(bool is_primary);

/******************************************************************************
 * Local utility functions
 ******************************************************************************/
/*! \brief Reset the timers struct. */
static void appSm_HandoverTimersReset(void)
{
    memset(&SmGetTaskData()->timers, 0, sizeof(earbud_sm_timers));
}

/*! \brief Calculate absolute local time (in us) from now for remaining time on a timer. */
static marshal_rtime_t appSm_HandoverTimerAbsoluteTime(int32 due_ms)
{
    rtime_t local_clock = SystemClockGetTimerTime();

    /* if timer is already past due, just use current local time,
     * by the time this is marshalled to the peer it will be in
     * the past and will fire immediately */
    if (due_ms < 0)
    {
        return (marshal_rtime_t)local_clock;
    }
    else
    {
        return (marshal_rtime_t)rtime_add(local_clock, (due_ms * US_PER_MS)); 
    }
}

/*! \brief Return remaining time in milliseconds before an absolute time. */
static uint32 appSm_HandoverTimerOffset(marshal_rtime_t abs_time)
{
    rtime_t local_clock = SystemClockGetTimerTime();
    int32 timer_offset = rtime_sub(abs_time, local_clock);

    DEBUG_LOG_VERBOSE("appSm_HandoverTimerOffset timer_offset %d", timer_offset);

    return rtime_gt(timer_offset, 0) ? timer_offset/US_PER_MS : D_IMMEDIATE;
}

/******************************************************************************
 * Registration of Earbud application marshal types with application handover
 ******************************************************************************/
/*! Types that will be marshalled. */
const marshal_type_t earbud_sm_marshal_types[] = {
    MARSHAL_TYPE(earbud_sm_timers),
};

/*! Application handover interface types configuration. */ 
const marshal_type_list_t earbud_sm_marshal_types_list = {earbud_sm_marshal_types, sizeof(earbud_sm_marshal_types)/sizeof(marshal_type_t)};

/*! Register with application handover the types and function pointers to use during
    marshalling. */
REGISTER_HANDOVER_INTERFACE(EARBUD_SM, &earbud_sm_marshal_types_list, appSm_HandoverVeto, appSm_HandoverMarshal, appSm_HandoverUnmarshal, appSm_HandoverCommit);

/******************************************************************************
 * Marshal Interface
 ******************************************************************************/
/*! 
    \brief Handle Veto check during handover
    \return bool
*/
static bool appSm_HandoverVeto(void)
{
    /* don't veto */
    return FALSE;
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
static bool appSm_HandoverMarshal(const bdaddr *bd_addr, 
                              marshal_type_t type,
                              void **marshal_obj)
{
    bool status = FALSE;

    UNUSED(bd_addr);

    DEBUG_LOG_VERBOSE("appSm_HandoverMarshal");

    appSm_HandoverTimersReset();

    switch (type)
    {
        case MARSHAL_TYPE(earbud_sm_timers):
        {
            int32 first_due = 0;
            if (MessagePendingFirst(SmGetTask(), SM_INTERNAL_TIMEOUT_OUT_OF_EAR_A2DP, &first_due))
            {
                SmGetTaskData()->timers.out_of_ear_a2dp_pause = appSm_HandoverTimerAbsoluteTime(first_due);
                SmGetTaskData()->timers.out_of_ear_a2dp_pause_valid = 1;
                DEBUG_LOG_VERBOSE("appSm_HandoverMarshal SM_INTERNAL_TIMEOUT_OUT_OF_EAR_A2DP due ms %d", first_due);
                status = TRUE;
            }
            if (MessagePendingFirst(SmGetTask(), SM_INTERNAL_TIMEOUT_OUT_OF_EAR_SCO, &first_due))
            {
                SmGetTaskData()->timers.out_of_ear_sco_transfer = appSm_HandoverTimerAbsoluteTime(first_due);
                SmGetTaskData()->timers.out_of_ear_sco_transfer_valid = 1;
                DEBUG_LOG_VERBOSE("appSm_HandoverMarshal SM_INTERNAL_TIMEOUT_OUT_OF_EAR_SCO due %d", first_due);
                status = TRUE;
            }
            if (MessagePendingFirst(SmGetTask(), SM_INTERNAL_TIMEOUT_IN_EAR_A2DP_START, &first_due))
            {
                SmGetTaskData()->timers.in_ear_a2dp_play = appSm_HandoverTimerAbsoluteTime(first_due);
                SmGetTaskData()->timers.in_ear_a2dp_play_valid = 1;
                DEBUG_LOG_VERBOSE("appSm_HandoverMarshal SM_INTERNAL_TIMEOUT_IN_EAR_A2DP_START due %d", first_due);
                status = TRUE;
            }

            if (status)
            {
                *marshal_obj = &SmGetTaskData()->timers;
            }
        }
        break;

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
static app_unmarshal_status_t appSm_HandoverUnmarshal(const bdaddr *bd_addr, 
                                                  marshal_type_t type,
                                                  void *unmarshal_obj)
{
    app_unmarshal_status_t result = UNMARSHAL_FAILURE;

    UNUSED(bd_addr);

    DEBUG_LOG_VERBOSE("appSm_HandoverUnmarshal");

    switch (type)
    {
        case MARSHAL_TYPE(earbud_sm_timers):
            SmGetTaskData()->timers = *(earbud_sm_timers*)unmarshal_obj;
            result = UNMARSHAL_SUCCESS_FREE_OBJECT;
            break;
        default:
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
static void appSm_HandoverCommit(bool is_primary)
{
    UNUSED(is_primary);

    DEBUG_LOG_VERBOSE("appSm_HandoverCommit");

    /*! Committed to the handover now:
        - On secondary cancel any active timers
        - On primary, start any timers
    */
    if (!is_primary)
    {
        MessageCancelFirst(SmGetTask(), SM_INTERNAL_TIMEOUT_OUT_OF_EAR_A2DP);
        MessageCancelFirst(SmGetTask(), SM_INTERNAL_TIMEOUT_OUT_OF_EAR_SCO);
        MessageCancelFirst(SmGetTask(), SM_INTERNAL_TIMEOUT_IN_EAR_A2DP_START);
    }
    else
    {
        if (SmGetTaskData()->timers.out_of_ear_a2dp_pause_valid)
        {
            uint32 offset = appSm_HandoverTimerOffset(SmGetTaskData()->timers.out_of_ear_a2dp_pause);
            DEBUG_LOG_VERBOSE("appSm_HandoverCommit SM_INTERNAL_TIMEOUT_OUT_OF_EAR_A2DP %d", offset);
            MessageSendLater(SmGetTask(), SM_INTERNAL_TIMEOUT_OUT_OF_EAR_A2DP, NULL, offset);
        }
        if (SmGetTaskData()->timers.out_of_ear_sco_transfer_valid)
        {
            uint32 offset = appSm_HandoverTimerOffset(SmGetTaskData()->timers.out_of_ear_sco_transfer);
            DEBUG_LOG_VERBOSE("appSm_HandoverCommit SM_INTERNAL_TIMEOUT_OUT_OF_EAR_SCO %d", offset);
            MessageSendLater(SmGetTask(), SM_INTERNAL_TIMEOUT_OUT_OF_EAR_SCO, NULL, offset);
        }
        if (SmGetTaskData()->timers.in_ear_a2dp_play_valid)
        {
            uint32 offset = appSm_HandoverTimerOffset(SmGetTaskData()->timers.in_ear_a2dp_play);
            DEBUG_LOG_VERBOSE("appSm_HandoverCommit SM_INTERNAL_TIMEOUT_IN_EAR_A2DP_START %d", offset);
            MessageSendLater(SmGetTask(), SM_INTERNAL_TIMEOUT_IN_EAR_A2DP_START, NULL, offset);
        }
    }
}


#endif /* INCLUDE_MIRRORING */
