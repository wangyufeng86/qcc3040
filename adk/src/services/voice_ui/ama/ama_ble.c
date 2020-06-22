/*!
\copyright  Copyright (c) 2020 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file       ama_ble.c
\brief      Implementation for AMA BLE
*/

#include <stream.h>
#include <panic.h>
#include <logging.h>
#include <connection_manager.h>
#include <connection.h>

#include "ama.h"
#include "le_advertising_manager.h"
#include "local_name.h"
#include "ama_ble.h"

#ifdef INCLUDE_AMA_LE
    #include "gatt_ama_server.h"
#endif

#define NUMBER_OF_ADVERT_DATA_ITEMS         (1)
#define AMA_SERVICE_ADV                     (0xFE03)
#define AMA_SERVICE_DATA_LENGTH             (13)
#define AMA_VENDOR_ID                       (0x000A)
#define AMA_PRODUCT_ID                      (0x0001)
#define AMA_ACCESSORY_COLOR                 (0x00)
#define AMA_DEVICE_STATE_DISCOVERABLE       (0x02)
#define AMA_DEVICE_STATE_NON_DISCOVERABLE   (0x00)
#define AMA_ACCESSORY_PREFERRED             (0x02)
#define AMA_LE_PREFERRED                    (0x00)
#define AMA_RFCOMM_PREFERRED                (0x01)
#define AMA_RESERVED                        (0x00)
#define AMA_DEVICE_STATE_OFFSET             (9)
#define AMA_PREFERRED_TRANSPORT_OFFSET      (10)


#ifdef INCLUDE_AMA_LE
    #define AMA_PREFERRED_TRANSPORT AMA_LE_PREFERRED
#else
    #define AMA_PREFERRED_TRANSPORT AMA_RFCOMM_PREFERRED
#endif

/* AMA_TODO: rationalize printf / hydra_log debug */

/********************************************************************
* Local functions prototypes:
*/
static unsigned int amaBle_NumberOfAdvItems(const le_adv_data_params_t * params);
static le_adv_data_item_t amaBle_GetAdvDataItems(const le_adv_data_params_t * params, unsigned int id);
static void amaBle_ReleaseAdvDataItems(const le_adv_data_params_t * params);
/*********************************************************************/
static le_adv_data_callback_t ama_le_advert_callback =
{
    .GetNumberOfItems   = amaBle_NumberOfAdvItems,
    .GetItem            = amaBle_GetAdvDataItems,
    .ReleaseItems       = amaBle_ReleaseAdvDataItems
};
/********************************************************************
* Advertising packet prototypes:
*/
static uint8 ama_service_adv_data[] = {
    (uint8)AMA_SERVICE_DATA_LENGTH,         /*Length for Service Data AD Type (23 bytes)*/
    (uint8)ble_ad_type_service_data,        /*Service Data AD Type Identifier*/
    (uint8)(AMA_SERVICE_ADV & 0xFF),        /*AMA Service ID*/
    (uint8)((AMA_SERVICE_ADV >> 8) & 0xFF),
    (uint8)(AMA_VENDOR_ID & 0xFF),          /*Vendor Id assigned by BT*/
    (uint8)((AMA_VENDOR_ID >> 8) & 0xFF),
    (uint8)(AMA_PRODUCT_ID & 0xFF),         /*Product Id for Alexa-enabled Headphones*/
    (uint8)((AMA_PRODUCT_ID >> 8) & 0xFF),
    (uint8)AMA_ACCESSORY_COLOR,             /*Color of the Accessory*/
    0x00,                                   /* Device State bit mask.  Bit 1: 1, if classic bluetooth is discoverable*/
    0x00,                                   /* Preferred Transport */
    (uint8)AMA_RESERVED,
    (uint8)AMA_RESERVED,
    (uint8)AMA_RESERVED
};

#define SIZE_AMA_SERVICE_DATA (sizeof(ama_service_adv_data)/sizeof(ama_service_adv_data[0]))

static le_adv_data_item_t ama_adv_data = {0};

/*********************************************************************/
void AmaBle_RegisterAdvertising(void)
{
    LeAdvertisingManager_Register(NULL, &ama_le_advert_callback);
    DEBUG_LOG("AMA LE ADV: Ama_BleRegisterAdvertising");
}
/********************************************************************
* Local functions:
*/
/*********************************************************************/
 static le_adv_data_item_t amaBle_GetAdvServiceItem(void)
{
    if(ama_adv_data.data == NULL)
    {
        ama_service_adv_data[AMA_DEVICE_STATE_OFFSET] = AMA_DEVICE_STATE_DISCOVERABLE;
        ama_service_adv_data[AMA_PREFERRED_TRANSPORT_OFFSET] = AMA_PREFERRED_TRANSPORT;
        ama_adv_data.size = SIZE_AMA_SERVICE_DATA;
        ama_adv_data.data = ama_service_adv_data;
        DEBUG_LOG("AMA LE ADV - Packet updated");
    }
    DEBUG_LOG("AMA LE ADV:- Packet sent");
    return ama_adv_data;
}

/*********************************************************************/
 static void amaBle_ReleaseAdvDataItems(const le_adv_data_params_t * params)
 {
     UNUSED(params);
     return;
 }

/*********************************************************************/
static bool amaBle_IsRequestValidForAmaDataSet(const le_adv_data_params_t * params)
{
    return ((params->data_set == le_adv_data_set_handset_identifiable) || (params->data_set == le_adv_data_set_handset_unidentifiable)) &&
            (params->completeness == le_adv_data_completeness_full) &&
            (params->placement == le_adv_data_placement_dont_care);
}

/*********************************************************************/
static unsigned int amaBle_NumberOfAdvItems(const le_adv_data_params_t * params)
{
    return amaBle_IsRequestValidForAmaDataSet(params) ? NUMBER_OF_ADVERT_DATA_ITEMS : 0;
}

/*********************************************************************/
static le_adv_data_item_t amaBle_GetAdvDataItems(const le_adv_data_params_t * params, unsigned int id)
{
    UNUSED(id);
    le_adv_data_item_t no_data = {.size = 0, .data = NULL};
    return amaBle_IsRequestValidForAmaDataSet(params) ? amaBle_GetAdvServiceItem() : no_data;
}

/*********************************************************************/
bool AmaBle_SendData(uint8* data, uint16 length)
{
#ifdef INCLUDE_AMA_LE
    return GattAmaServerSendNotification(data , length);  // Can also call GattAmaServerSendNotification()
#else
    UNUSED(data);
    UNUSED(length);
    return FALSE;
#endif
}


