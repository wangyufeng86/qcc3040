/*!
\copyright  Copyright (c) 2008 - 2019 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file       marshal_common.c
\brief      Common types used by all marshallers
*/

#include "marshal_common.h"
#include <marshal.h>
#include <panic.h>
#include <sink.h>

/*!< The sink of the L2CAP link */
static Sink link_sink;

/*! \brief Set the L2CAP link sink into marshal common module for
    timestamp conversions(from local time to BT wall clock and vice versa).
    Peer signalling module set and reset the L2CAP sink.

    \sink  The sink of the L2CAP link
*/
void MarshalCommon_SetSink(Sink sink)
{
    link_sink = sink;
}

/*! \brief Get the L2CAP link sink,this sink is provided by
    peer signalling module.
*/
static Sink marshalCommon_GetSink(void)
{
    return link_sink;
}

/*! \brief Marshal, converting local time to BT wall clock time.

    \param dest desination for conversion result
    \param src source of local timestamp
    \param n size of the timestamp

    \return dest.
*/
static void *marshalCommon_MarshalRtimeMarshal(void *dest, const void *src, size_t n)
{
    marshal_rtime_t local_time, wallclock_time = 0;
    wallclock_state_t state;

    PanicFalse(n == sizeof(marshal_rtime_t));

    memcpy(&local_time, src, n);

    /* the link may have disconnected, but upper layer code calling in here may
     * not yet be aware, so the sink may no longer be valid. Only attempt the
     * conversion if the sink is still valid and the attempt to get the wallclock
     * state succeeded. */
    if (RtimeWallClockGetStateForSink(&state, marshalCommon_GetSink()))
    {
        RtimeLocalToWallClock(&state, local_time, &wallclock_time);
    }

    return memcpy(dest, &wallclock_time, n);
}

/*! \brief Unmarshal, converting BT wall clock time to local time.

    \param dest desination for conversion result
    \param src source of wallclock timestamp
    \param n size of the timestamp

    \return dest.
*/
static void *marshalCommon_MarshalRtimeUnmarshal(void *dest, const void *src, size_t n)
{
    marshal_rtime_t local_time, wallclock_time;
    wallclock_state_t state;

    PanicFalse(n == sizeof(marshal_rtime_t));

    memcpy(&wallclock_time, src, n);

    PanicFalse(RtimeWallClockGetStateForSink(&state, marshalCommon_GetSink()));
    PanicFalse(RtimeWallClockToLocal(&state, wallclock_time, &local_time));

    return memcpy(dest, &local_time, n);
}

/*! \brief Marshal type descriptor for uint8 common type. */
const marshal_type_descriptor_t marshal_type_descriptor_uint8 =
    MAKE_MARSHAL_TYPE_DEFINITION_BASIC(uint8);

/*! \brief Marshal type descriptor for uint16 common type. */
const marshal_type_descriptor_t marshal_type_descriptor_uint16 =
    MAKE_MARSHAL_TYPE_DEFINITION_BASIC(uint16);

/*! \brief Marshal type descriptor for uint32 common type. */
const marshal_type_descriptor_t marshal_type_descriptor_uint32 =
    MAKE_MARSHAL_TYPE_DEFINITION_BASIC(uint32);

/*! \brief Marshal type descriptor for uint64 common type. */
const marshal_type_descriptor_t marshal_type_descriptor_uint64 =
    MAKE_MARSHAL_TYPE_DEFINITION_BASIC(unsigned long long);

/*! \brief Marshal type descriptor for int8 common type. */
const marshal_type_descriptor_t marshal_type_descriptor_int8 =
    MAKE_MARSHAL_TYPE_DEFINITION_BASIC(int8);

/*! \brief Marshal type descriptor for int16 common type. */
const marshal_type_descriptor_t marshal_type_descriptor_int16 =
    MAKE_MARSHAL_TYPE_DEFINITION_BASIC(int16);

/*! \brief Marshal type descriptor for int32 common type. */
const marshal_type_descriptor_t marshal_type_descriptor_int32 =
    MAKE_MARSHAL_TYPE_DEFINITION_BASIC(int32);

/*! \brief Marshal type descriptor for bdaddr type. */
const marshal_member_descriptor_t bdaddr_member_descriptors[] =
{
    MAKE_MARSHAL_MEMBER(bdaddr, uint32, lap),
    MAKE_MARSHAL_MEMBER(bdaddr, uint8, uap),
    MAKE_MARSHAL_MEMBER(bdaddr, uint16, nap),
};
const marshal_type_descriptor_t marshal_type_descriptor_bdaddr =
    MAKE_MARSHAL_TYPE_DEFINITION(bdaddr, bdaddr_member_descriptors);

const marshal_member_descriptor_t typed_bdaddr_member_descriptors[] =
{
    MAKE_MARSHAL_MEMBER(typed_bdaddr, uint8,  type),
    MAKE_MARSHAL_MEMBER(typed_bdaddr, bdaddr, addr),
};

const marshal_type_descriptor_t marshal_type_descriptor_typed_bdaddr =
    MAKE_MARSHAL_TYPE_DEFINITION(bdaddr, typed_bdaddr_member_descriptors);

const marshal_member_descriptor_t tp_bdaddr_member_descriptors[] =
{
    MAKE_MARSHAL_MEMBER(tp_bdaddr, typed_bdaddr, taddr),
    MAKE_MARSHAL_MEMBER(tp_bdaddr, TRANSPORT_T, transport),
};

const marshal_type_descriptor_t marshal_type_descriptor_tp_bdaddr =
    MAKE_MARSHAL_TYPE_DEFINITION(bdaddr, tp_bdaddr_member_descriptors);

const marshal_custom_copy_cbs marshal_rtime_callbacks = { marshalCommon_MarshalRtimeMarshal ,
                                                         marshalCommon_MarshalRtimeUnmarshal };

/*! marshal_rtime_t are marshalled/unmarshalled with implicit clock conversion
    by the marshalling library, using the marshal_rtime_callbacks callbacks. */
const marshal_type_descriptor_t marshal_type_descriptor_marshal_rtime_t =
    { .u.custom_copy_cbs = &marshal_rtime_callbacks,
      .size = sizeof(marshal_rtime_t),
      .members_len = 0,
      .is_union = 0,
      .dynamic_length = 0,
      .dynamic_type = 0 };

#ifndef HOSTED_TEST_ENVIRONMENT
COMPILE_TIME_ASSERT(sizeof(TRANSPORT_T) == sizeof(uint8), transport_t_is_not_the_same_size_as_uint8);
#endif
