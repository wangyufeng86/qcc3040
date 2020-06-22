/*!
\copyright  Copyright (c) 2018 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file       
\brief      Internal defines used by the advertising manager
*/

#ifndef LE_ADVERTISING_MANAGER_PRIVATE_H_

#define LE_ADVERTISING_MANAGER_PRIVATE_H_

#include "le_advertising_manager.h"
#include "logging.h"

/*! Macro to make a message based on type. */
#define MAKE_MESSAGE(TYPE) TYPE##_T *message = PanicUnlessNew(TYPE##_T);

/*! Macro to make a variable length message based on type. */
#define MAKE_MESSAGE_VAR(VAR, TYPE) TYPE##_T *VAR = PanicUnlessNew(TYPE##_T);

/*! Logging Macros */
#if defined DEBUG_LOG_EXTRA

#define DEBUG_LOG_LEVEL_1 DEBUG_LOG /* Additional Failure Logs */
#define DEBUG_LOG_LEVEL_2 DEBUG_LOG /* Additional Information Logs */

#else

#define DEBUG_LOG_LEVEL_1(...) ((void)(0))
#define DEBUG_LOG_LEVEL_2(...) ((void)(0))

#endif

#define DEFAULT_ADVERTISING_INTERVAL_MIN_IN_SLOTS 148 /* This value is in units of 0.625 ms */
#define DEFAULT_ADVERTISING_INTERVAL_MAX_IN_SLOTS 160 /* This value is in units of 0.625 ms */

/* Number of clients supported that can register callbacks for advertising data */
#define MAX_NUMBER_OF_CLIENTS 10

/*! Maximum data length of an advert if advertising length extensions are not used */
#define MAX_AD_DATA_SIZE_IN_OCTETS  (0x1F)

/*! Given the total space available, returns space available once a header is included

    This makes allowance for being passed negative values, or space remaining
    being less that that needed for a header.

    \param[in] space    Pointer to variable holding the remaining space
    \returns    The usable space, having allowed for a header
*/
#define USABLE_SPACE(space)             ((*space) > AD_DATA_HEADER_SIZE ? (*space) - AD_DATA_HEADER_SIZE : 0)

/*! Helper macro to return total length of a field, once added to advertising data

    \param[in] data_length  Length of field

    \returns    Length of field, including header, in octets
*/
#define AD_FIELD_LENGTH(data_length)    (data_length + 1)

/*! Size of flags field in advertising data */
#define FLAGS_DATA_LENGTH           (0x02)

/*! Calculate value for the maximum possible length of a name in advertising data */
#define MAX_AD_NAME_SIZE_OCTETS     (MAX_AD_DATA_SIZE_IN_OCTETS - AD_DATA_HEADER_SIZE)

/*! Minimum length of the local name being advertised, if we truncate */
#define MIN_LOCAL_NAME_LENGTH       (0x10)

typedef struct
{
    bool   action;
} LE_ADV_INTERNAL_MSG_ENABLE_ADVERTISING_T;

/*! Enumerated type for messages sent within the advertising manager only. */
enum adv_mgr_internal_messages_t
{
        /*! Start advertising using this advert */
    ADV_MANAGER_START_ADVERT = 1,
        /*! Set advertising data using this advert. Used for connections (from Gatt) */
    ADV_MANAGER_SETUP_ADVERT,
    LE_ADV_INTERNAL_MSG_ENABLE_ADVERTISING,
    LE_ADV_INTERNAL_MSG_NOTIFY_RPA_CHANGE,
    LE_ADV_MGR_INTERNAL_START,
    LE_ADV_MGR_INTERNAL_MSG_NOTIFY_INTERVAL_SWITCHOVER
};

/*! Advertising manager task structure */
typedef struct
{
    /*! Task for advertisement management */
    TaskData                    task;
    /*! Local state for advertising manager */
    adv_mgr_state_t             state;
    /*! Bitmask for allowed advertising event types */
    uint8                       mask_enabled_events;
    /*! Flag to indicate enabled/disabled state of all advertising event types */
    bool                        is_advertising_allowed;
    /*! Flag to indicate if data update is required */
    bool                        is_data_update_required;
    /*! Selected handset advertising data set for the undirected advertising */
    le_adv_data_set_handle      dataset_handset_handle;
    /*! Selected peer advertising data set for the undirected advertising */
    le_adv_data_set_handle      dataset_peer_handle; 
    /*! Configured advertising parameter set for the undirected advertising */
    le_adv_params_set_handle    params_handle;
    /*! The condition (internal) that the blocked operation is waiting for */
    uint16                      blockingCondition;
} adv_mgr_task_data_t;

/*!< Task information for the advertising manager */
extern adv_mgr_task_data_t  app_adv_manager;

/*! Get the advertising manager data structure */
#define AdvManagerGetTaskData()      (&app_adv_manager)

/*! Get the advertising manager task */
#define AdvManagerGetTask()  (&app_adv_manager.task)

struct _le_adv_data_set
{
    Task task;
    le_adv_data_set_t set;
};

struct _le_adv_params_set
{
    /*! Registered advertising parameter sets */
    le_adv_parameters_set_t * params_set;
    /*! Registered advertising parameter config table */
    le_adv_parameters_config_table_t * config_table;
    /*! Selected config table entry */
    uint8 index_active_config_table_entry;
    /*! Selected advertising parameter set */
    le_adv_preset_advertising_interval_t active_params_set;
};

typedef struct
{
    le_adv_data_set_t set;

} LE_ADV_MGR_INTERNAL_START_T;

/*! Enumerated type used to note reason for blocking

    Advertising operations can be delayed while a previous operation completes.
    The reason for the delay is recorded using these values */
typedef enum {
    ADV_SETUP_BLOCK_NONE,               /*!< No blocking operation at present */
    ADV_SETUP_BLOCK_ADV_DATA_CFM = 1,   /*!< Blocked pending appAdvManagerSetAdvertisingData() completing */
    ADV_SETUP_BLOCK_ADV_PARAMS_CFM = 2, /*!< Blocked pending appAdvManagerHandleSetAdvertisingDataCfm completing */
    ADV_SETUP_BLOCK_ADV_SCAN_RESPONSE_DATA_CFM = 3,
    ADV_SETUP_BLOCK_ADV_ENABLE_CFM = 4,
    ADV_SETUP_BLOCK_INVALID = 0xFF
} adv_mgr_blocking_state_t;

/*! Data type for the supported advertising events */
typedef enum
{
    le_adv_event_type_connectable_general = 1UL<<0,
    le_adv_event_type_connectable_directed = 1UL<<1,
    le_adv_event_type_nonconnectable_discoverable = 1UL<<2,
    le_adv_event_type_nonconnectable_nondiscoverable = 1UL<<3

} le_adv_event_type_t;

/*! Data structure to specify the input parameters for leAdvertisingManager_Start() API function */
typedef struct
{
    le_adv_data_set_t set;
    le_adv_data_set_t set_awaiting_select_cfm_msg;
    le_adv_event_type_t event;    
}le_advert_start_params_t;

#endif
