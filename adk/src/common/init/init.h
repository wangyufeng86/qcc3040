/*!
\copyright  Copyright (c) 2018 - 2020 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\defgroup   init Init
\brief      Application initialisation framework based on a table-driven approach.
*/

#ifndef INIT_H
#define INIT_H

#include <message.h>

/*@{*/

/*! \brief Initialisation function prototype.

    An initialisation function can be synchronous or asynchronous.

    Synchronous initialisation functions must return either TRUE or FALSE,
    depending on whether they were successful.

    Asynchronous initialisation functions should return TRUE if the
    initialisation is in progress.

    When the initialisation completes this should be signalled to the init
    module by sending the expected message to the #init_task passed into the
    #init_function. The message may optionally contain data, e.g. a success or
    fail code, in which case a #init_handler for the message can be used to
    check if the initialisation was successful.

    Asynchronous initialisation functions may return FALSE if the
    initialisation fails before it reaches the asynchronous part.

    \param init_task Task to send the expected completion message to when
                     an asynchronous init has finished. Synchronous init
                     functions do not use this.

    \return TRUE if initialisation was successful or is in progress,
            FALSE otherwise.
*/
typedef bool (*init_function)(Task init_task);

/*! \brief Asynchronous initialisation complete handler prototype.

    If an asynchronous init function returns a message with a payload,
    for example a success code, then the user module can use a function
    of this type to process the contents of the payload and tell the
    init module if the initialisation was successful or not.

    \param message [in] Message payload of the initialisation complete message.

    \return TRUE if the initialisation was successful, FALSE otherwise.

*/
typedef bool (*init_handler)(Message message);

/*! \brief Initialisation Table Entry
 */
typedef struct
{
    init_function init; /*!< Initialisation function to call. */
    uint16 async_message_id; /*!< Message ID to wait for, 0 if no message required */
    init_handler async_handler; /*!< Function to call when message with above ID is received. */
} init_table_entry_t;

/*! \brief Payload for APPS_COMMON_INIT_CFM message
 */
typedef struct
{
    bool success; /*!< TRUE if all initialisation functions executed successfully, FALSE otherwise */
} APPS_COMMON_INIT_CFM_T;

/*! \brief Initialisation messages
 */
typedef enum
{
    APPS_COMMON_INIT_CFM = 0x200, /* INIT_MESSAGE_BASE */ /*!< Confirmation of initialisation completion. */
} init_messages_t;


/*! \brief Start initialisation

    This function is called by an application to handle initialisation of all
    parts of the system required by the application. This list of initialisation
    functions is defined in a table that is generated and passed in by the Application.

    The number of rows in the table is given by #init_table_count;

    \param app_task The Task to message once initialisation is complete.
    \param init_table Table of functions that must be called as part of initialisation.
    \param init_table_count The number of rows in the init table.
*/
void AppsCommon_StartInit(Task app_task, const init_table_entry_t* init_table, uint16 init_table_count);

/*! \brief Store task used for initialisation.

    This is helper function allowing clients store their task used for initialisation,
    to be retrieved by Init_GetInitTask().

    Purpose of this function is to provide storage only,
    otherwise stored task is not used by the init component.

    \param init_task Task to be stored.
*/
void Init_SetInitTask(Task init_task);

/*! \brief Retrieve previously stored task.

    \return Previously stored task.
*/
Task Init_GetInitTask(void);

/* \brief Indicate that initialisation is complete.

   Should be called by an application when initialisation process is complete.
   It allows clients to determine if initialisation is complete by using Init_IsCompleted().
*/
void Init_SetCompleted(void);

/* \brief Check if initialisation is complete.

   \return TRUE if initialisation is complete.
*/
bool Init_IsCompleted(void);

/*@}*/

#endif // INIT_H
