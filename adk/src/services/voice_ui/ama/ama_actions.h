/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file       ama_actions.h
\brief      Interface for handling ama user events
*/

#ifndef AMA_ACTIONS_H
#define AMA_ACTIONS_H


#include <stdlib.h>
#include <csrtypes.h>
#include <stdio.h>
#include "ui_inputs.h"
/*! \brief init the VaActionsTranslationTable
    \param[in] void
    \return void
*/
void AmaActions_Init(void);

/*! \brief Pass the VA user event to ama
    \param ui_input_t user input event
    \return bool TRUE if event was handled
*/
bool AmaActions_HandleVaEvent(ui_input_t voice_assistant_user_event);
#endif /* AMA_ACTIONS_H */
