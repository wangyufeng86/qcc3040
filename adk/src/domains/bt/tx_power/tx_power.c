/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file
\brief      Management of Tx Power */

#include "tx_power.h"
#include "tx_power_msg_handler.h"

#include <connection.h>
#include <panic.h>
#include <string.h>
#include <stdio.h>
#include <logging.h>

#define TX_POWER_NUM_ITEMS   (1)

/*!< Tx Power data states */
typedef enum
{
    tx_power_initialising,
    tx_power_ready
} tx_power_state_t;

/*!< Tx Power data readyness */
typedef struct
{
    tx_power_state_t tx_power_state_le;
    tx_power_state_t tx_power_state_br_edr;
}tx_power_data_state_t;

/*!< Task information for the Tx Power */
static tx_power_data_t tx_power_data;
static tx_power_data_state_t data_state;
static TaskData tx_power_task_data = {TxPowerTaskHandler};

static unsigned int txPower_AdvGetNumberOfItems(const le_adv_data_params_t * params);
static le_adv_data_item_t txPower_AdvertData(const le_adv_data_params_t * params, unsigned int);
static void txPower_ReleaseItems(const le_adv_data_params_t *params);
le_adv_data_callback_t txpower_AdvCallback ={.GetNumberOfItems = &txPower_AdvGetNumberOfItems,
                                             .GetItem = &txPower_AdvertData,
                                             .ReleaseItems = &txPower_ReleaseItems};

void TxPower_SetTxPowerPathLoss(uint8 path_loss)
{
    tx_power_data.tx_power_path_loss = path_loss;
}

uint8 TxPower_GetTxPowerPathLoss(void)
{
    return tx_power_data.tx_power_path_loss;
}

bool TxPower_Init(Task init_task)
{
    UNUSED(init_task);

    DEBUG_LOG("TxPower_Init");
    data_state.tx_power_state_br_edr = tx_power_initialising;
    data_state.tx_power_state_le = tx_power_initialising;
    tx_power_data.tx_power_mandatory = 0;
    /* Request from Bluestack to get the BLE advertisement's Tx power */
    ConnectionDmBleReadAdvertisingChannelTxPower((Task)&tx_power_task_data);
    /* Register callback with advertising manager */
    tx_power_data.adv_mgr_handle = LeAdvertisingManager_Register(&tx_power_task_data, &txpower_AdvCallback);

    return (tx_power_data.adv_mgr_handle ? TRUE : FALSE);
}


void TxPower_Retrigger_LE_Data(void)
{
    DEBUG_LOG("TxPower_Retrigger_LE_Data");
    ConnectionDmBleReadAdvertisingChannelTxPower((Task)&tx_power_task_data);
}


void TxPower_Mandatory(bool enable_mandatory, tx_power_client_t client_id)
{
    DEBUG_LOG("TxPower_Mandatory: enable_mandatory = %d, client_id = %d", enable_mandatory, client_id);
    DEBUG_LOG("TxPower_Mandatory: Present Mandatory flag is set to %s", tx_power_data.mandatory? "TRUE":"FALSE");

    if(client_id >= le_client_last)
    {
        DEBUG_LOG("Invalid client id");
        Panic();
    }
    if(enable_mandatory)
    {
        tx_power_data.tx_power_mandatory |= (1U << client_id);
        DEBUG_LOG("Tx Power Mandatory set for client_id=%d", client_id);
    }
    else
    {
        tx_power_data.tx_power_mandatory &= ~(1U << client_id);
        DEBUG_LOG("Tx Power not Mandatory for client_id=%d", client_id);
    }
    /* Tx Power is mandatory even if one client has it set it */
    tx_power_data.mandatory = (tx_power_data.tx_power_mandatory != 0);
    DEBUG_LOG("TxPower_Mandatory:Updated Mandatory flag is %s", tx_power_data.mandatory? "TRUE":"FALSE");
}

int8 TxPower_LEGetData(void)
{
    int8 tx_power_level;
    DEBUG_LOG("TxPower_LEGetData");
    if(data_state.tx_power_state_le != tx_power_ready)
    {
        DEBUG_LOG("Tx Power for LE adverts is not available yet");
    }
    /* Add board tx power path loss */
    tx_power_level = tx_power_data.ble_value + TxPower_GetTxPowerPathLoss();
    /* Tx Power level (in dBm) */
    return tx_power_level;
}


bool TxPower_GetMandatoryStatus(void)
{
    return tx_power_data.mandatory;
}


void TxPowerTaskHandler(Task task, MessageId id, Message message)
{
    UNUSED(task);

    DEBUG_LOG("TxPowerTaskHandler: MessageID=%d", id); 
    switch(id)
    {
        case CL_DM_BLE_READ_ADVERTISING_CHANNEL_TX_POWER_CFM:
        {
            CL_DM_BLE_READ_ADVERTISING_CHANNEL_TX_POWER_CFM_T* cfm =
                    (CL_DM_BLE_READ_ADVERTISING_CHANNEL_TX_POWER_CFM_T*) message;
            if(hci_success == cfm->status)
            {
                tx_power_data.ble_value = cfm->tx_power;
                data_state.tx_power_state_le = tx_power_ready;
            }
            else
            {
                DEBUG_LOG("TxPowerTaskHandler: Error while reading LE Tx Power: hci_error = %d", cfm->status);
            }
        }
        break;

        default:
            DEBUG_LOG("TxPowerTaskHandler: Unhandled");
        break;
    }
}


static unsigned int txPower_AdvGetNumberOfItems(const le_adv_data_params_t * params)
{
    /* Return number of items as 1 for these conditions
       1. TxPower is Mandatory and completeness_full.
       2. TxPower is Optional and completeness_skip 
     */
    DEBUG_LOG_V_VERBOSE("txPower_AdvGetNumberOfItems: Completeness=%d", params->completeness);
    if(((params->completeness == le_adv_data_completeness_full && tx_power_data.tx_power_mandatory) ||
       (params->completeness == le_adv_data_completeness_can_be_skipped && !tx_power_data.tx_power_mandatory)) &&
       (params->placement == le_adv_data_placement_advert) &&
       (params->data_set != le_adv_data_set_peer))
    {
        return TX_POWER_NUM_ITEMS;
    }
    else
    {
        return 0;
    }
}


static le_adv_data_item_t txPower_AdvertData(const le_adv_data_params_t * params, unsigned int number)
{
    le_adv_data_item_t adv_data_item={0};

    DEBUG_LOG_V_VERBOSE("txPower_AdvertData: Completeness=%d, number=%d", params->completeness, number);

    if(((params->completeness == le_adv_data_completeness_full && tx_power_data.tx_power_mandatory) ||
       (params->completeness == le_adv_data_completeness_can_be_skipped && !tx_power_data.tx_power_mandatory)) &&
       (params->placement == le_adv_data_placement_advert) &&
       (params->data_set != le_adv_data_set_peer))
    {
        adv_data_item.size = TX_POWER_ADV_SIZE;

        /* Set the data field in the format of advertising packet format. Adhering to le_advertising_mgr */
        tx_power_data.tx_power_adv_data[0] = TX_POWER_ADV_SIZE - 1 ;
        tx_power_data.tx_power_adv_data[1] = (uint8)ble_ad_type_tx_power_level;
        tx_power_data.tx_power_adv_data[2] = TxPower_LEGetData();
        adv_data_item.data = tx_power_data.tx_power_adv_data;
    }
    else
    {
        adv_data_item.size = 0;
        adv_data_item.data = NULL;
    }

    return adv_data_item;
}


Task TxPower_GetTaskData(void)
{
    return &tx_power_task_data;
}


static void txPower_ReleaseItems(const le_adv_data_params_t * params)
{
    UNUSED(params);
}

