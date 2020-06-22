/*!
\copyright  Copyright (c) 2008 - 2019 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file       earbud_init.h
\brief      Header file for initialisation module
*/

#ifndef EARBUD_INIT_H
#define EARBUD_INIT_H

#include <message.h>

 /*! \brief Initialisation module data */
typedef struct
{
    /*!< Init's local task */
    TaskData task;
#ifdef USE_BDADDR_FOR_LEFT_RIGHT
    uint8 appInitIsLeft:1;  /*!< Set if this is the left earbud */
#endif
} initData;

/*!< Structure used while initialising */
extern initData    app_init;

/*! Get pointer to init data structure */
#define InitGetTaskData()        (&app_init)

/*! \brief Initialise init module
    This function is the start of all initialisation.

    When the initialisation sequence has completed #INIT_CFM will
    be sent to the main app Task.
*/
void appInit(void);

/*! Let the application initialisation function know that initialisation is complete

    This should be called on receipt of the #APPS_COMMON_INIT_CFM message.
 */
void appInitSetInitialised(void);

#endif /* EARBUD_INIT_H */
