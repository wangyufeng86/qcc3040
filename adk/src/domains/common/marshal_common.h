/*!
\copyright  Copyright (c) 2008 - 2019 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file       marshal_common.h
\brief      Common types used by all marshallers
*/
#ifndef MARSHAL_COMMON_H
#define MARSHAL_COMMON_H

#include <hydra_macros.h>
#include <marshal.h>
#include <bdaddr.h>
#include <rtime.h>

/*! A special microsecond resolution timestamp type.

    When this type is marshalled, the timestamp is automatically converted
    from local clock domain to BT clock domain on marshalling, and from BT clock
    domain to local clock domain on unmarshalling. The client code therefore can
    transmit local clock timestamps to the peer and the timestamp received by the
    peer will represent same time on the peer device's local clock.
*/
typedef rtime_t marshal_rtime_t;

/*! Maximum number of common types permitted. */
#define MARSHAL_COMMON_TYPES_MAX    11

/*! List of common types for x-macro expansion. */
#define MARSHAL_COMMON_TYPES_TABLE(ENTRY) \
    ENTRY(uint8) \
    ENTRY(uint16) \
    ENTRY(uint32) \
    ENTRY(uint64) \
    ENTRY(int8) \
    ENTRY(int16) \
    ENTRY(int32) \
    ENTRY(bdaddr) \
    ENTRY(tp_bdaddr) \
    ENTRY(typed_bdaddr) \
    ENTRY(marshal_rtime_t)

/*! Expand the list of common types into an enumeration of marshalling types. */
#define EXPAND_AS_ENUMERATION(type) MARSHAL_TYPE(type),
enum MARSHAL_COMMON_TYPES
{
    MARSHAL_COMMON_TYPES_TABLE(EXPAND_AS_ENUMERATION)
    NUMBER_OF_COMMON_MARSHAL_OBJECT_TYPES
};
#undef EXPAND_AS_ENUMERATION

/* Ensure that no additional entries are added to the common types. */
COMPILE_TIME_ASSERT(NUMBER_OF_COMMON_MARSHAL_OBJECT_TYPES == MARSHAL_COMMON_TYPES_MAX, marshal_common_types_max_exceeded);

/* TRANSPORT_T is represented as uint8 */
#define MARSHAL_TYPE_TRANSPORT_T MARSHAL_TYPE_uint8

/*! \brief Marshal type descriptor for uint8 common type. */
extern const marshal_type_descriptor_t marshal_type_descriptor_uint8;

/*! \brief Marshal type descriptor for uint16 common type. */
extern const marshal_type_descriptor_t marshal_type_descriptor_uint16;

/*! \brief Marshal type descriptor for uint32 common type. */
extern const marshal_type_descriptor_t marshal_type_descriptor_uint32;

/*! \brief Marshal type descriptor for uint64 common type. */
extern const marshal_type_descriptor_t marshal_type_descriptor_uint64;

/*! \brief Marshal type descriptor for int8 common type. */
extern const marshal_type_descriptor_t marshal_type_descriptor_int8;

/*! \brief Marshal type descriptor for int32 common type. */
extern const marshal_type_descriptor_t marshal_type_descriptor_int16;

/*! \brief Marshal type descriptor for int32 common type. */
extern const marshal_type_descriptor_t marshal_type_descriptor_int32;

/*! \brief Marshal type descriptor for bdaddr type. */
extern const marshal_type_descriptor_t marshal_type_descriptor_bdaddr;

/*! \brief Marshal type descriptor for tp_bdaddr type. */
extern const marshal_type_descriptor_t marshal_type_descriptor_tp_bdaddr;

/*! \brief Marshal type descriptor for typed_bdaddr type. */
extern const marshal_type_descriptor_t marshal_type_descriptor_typed_bdaddr;

/*! Marshal type descriptor for peer_sig_rtime_t. */
extern const marshal_type_descriptor_t marshal_type_descriptor_marshal_rtime_t;

/*! \brief Set the L2CAP link sink into marshal common module for timestamp 
conversions(from local time to BT wall clock and vice versa). */
void MarshalCommon_SetSink(Sink sink);

#endif /* MARSHAL_COMMON_H */
