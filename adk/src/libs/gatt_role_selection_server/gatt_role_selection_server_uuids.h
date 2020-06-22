/*******************************************************************************
Copyright (c) 2019 Qualcomm Technologies International, Ltd.
 
*******************************************************************************/
/*! \file
    Definitions of UUIDs for the Gatt Role Selection service.
 */

#ifndef __GATT_ROLE_SELECTION_UUIDS_H__
#define __GATT_ROLE_SELECTION_UUIDS_H__

#define UUID128_ROLE_SEL_FORMAT_GATT_DB_(a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p) 0x##a##b##c##d##e##f##g##h##i##j##k##l##m##n##o##p
#define UUID128_ROLE_SEL_FORMAT_GATT_DB(uuid) UUID128_ROLE_SEL_FORMAT_GATT_DB_(uuid)

#define UUID16_ROLE_SELECTION_SERVICE       0xFD92
#define UUID128_ROLE_SEL_MIRRORING_STATE    00,00,eb,20,d1,02,11,e1,9b,23,00,02,5b,00,a5,a5
#define UUID128_ROLE_SEL_CONTROL_POINT      00,00,eb,21,d1,02,11,e1,9b,23,00,02,5b,00,a5,a5
#define UUID128_ROLE_SEL_FIGURE_OF_MERIT    00,00,eb,22,d1,02,11,e1,9b,23,00,02,5b,00,a5,a5

#define UUID_ROLE_SELECTION_SERVICE         UUID16_ROLE_SELECTION_SERVICE
#define UUID_ROLE_SEL_MIRRORING_STATE       UUID128_ROLE_SEL_FORMAT_GATT_DB(UUID128_ROLE_SEL_MIRRORING_STATE)
#define UUID_ROLE_SEL_CONTROL_POINT         UUID128_ROLE_SEL_FORMAT_GATT_DB(UUID128_ROLE_SEL_CONTROL_POINT)
#define UUID_ROLE_SEL_FIGURE_OF_MERIT       UUID128_ROLE_SEL_FORMAT_GATT_DB(UUID128_ROLE_SEL_FIGURE_OF_MERIT)

#endif /* __GATT_ROLE_SELECTION_UUIDS_H__ */
