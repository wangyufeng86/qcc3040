/*!
\copyright  Copyright (c) 2020 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file       aec_leakthrough.h
\brief      Leak-through API references for other domains/services.
*/

#ifndef AEC_LEAKTHROUGH_H_
#define AEC_LEAKTHROUGH_H_

#include <message.h>
#include <task_list.h>
#include <logging.h>
#include <panic.h>
#include <stdlib.h>
#include <util.h>
#include <byte_utils.h>
#include "domain_message.h"

typedef enum
{
    LEAKTHROUGH_MODE_1,
    LEAKTHROUGH_MODE_2,
    LEAKTHROUGH_MODE_3
}leakthrough_mode_t;

/*! Leak-through task data*/
typedef struct
{
    /*! Leakthrough module task */
    TaskData task;
    /*! List of tasks registered for notifications */
    task_list_t *client_tasks;
    bool leakthrough_status:1;
    leakthrough_mode_t leakthrough_mode;
}leakthroughTaskData;

typedef struct
{
    bool state;
    leakthrough_mode_t mode;
}leakthrough_sync_data_t;

typedef leakthrough_sync_data_t LEAKTHROUGH_UPDATE_IND_T;

/*! \brief Events sent by leakthrough module to the other modules */
typedef enum
{
    LEAKTHROUGH_UPDATE_IND = LEAKTHROUGH_MESSAGE_BASE,
}leakthrough_msg_t;

/*!
    \brief API to know the status of the software leak-through
    \param None
    \returns TRUE if leak-through is enabled otherise FALSE
 */

#ifdef ENABLE_AEC_LEAKTHROUGH
bool AecLeakthrough_IsLeakthroughEnabled(void);
#else
#define AecLeakthrough_IsLeakthroughEnabled() (FALSE)
#endif

/*!
    \brief API to enable the software leak-through
    \param None
    \returns None
 */

#ifdef ENABLE_AEC_LEAKTHROUGH
void AecLeakthrough_Enable(void);
#else
#define AecLeakthrough_Enable() ((void)(0))
#endif

/*!
    \brief API to disable the software leak-through
    \param None
    \returns None
 */

#ifdef ENABLE_AEC_LEAKTHROUGH
void AecLeakthrough_Disable(void);
#else
#define AecLeakthrough_Disable() ((void)(0))
#endif

/*!
    \brief API to set the leak-through mode- works only if the software leakthrough is enabled.
    \param leakthrough mode to set, valid values are LEAKTHROUGH_MODE_1,LEAKTHROUGH_MODE_2,LEAKTHROUGH_MODE_3 of leakthrough_mode_t type.
    \returns None
 */

#ifdef ENABLE_AEC_LEAKTHROUGH
void AecLeakthrough_SetMode(leakthrough_mode_t leakthrough_mode);
#else
#define AecLeakthrough_SetMode(x) ((void)(0))
#endif


/*!
    \brief API to set the next software leak-through mode, writes the default value if the next mode to set is not available.
           (>MAX_NUM_LEAKTHROUGH_MODE).Works only if the software leakthrough is enabled.
    \param None
    \returns None
 */

#ifdef ENABLE_AEC_LEAKTHROUGH
void AecLeakthrough_SetNextMode(void);
#else
#define AecLeakthrough_SetNextMode() ((void)(0))
#endif

/*!
    \brief API to get the current mode of leak-through
    \param None
    \returns current mode of software leak-through.
 */

#ifdef ENABLE_AEC_LEAKTHROUGH
leakthrough_mode_t AecLeakthrough_GetMode(void);
#else
#define AecLeakthrough_GetMode() (LEAKTHROUGH_MODE_1)
#endif

/*!
    \brief API to handle Power On event in leak-through
    \param None
    \returns None
 */

#ifdef ENABLE_AEC_LEAKTHROUGH
void AecLeakthrough_PowerOn(void);
#else
#define AecLeakthrough_PowerOn() ((void)(0))
#endif

/*!
    \brief API to handle Power Off event in leak-through
    \param None
    \returns None
 */

#ifdef ENABLE_AEC_LEAKTHROUGH
void AecLeakthrough_PowerOff(void);
#else
#define AecLeakthrough_PowerOff() ((void)(0))
#endif

/*!
    \brief Initialisation of leak-through
    \param init_task
    \return TRUE after the successful initialization.
 */

#ifdef ENABLE_AEC_LEAKTHROUGH
bool AecLeakthrough_Init(Task init_task);
#else
#define AecLeakthrough_Init(x) (FALSE)
#endif

/*!
    \brief API to register the client to the leakthrough task
    \param client_task
    \returns None
 */

#ifdef ENABLE_AEC_LEAKTHROUGH
void AecLeakthrough_ClientRegister(Task client_task);
#else
#define AecLeakthrough_ClientRegister(x) ((void)(0))
#endif

/*!
    \brief API to un-register the client from the leakthrough task
    \param client_task
    \returns None
 */

#ifdef ENABLE_AEC_LEAKTHROUGH
void AecLeakthrough_ClientUnregister(Task client_task);
#else
#define AecLeakthrough_ClientUnregister(x) ((void)(0))
#endif

/*!
    \brief API to get the leakthrough task data
    \param None
    \return leakthrough task data
 */

#ifdef ENABLE_AEC_LEAKTHROUGH
leakthroughTaskData* AecLeakthrough_GetTaskData(void);
#else
#define AecLeakthrough_GetTaskData() ((void)(0))
#endif

/*! Register with state proxy after initialization */
#ifdef ENABLE_AEC_LEAKTHROUGH
void AecLeakthrough_PostInitSetup(void);
#else
#define AecLeakthrough_PostInitSetup() ((void)(0))
#endif

#endif /* AEC_LEAKTHROUGH_H_ */
