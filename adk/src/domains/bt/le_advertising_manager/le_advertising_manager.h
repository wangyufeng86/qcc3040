/*!
\copyright  Copyright (c) 2018 - 2019 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Header file for management of Bluetooth Low Energy advertising settings

Provides control for Bluetooth Low Energy (BLE) advertisements.
*/


#ifndef LE_ADVERTSING_MANAGER_H_
#define LE_ADVERTSING_MANAGER_H_

#include "domain_message.h"

#include <connection.h>
#include <connection_no_ble.h>

/*! Size of a data element header in advertising data

      * Octet[0]=length,
      * Octet[1]=Tag
      * Octets[2]..[n]=Data
*/
#define AD_DATA_LENGTH_OFFSET       (0)
#define AD_DATA_TYPE_OFFSET         (1)
#define AD_DATA_HEADER_SIZE         (2)

/*! \brief Data type to specify the preset advertising interval for advertising events */
typedef enum
{
le_adv_preset_advertising_interval_slow,
le_adv_preset_advertising_interval_fast,
le_adv_preset_advertising_interval_max = le_adv_preset_advertising_interval_fast,
le_adv_preset_advertising_interval_total
} le_adv_preset_advertising_interval_t;

/*! \brief Data type to specify the supported advertising config sets */
typedef enum
{
le_adv_advertising_config_set_1,
le_adv_advertising_config_set_2,
le_adv_advertising_config_set_3,
le_adv_advertising_config_set_max = le_adv_advertising_config_set_3,
le_adv_advertising_config_set_total
} le_adv_advertising_config_set_t;

/*! \brief Common LE advertising parameters in units of 0.625 ms */
typedef struct
{
 uint16 le_adv_interval_min;
 uint16 le_adv_interval_max;
} le_adv_common_parameters_t;

/*! \brief Parameter set (one for each type (slow/fast)) */
typedef struct
{
 le_adv_common_parameters_t set_type[le_adv_preset_advertising_interval_total];
} le_adv_parameters_set_t;

/*! \brief Parameter config entry */
typedef struct
{
    le_adv_preset_advertising_interval_t set_default;
    uint16 timeout_fallback_in_seconds;
}le_adv_parameters_config_entry_t;

/*! \brief Parameter config table with entries, one for each tuple [index_set_default, timeout_fallback]) */
typedef struct
{
    le_adv_parameters_config_entry_t row[le_adv_advertising_config_set_total];
}le_adv_parameters_config_table_t;

/*! \brief LE advertising parameter sets (slow and fast parameters only) */
typedef struct
{
 const le_adv_parameters_set_t *sets;
 const le_adv_parameters_config_table_t *table;
} le_adv_parameters_t;

/*! \brief Data type for API function return values to indicate success/error status */
typedef enum{
    le_adv_mgr_status_success,
    le_adv_mgr_status_error_unknown
}le_adv_mgr_status_t;

/*! State of the advertising manager (internal) */
typedef enum _le_advertising_manager_state_t
{
        /*! Initial state of advertising manager */
    ADV_MGR_STATE_STARTING,
        /*! Initialisation completed. Any external information has now been retrieved. */
    ADV_MGR_STATE_INITIALISED,
        /*! There is an active advert (on the stack) */
    ADV_MGR_STATE_ADVERTISING,
} adv_mgr_state_t;

/*! Type of discoverable advertising to use.

    The value of defines from connection.h are used, but note that only
    a subset are available in the enum. */
typedef enum
{
        /*! Not using discoverable advertising */
    ble_discoverable_mode_none = 0,
        /*! LE Limited Discoverable Mode */
    ble_discoverable_mode_limited = BLE_FLAGS_LIMITED_DISCOVERABLE_MODE,
        /*! LE General Discoverable Mode */
    ble_discoverable_mode_general = BLE_FLAGS_GENERAL_DISCOVERABLE_MODE,
} adv_mgr_ble_discoverable_mode_t;

/*! \todo work out what this is for. Note that have removed broadcasting reasons. */
typedef enum
{
    ble_gap_read_name_advertising  = 1,
    ble_gap_read_name_gap_server,// = 2,
    ble_gap_read_name_associating,//= 8,
} adv_mgr_ble_gap_read_name_t;

/*! \brief Opaque type for LE Advertising Manager data set object. */
struct _le_adv_data_set;
/*! \brief Handle to LE Advertising Manager data set object. */
typedef struct _le_adv_data_set * le_adv_data_set_handle;

/*! \brief Opaque type for LE Advertising Manager params set object. */
struct _le_adv_params_set;
/*! \brief Handle to LE Advertising Manager params set object. */
typedef struct _le_adv_params_set * le_adv_params_set_handle;

struct _adv_mgr_advert_t;

/*! Definition for an advert. The structure will contain all needed settings.

    This is an anonymous type as no external code should need direct access.
*/
typedef struct _adv_mgr_advert_t adv_mgr_advert_t;

/*! \brief Data type to define whether the data is strictly needed in full or can be skipped/shortened for LE advertising */
typedef enum
{
 le_adv_data_completeness_full,
 le_adv_data_completeness_can_be_shortened,
 le_adv_data_completeness_can_be_skipped
} le_adv_data_completeness_t;
/*! \brief Data type to define the advertisement packet types where the data item is aimed at */
typedef enum
{
 le_adv_data_placement_advert,
 le_adv_data_placement_scan_response,
 le_adv_data_placement_dont_care
} le_adv_data_placement_t;
/*! \brief Data type for the supported advertising data sets */
typedef enum
{
 le_adv_data_set_handset_identifiable = 1UL<<0,
 le_adv_data_set_handset_unidentifiable = 1UL<<1,
 le_adv_data_set_peer =  1UL<<2
} le_adv_data_set_t;
/*! \brief Data structure to specify the attributes for the individual data items */
typedef struct
{
 le_adv_data_set_t data_set;
 le_adv_data_completeness_t completeness;
 le_adv_data_placement_t placement;
}le_adv_data_params_t;

/*! \brief Data structure for the individual data items */
typedef struct
{
    unsigned size;
    const uint8 * data;
}le_adv_data_item_t;

/*! \brief Data structure to specify the callback functions for the data items to be registered */
typedef struct
{
 unsigned int (*GetNumberOfItems)(const le_adv_data_params_t * params);
 le_adv_data_item_t (*GetItem)(const le_adv_data_params_t * params, unsigned int);
 void (*ReleaseItems)(const le_adv_data_params_t * params);
}le_adv_data_callback_t;

/*! \brief Opaque type for LE Advertising Manager registry object */
struct _le_adv_mgr_register;

/*! \brief Handle to LE Advertising Manager registry object */
typedef struct _le_adv_mgr_register * le_adv_mgr_register_handle;

/*! \brief Data type for the message identifiers */
typedef enum
{
    /*! Message signalling the battery module initialisation is complete */
    APP_ADVMGR_INIT_CFM = ADV_MANAGER_MESSAGE_BASE,
    /*! Message responding to AdvertisingManager_Start */
    APP_ADVMGR_ADVERT_START_CFM,
    /*! Message responding to AdvertisingManager_SetAdvertData */
    APP_ADVMGR_ADVERT_SET_DATA_CFM,
    LE_ADV_MGR_SELECT_DATASET_CFM,
    LE_ADV_MGR_RELEASE_DATASET_CFM,
    LE_ADV_MGR_ENABLE_CONNECTABLE_CFM,
    LE_ADV_MGR_ALLOW_ADVERTISING_CFM,
    LE_ADV_MGR_NOTIFY_DATA_CHANGE_CFM,
    LE_ADV_MGR_RPA_TIMEOUT_IND,
}le_adv_mgr_message_id_t;

/*! Message sent when AdvertisingManager_Start() completes */
typedef struct
{
    /*! Final result of the AdvertisingManager_Start() operation */
    connection_lib_status   status;
} APP_ADVMGR_ADVERT_START_CFM_T;

/*! Message sent when AdvertisingManager_SetAdvertData() completes */
typedef struct
{
    /*! Final result of the AdvertisingManager_SetAdvertData() operation */
    connection_lib_status   status;
} APP_ADVMGR_ADVERT_SET_DATA_CFM_T;

/*! \brief Data structure for the confirmation message LE_ADV_MGR_SELECT_DATASET_CFM */
typedef struct
{
    le_adv_mgr_status_t status;
}LE_ADV_MGR_SELECT_DATASET_CFM_T;

/*! \brief Data structure for the confirmation message LE_ADV_MGR_RELEASE_DATASET_CFM */
typedef struct
{
    le_adv_mgr_status_t status;
}LE_ADV_MGR_RELEASE_DATASET_CFM_T;

/*! \brief Data structure for the confirmation message LE_ADV_MGR_ADVERT_ENABLE_CONNECTABLE_CFM */
typedef struct
{
    le_adv_mgr_status_t status;
    bool enable;

}LE_ADV_MGR_ENABLE_CONNECTABLE_CFM_T;

/*! \brief Data structure for the confirmation message LE_ADV_MGR_ADVERT_ALLOW_CFM */
typedef struct
{
    le_adv_mgr_status_t status;
    bool allow;

}LE_ADV_MGR_ALLOW_ADVERTISING_CFM_T;

/*! \brief Data structure for the confirmation message LE_ADV_MGR_NOTIFY_DATA_CHANGE_CFM */
typedef struct
{
    le_adv_mgr_status_t status;
}LE_ADV_MGR_NOTIFY_DATA_CHANGE_CFM_T;

/*! \brief Data structure to specify the input parameters for LeAdvertisingManager_SelectAdvertisingDataSet() API function */
typedef struct
{
    le_adv_data_set_t set;
}le_adv_select_params_t;

/*! \brief Data type for own address types */
typedef enum
{
    le_adv_own_address_type_public = 0x0,
    le_adv_own_address_type_random = 0x1,
    le_adv_own_address_type_rpa_fallback_public = 0x2,
    le_adv_own_address_type_rpa_fallback_random = 0x3
}le_adv_own_address_type_t;

/*! \brief Data structure to specify the use of public address or resolvable private address with/without configurable timer for the own address */
typedef struct
{
    le_adv_own_address_type_t own_address_type;
    uint16 timeout;
}le_adv_own_addr_config_t;

/*! Initialise LE Advertising Manager

    \param init_task
           Task to send init completion message to

    \return TRUE to indicate successful initialisation.
            FALSE otherwise.
*/
bool LeAdvertisingManager_Init(Task init_task);

/*! Deinitialise LE Advertising Manager

    \param

    \return TRUE to indicate successful uninitialisation.
            FALSE to indicate failure.
*/
bool LeAdvertisingManager_DeInit(void);

/*! \brief Public API to allow/disallow LE advertising events

    \param[in] task Task to send confirmation message
    
    \param[in] allow Boolean value, TRUE for allow operation, FALSE for disallow operation.

    \return status_t API status
    \return Sends LE_ADV_MGR_ADVERT_ALLOW_CFM message to the task

*/
bool LeAdvertisingManager_AllowAdvertising(Task task, bool allow);

/*! \brief Public API to enable/disable connectable LE advertising events

    \param[in] task Task to send confirmation message
    
    \param[in] enable Boolean value, TRUE for enable operation, FALSE for disable operation.

    \return status_t API status
    \return Sends LE_ADV_MGR_ADVERT_ENABLE_CONNECTABLE_CFM message to the task               

*/
bool LeAdvertisingManager_EnableConnectableAdvertising(Task task, bool enable);

/*! \brief Public API to register callback functions for advertising data
    \param[in] task The task to receive messages from LE advertising manager when advertising states change
    \param[in] callback A const pointer to the data structure of type le_adv_data_callback_t to specify function pointers for LE Advertising Manager to use to collect the data items to be advertised
    \note The advertising manager only stores a pointer, so the callback object needs to have a lifetime
          as long as the system (or until the unimplemented DeRegister function is provided).
    \return Valid pointer to the handle of type le_adv_mgr_register_handle, NULL otherwise.
*/
le_adv_mgr_register_handle LeAdvertisingManager_Register(Task task, const le_adv_data_callback_t * const callback);

/*! \brief Handler for connection library messages not sent directly

    This function is called to handle any connection library messages sent to
    the application that the advertising module is interested in. If a message
    is processed then the function returns TRUE.

    \note Some connection library messages can be sent directly as the
        request is able to specify a destination for the response.

    \param[in]  id              Identifier of the connection library message
    \param[in]  message         A valid pointer to the instance of message structure if message has any payload, NULL otherwise.
    \param[in]  already_handled Indication whether this message has been processed by
                            another module. The handler may choose to ignore certain
                            messages if they have already been handled.

    \return TRUE if the message has been processed, otherwise returns the
        value in already_handled
 */
extern bool LeAdvertisingManager_HandleConnectionLibraryMessages(MessageId id, Message message, bool already_handled);

/*! \brief Select LE Advertising Data Set
\param[in] task The task to receive LE_ADV_MGR_SELECT_DATASET_CFM message from LE Advertising Manager
\param[in] params Pointer to an instance of advertising select parameters of type le_adv_select_params_t
\return Valid pointer to indicate successful LE advertising data set select operation, NULL otherwise.
\return Sends LE_ADV_MGR_SELECT_DATASET_CFM message when LE advertising gets started for the selected data set.
*/
le_adv_data_set_handle LeAdvertisingManager_SelectAdvertisingDataSet(Task task, const le_adv_select_params_t * params);

/*! \brief Release LE Advertising Data Set
    \param Handle to the advertising data set object returned from the call to the API LeAdvertisingManager_SelectAdvertisingDataSet().

    \return TRUE to indicate successful LE advertising data set release operation, FALSE otherwise.
    \return Sends LE_ADV_MGR_RELEASE_CFM message when LE advertising gets terminated for the released data set. 
*/
bool LeAdvertisingManager_ReleaseAdvertisingDataSet(le_adv_data_set_handle handle);

/*! \brief Public API to notify the changes in the advertising data
\param[in] task The task to receive message LE_ADV_MGR_NOTIFY_DATA_CHANGE_CFM from LE advertising manager
\param[in] handle Handle returned from the call to the API LeAdvertisingManager_Register().
\return TRUE for a succesful call to API for notification, FALSE otherwise.
\return Sends LE_ADV_MGR_NOTIFY_DATA_CHANGE_CFM when LE advertising gets started with the modified data items.
*/
bool LeAdvertisingManager_NotifyDataChange(Task task, const le_adv_mgr_register_handle handle);

/*! \brief Register the parameters to be used by LE advertising manager.
\param[in] params Pointer to LE advertising parameters
\return TRUE for a succesful call to API for parameter register operation, FALSE otherwise.
*/
bool LeAdvertisingManager_ParametersRegister(const le_adv_parameters_t *params);

/*! \brief Select the use of set in the array of LE advertising parameters.
\param[in] index The index to activate
\return TRUE for a succesful call to API for parameter select operation, FALSE otherwise.
*/
bool LeAdvertisingManager_ParametersSelect(uint8 index);

/*! \brief Public API to get LE Advertising interval min and max values
\param[out] interval Pointer to an instance of advertising interval min and max values of the type le_adv_common_parameters_t
\return TRUE to indicate successful advertising interval parameter get operation, FALSE otherwise.
*/
bool LeAdvertisingManager_GetAdvertisingInterval(le_adv_common_parameters_t * interval);

/*! \brief Public API to get the configuration for the own address
\param[out] own_address_config The own address configuration parameter of the type le_adv_own_addr_config_t
\return TRUE to indicate successful own address config get operation, FALSE otherwise.
*/
bool LeAdvertisingManager_GetOwnAddressConfig(le_adv_own_addr_config_t * own_address_config);

#endif /* LE_ADVERTSING_MANAGER_H_ */
