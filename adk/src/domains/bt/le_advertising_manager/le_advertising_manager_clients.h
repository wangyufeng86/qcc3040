/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Manage providers of advertising/scan response data to LE advertising manager
*/

#ifndef LE_ADVERTSING_MANAGER_CLIENTS_H_
#define LE_ADVERTSING_MANAGER_CLIENTS_H_

#include "le_advertising_manager.h"
#include "le_advertising_manager_private.h"

struct _le_adv_mgr_register
{
    Task task;
    const le_adv_data_callback_t *callback;
};

typedef struct
{
    le_adv_mgr_register_handle client_handle;
} le_adv_mgr_client_iterator_t;

/*! \brief Initialise le_advertising_manager_clients
*/
void leAdvertisingManager_ClientsInit(void);

/*! \brief Verify client handle is valid
    \return TRUE if valid, FALSE otherwise
*/
bool leAdvertisingManager_ClientHandleIsValid(const struct _le_adv_mgr_register * handle);

/*! \brief Create a new client
    \param task The task for client to receive messages
    \param callback Client callback function pointers
    \return Client handle or NULL if a new client could not be created
*/
le_adv_mgr_register_handle LeAdvertisingManager_NewClient(Task task, const le_adv_data_callback_t * const callback);

/*! \brief Get the head client from the list
    \param iterator Optional iterator to use with leAdvertisingManager_NextClient
    \return The client at the head of the list
*/
le_adv_mgr_register_handle leAdvertisingManager_HeadClient(le_adv_mgr_client_iterator_t* iterator);

/*! \brief Get the next client from an iterator
    \param iterator The iterator
    \return The next client or NULL if the end of the list has been reached
*/
le_adv_mgr_register_handle leAdvertisingManager_NextClient(le_adv_mgr_client_iterator_t* iterator);

/*! \brief Check if the client list is empty
    \return TRUE if empty, otherwise FALSE
*/
bool leAdvertisingManager_ClientListIsEmpty(void);

/*! \brief Get the number of data items a client has for a given set of advertising parameters
    \param client_handle The client handle
    \param params Advertising parameters
    \return number of data items
*/
size_t leAdvertisingManager_ClientNumItems(le_adv_mgr_register_handle client_handle, const le_adv_data_params_t* params);

#endif /* LE_ADVERTSING_MANAGER_CLIENTS_H_ */
