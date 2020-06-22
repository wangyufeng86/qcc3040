/*******************************************************************************
Copyright (c) 2019 Qualcomm Technologies International, Ltd.
 
*******************************************************************************/
/*! \file
    Definitions of UUIDs for the Gatt Root Key Transfer service.
 */

#ifndef __GATT_ROOT_KEY_SERVER_UUIDS_H__
#define __GATT_ROOT_KEY_SERVER_UUIDS_H__

#define UUID128_ROOT_KEY_FORMAT_GATT_DB_(a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p) 0x##a##b##c##d##e##f##g##h##i##j##k##l##m##n##o##p
#define UUID128_ROOT_KEY_FORMAT_GATT_DB(uuid) UUID128_ROOT_KEY_FORMAT_GATT_DB_(uuid)

#define UUID128_ROOT_KEY_SERVICE            00,00,eb,10,d1,02,11,e1,9b,23,00,02,5b,00,a5,a5
#define UUID128_ROOT_KEY_FEATURES           00,00,eb,11,d1,02,11,e1,9b,23,00,02,5b,00,a5,a5
#define UUID128_ROOT_KEY_STATUS             00,00,eb,12,d1,02,11,e1,9b,23,00,02,5b,00,a5,a5
#define UUID128_ROOT_KEY_CHALLENGE_CONTROL  00,00,eb,13,d1,02,11,e1,9b,23,00,02,5b,00,a5,a5
#define UUID128_ROOT_KEY_KEYS_CONTROL       00,00,eb,14,d1,02,11,e1,9b,23,00,02,5b,00,a5,a5

#define UUID_ROOT_KEY_SERVICE               UUID128_ROOT_KEY_FORMAT_GATT_DB(UUID128_ROOT_KEY_SERVICE)
#define UUID_ROOT_KEY_FEATURES              UUID128_ROOT_KEY_FORMAT_GATT_DB(UUID128_ROOT_KEY_FEATURES)
#define UUID_ROOT_KEY_STATUS                UUID128_ROOT_KEY_FORMAT_GATT_DB(UUID128_ROOT_KEY_STATUS)
#define UUID_ROOT_KEY_CHALLENGE_CONTROL     UUID128_ROOT_KEY_FORMAT_GATT_DB(UUID128_ROOT_KEY_CHALLENGE_CONTROL)
#define UUID_ROOT_KEY_KEYS_CONTROL          UUID128_ROOT_KEY_FORMAT_GATT_DB(UUID128_ROOT_KEY_KEYS_CONTROL)

#define UUID128_ROOT_KEY_SERVICE_LEFT            00,00,eb,15,d1,02,11,e1,9b,23,00,02,5b,00,a5,a5
#define UUID128_ROOT_KEY_SERVICE_RIGHT           00,00,eb,16,d1,02,11,e1,9b,23,00,02,5b,00,a5,a5
#define UUID_ROOT_KEY_SERVICE_LEFT               UUID128_ROOT_KEY_FORMAT_GATT_DB(UUID128_ROOT_KEY_SERVICE_LEFT)
#define UUID_ROOT_KEY_SERVICE_RIGHT              UUID128_ROOT_KEY_FORMAT_GATT_DB(UUID128_ROOT_KEY_SERVICE_RIGHT)

#endif /* __GATT_ROOT_KEY_SERVER_UUIDS_H__ */
