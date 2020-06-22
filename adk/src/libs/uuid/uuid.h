/* Copyright (c) 2015 - 2019 Qualcomm Technologies International, Ltd. */
/*  */
/*!
  \file uuid.h
  \brief Interface to UUID library.
*/

#ifndef __UUID_H
#define __UUID_H

#include <csrtypes.h>

/*! \brief Enumeration of type of UUID.
 */
typedef enum uuid_type
{
    UUID_16 = 2,
    UUID_32 = 4,
    UUID_128 = 16
} uuid_type_t;

/*! \brief Definition of a UUID.
 */
typedef struct uuid
{
    uuid_type_t type;
    uint16 uuid[8];
} uuid_t;

/*! \brief Definition of the 96-bit base of a 128-bit UUID.
 */
typedef struct uuid_base
{
    uint16 base[6];
} uuid_base_t;


#define UUID_128_FORMAT_gatt_uuid_t_(a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p) 0x##a##b##c##d##u, 0x##e##f##g##h##u, 0x##i##j##k##l##u, 0x##m##n##o##p##u
#define UUID_128_FORMAT_uint8_(a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p) 0x##p, 0x##o, 0x##n, 0x##m, 0x##l, 0x##k, 0x##j, 0x##i, 0x##h, 0x##g, 0x##f, 0x##e, 0x##d, 0x##c, 0x##b, 0x##a

/*! \def UUID_128_FORMAT_gatt_uuid_t(uuid)
    \brief A macro that converts a 128-bit GATT UUID into the gatt library format.
   
    The gatt library stores the GATT UUID in a gatt_uuid_t array of 4 elements that are Big Endian. 
    This macro handles the conversion between a 128-bit UUID of the format a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p
    into the gatt library format.
    
    Example:
    #define UUID128_EXAMPLE_UUID 00,11,22,33,44,55,66,77,88,99,88,77,66,55,44,33
    gatt_uuid_t uuid[] = {UUID_128_FORMAT_gatt_uuid_t(UUID128_EXAMPLE_UUID)};
*/
#define UUID_128_FORMAT_gatt_uuid_t(uuid) UUID_128_FORMAT_gatt_uuid_t_(uuid)
/*! \def UUID_128_FORMAT_uint8(uuid)
    \brief A macro that converts a 128-bit GATT UUID into 8-bit elements that are Little Endian.
   
    Example:
    #define UUID128_EXAMPLE_UUID 00,11,22,33,44,55,66,77,88,99,88,77,66,55,44,33
    uint8 uuid[] = {UUID_128_FORMAT_uint8(UUID128_EXAMPLE_UUID)};
*/
#define UUID_128_FORMAT_uint8(uuid) UUID_128_FORMAT_uint8_(uuid)

/*
    To convert a 128-bit UUID of the format a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p
    into a value that can be used in a GATT Database .dbi file, a local macro can defined
    as follows. It wasn't included in this module because of complications with the
    build system:

    #define UUID128_FORMAT_GATT_DB_(a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p) 0x##a##b##c##d##e##f##g##h##i##j##k##l##m##n##o##p
    #define UUID128_FORMAT_GATT_DB(uuid) UUID128_FORMAT_GATT_DB_(uuid)
*/

            
/*! \brief Initialise a 16-bit UUID.
 */
void Uuid16Init(uuid_t *uuid, uint16 uuid_data);

/*! \brief Initialise a 32-bit UUID.
 */
void Uuid32Init(uuid_t *uuid, uint32 uuid_data);

/*! \brief Initialise a 128-bit UUID.
 */
void Uuid128Init(uuid_t *uuid, uint32 uuid_data[4]);

/*! \brief Initialise a 128-bit UUID with a base and most significant 32-bits.
 */
void Uuid128InitWithBase(uuid_t *uuid, const uuid_base_t* uuid_base, uint32 first32);

/*! \brief Get the size of a UUID.
 */
int UuidSize(const uuid_t *uuid);

/*! \brief Get the type of a UUID.
 */
uuid_type_t UuidType(const uuid_t *uuid);

/*! \brief Compare a UUID with a base.
 */
bool UuidHasBase(const uuid_t *uuid, const uuid_base_t *uuid_base);

/*! \brief Compare two 16-bit UUIDs.
 */
bool Uuid16IsSame(const uuid_t *uuid_a, const uuid_t *uuid_b);

/*! \brief Compare any two UUIDs.
 */
bool UuidIsSame(const uuid_t *uuid_a, const uuid_t *uuid_b);

/*! \brief Copy a UUID.
 */
void UuidCopy(uuid_t *uuid_a, const uuid_t *uuid_b);

/*! \brief Return the most significant 32-bits of a 128-bit UUID.
 */
bool Uuid128HasBaseGet32(const uuid_t* uuid, uint32* out, const uuid_base_t* uuid_base);

#endif

