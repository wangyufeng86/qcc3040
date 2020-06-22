/*!
\copyright  Copyright (c) 2015 - 2019 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file
\defgroup   bredr_scan_manager
\ingroup    bt_domain
\brief	    Interface to module managing inquiry and page scanning.
*/

#ifndef BREDR_SCAN_MANAGER_H_
#define BREDR_SCAN_MANAGER_H_

#include "message.h"
#include "domain_message.h"

/*! BREDR scan manager external messages. Message ordering between page and
    inquiry scan 'pairs' of messages should be maintained - inquiry message
    should be listed after page message. */
enum bredr_scan_manager_messages
{
    /*! Confirm initialisation is complete */
    BREDR_SCAN_MANAGER_INIT_CFM = BREDR_SCAN_MANAGER_MESSAGE_BASE,

    /*! Indication page scan has paused - another client has forced-off the scan */
    BREDR_SCAN_MANAGER_PAGE_SCAN_PAUSED_IND,

    /*! Indication inquiry scan has paused - another client has forced-off the scan */
    BREDR_SCAN_MANAGER_INQUIRY_SCAN_PAUSED_IND,

    /*! Indication page scan has resumed - the client has released the force-off. */
    BREDR_SCAN_MANAGER_PAGE_SCAN_RESUMED_IND,

    /*! Indication inquiry scan has resumed - the client has released the force-off. */
    BREDR_SCAN_MANAGER_INQUIRY_SCAN_RESUMED_IND,

    /*! Confirmation scanning has been disabled - sent to the client that requested
        the scan disable using #BredrScanManager_ScanDisable when scanning has been
        disabled. */
    BREDR_SCAN_MANAGER_DISABLE_CFM,

};

/*! Message content, confirming disable success or cancellation. */
typedef struct
{
    /*! TRUE = disabled, FALSE = disable was cancelled */
    bool disabled;

} BREDR_SCAN_MANAGER_DISABLE_CFM_T;


/*! @brief Enumeration of types of scanning parameters.

    This is an ordered list, the higher the value the higher the priority of
    the parameters. So a request for slow parameters when fast are already
    running will not be honoured (though the scan will still 'start'), while a
    request for fast parameters when slow are already configured will result
    in the parameters being changed to fast.
*/
typedef enum
{
    /*! Slow parameters type, lower duty cycle. */
    SCAN_MAN_PARAMS_TYPE_SLOW,

    /*! Fast parameters type, higher duty cycle. */
    SCAN_MAN_PARAMS_TYPE_FAST,

    /*! Always the largest acceptable type */
    SCAN_MAN_PARAMS_TYPE_MAX = SCAN_MAN_PARAMS_TYPE_FAST,

    /*! Total number of types */
    SCAN_MAN_PARAMS_TYPES_TOTAL,

} bredr_scan_manager_scan_type_t;

/*! \brief Defines a type for the function pointer TO the callback used in
           ScanManager_ConfigureEirData API.*/
typedef void (*eir_setup_complete_callback_t)(bool success);

/*! Common page and inquiry scan parameters */
typedef struct
{
    /*! The scan interval in slots */
    uint16 interval;
    /*! The scan window in slots, one for each type */
    uint16 window;

} bredr_scan_manager_scan_parameters_t;

/*! Parameter set (one for each type (slow/fast)) */
typedef struct
{
    /*! Parameter set */
    bredr_scan_manager_scan_parameters_t set_type[SCAN_MAN_PARAMS_TYPES_TOTAL];

} bredr_scan_manager_scan_parameters_set_t;


/*! Page and Inquiry Scan Parameter Sets (slow and fast parameters only) */
typedef struct
{
    /*! Scan params (array of sets) */
    const bredr_scan_manager_scan_parameters_set_t *sets;
    /*! Number of scan parameter sets in the array */
    uint8 len;

} bredr_scan_manager_parameters_t;

/*! @brief Initialise the scan manager data structure.
           #BREDR_SCAN_MANAGER_INIT_CFM is sent to init_task when initialisation
           is complete.
*/
bool BredrScanManager_Init(Task init_task);

/*! @brief Register the page scan parameters to be used by the scan manager.
    @param params Pointer to the page scan parameters.
*/
void BredrScanManager_PageScanParametersRegister(const bredr_scan_manager_parameters_t *page_scan_params);

/*! @brief Select the use of set in the array of page scan parameters.
    @param index The index to activate.
*/
void BredrScanManager_PageScanParametersSelect(uint8 index);

/*! @brief Register the inquiry scan parameters to be used by the scan manager.
    @param params Pointer to the inquiry scan parameters.
*/
void BredrScanManager_InquiryScanParametersRegister(const bredr_scan_manager_parameters_t *inquiry_scan_params);

/*! @brief Select the use of a set in the array of inquiry scan parameters.
    @param index The index to activate.
*/
void BredrScanManager_InquiryScanParametersSelect(uint8 index);

/*! @brief Request inquiry scanning for a specifc client.

    Note the parameters may not be honoured if another scan is already running
    with higher priority parameters.

    @param client     Client's task requesting the scan.
    @param inq_type Type of inquiry scan parameters;
 */
void BredrScanManager_InquiryScanRequest(Task client, bredr_scan_manager_scan_type_t inq_type);

/*! @brief Request page scanning for a specified client.

    Note the parameters may not be honoured if another scan is already running
    with higher priority parameters.

    @param client      Client type requesting the scan.
    @param page_type Type of page scan parameters;
*/
void BredrScanManager_PageScanRequest(Task client, bredr_scan_manager_scan_type_t page_type);

/*! @brief Release inquiry scanning request for a specified client.

    @param client Client type requesting the scan.
*/
void BredrScanManager_InquiryScanRelease(Task client);

/*! @brief Release page scanning request for a specified client.

    @param client Client type requesting the scan.
*/
void BredrScanManager_PageScanRelease(Task client);

/*! @brief Determine if page scan is requested enabled for a client.

    @param client Client to check.

    @return bool TRUE if the client has requested page scan is enabled, FALSE if
            the client has not made a request.
 */
bool BredrScanManager_IsPageScanEnabledForClient(Task client);

/*! @brief Configure the Eir data for the device.

    @param callback_function Function to call once the configuration
                             of the EIR data has finished.

    @return TRUE Configuration has started, FALSE The setup is already in progress so cannot
                 currenty process another request.
 */
bool ScanManager_ConfigureEirData(eir_setup_complete_callback_t callback_function);


/*! \brief Determine if scanning is disabled in all scan contexts.
    \return TRUE if disabled.
*/
bool BredrScanManager_IsScanDisabled(void);

#endif /* BREDR_SCAN_MANAGER_H_ */
