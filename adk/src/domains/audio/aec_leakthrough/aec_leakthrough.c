/*!
\copyright  Copyright (c) 2020 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file       aec_leakthrough.c
\brief      Leak-through core logic implementation.
*/

#ifdef ENABLE_AEC_LEAKTHROUGH
#include "aec_leakthrough.h"
#include "kymera.h"
#include "state_proxy.h"

/*! Number of leak-through mode */
#define MAX_NUM_LEAKTHROUGH_MODE    (3)

leakthroughTaskData leakthrough_task_data;

leakthroughTaskData* AecLeakthrough_GetTaskData(void)
{
    return (&leakthrough_task_data);
}

/*! Static function to update the leak-through mode */
static void aecLeakthrough_UpdateMode(leakthrough_mode_t mode)
{
    if(mode < MAX_NUM_LEAKTHROUGH_MODE)
    {
        DEBUG_LOG("aecLeakthrough_UpdateMode");
        leakthroughTaskData *theLeakThrough = AecLeakthrough_GetTaskData();
        theLeakThrough->leakthrough_mode = mode;
    }
    else
    {
        DEBUG_LOG("aecLeakthrough_UpdateMode ignored as inavlid argument for leakthrough mode is passed");
    }
}

static void aecLeakthrough_UpdateState(bool new_leakthrough_state)
{
    bool current_leakthrough_state = AecLeakthrough_IsLeakthroughEnabled();
    DEBUG_LOG("aecLeakthrough_UpdateState: current state = %u, new state = %u", current_leakthrough_state, new_leakthrough_state);

    if(current_leakthrough_state != new_leakthrough_state)
    {
        if(new_leakthrough_state)
        {
            AecLeakthrough_Enable();
        }
        else
        {
            AecLeakthrough_Disable();
        }
    }
}

static void aecLeakthrough_HandleStateProxyEvent(const STATE_PROXY_EVENT_T* event)
{
    switch(event->type)
    {
        case state_proxy_event_type_leakthrough:
            DEBUG_LOG("aecLeakthrough_HandleStateProxyEvent: state proxy leakthrough synchronization");
            aecLeakthrough_UpdateState(event->event.leakthrough_data.state);
            aecLeakthrough_UpdateMode(event->event.leakthrough_data.mode);

        break;

        default:
            break;
    }
}

static void aecLeakthrough_msg_handler(Task task, MessageId id, Message msg)
{
    UNUSED(task);
    switch(id)
    {
        case STATE_PROXY_EVENT:
            aecLeakthrough_HandleStateProxyEvent((const STATE_PROXY_EVENT_T*)msg);
            break;

        default:
            DEBUG_LOG("aecLeakthrough_msg_handler: Event not handled");
            break;
    }
}

/*! \brief Notify Leakthrough update to registered clients. */
static void aecLeakthrough_MsgRegisteredClients(void)
{
    leakthroughTaskData *theLeakThrough = AecLeakthrough_GetTaskData();
     /* Check if any of the clients registered */
    if(theLeakThrough->client_tasks)
    {
        MESSAGE_MAKE(ind, LEAKTHROUGH_UPDATE_IND_T);
        ind->state = AecLeakthrough_IsLeakthroughEnabled();
        ind->mode = AecLeakthrough_GetMode();

        TaskList_MessageSend(theLeakThrough->client_tasks, LEAKTHROUGH_UPDATE_IND, ind);
    }
}

bool AecLeakthrough_IsLeakthroughEnabled(void)
{
    DEBUG_LOG("AecLeakthrough_IsLeakthroughEnabled");
    leakthroughTaskData *theLeakThrough = AecLeakthrough_GetTaskData();
    return theLeakThrough->leakthrough_status;
}

void AecLeakthrough_Enable(void)
{
    DEBUG_LOG("AecLeakthrough_Enable");
    if(!AecLeakthrough_IsLeakthroughEnabled())
    {
        leakthroughTaskData *theLeakThrough = AecLeakthrough_GetTaskData();
        /*! Update the leak-through status */
        theLeakThrough->leakthrough_status = TRUE;
        Kymera_EnableLeakthrough();
        /*! Notify Leakthrough state update to the registered clients */
        aecLeakthrough_MsgRegisteredClients();
    }
    else
    {
        DEBUG_LOG("AecLeakthrough_Enable: Failed as not in LEAKTHROUGH_DISABLE state ");
    }
}

void AecLeakthrough_Disable(void)
{
    DEBUG_LOG("AecLeakthrough_Disable");
    if(AecLeakthrough_IsLeakthroughEnabled())
    {
        leakthroughTaskData *theLeakThrough = AecLeakthrough_GetTaskData();
        /*! Update the leak-through status */
        theLeakThrough->leakthrough_status = FALSE;
        Kymera_DisableLeakthrough();
        /*! Notify Leakthrough state update to registered clients */
        aecLeakthrough_MsgRegisteredClients();
    }
    else
    {
        DEBUG_LOG("AecLeakthrough_Disable: Failed as not in LEAKTHROUGH_ENABLE state ");
    }
}

void AecLeakthrough_SetMode(leakthrough_mode_t mode)
{
    DEBUG_LOG("AecLeakthrough_SetMode Mode=%d", mode);
    if(AecLeakthrough_IsLeakthroughEnabled() && (mode < (MAX_NUM_LEAKTHROUGH_MODE)))
    {
        /* Update the mode */
        aecLeakthrough_UpdateMode(mode);

        /* Update he leakthrough operator ucid */
        Kymera_LeakthroughUpdateAecOperatorUcid();

        /*Update the sidetone Path */
        Kymera_LeakthroughEnableAecSideToneAfterTimeout();

        /* Notify Leakthrough mode update to registered clients */
        aecLeakthrough_MsgRegisteredClients();
    }
    else
    {
        DEBUG_LOG("AecLeakthrough_SetMode: Failed as not in LEAKTHROUGH_ON state or Invalid mode requested");
    }
}

leakthrough_mode_t AecLeakthrough_GetMode(void)
{
    leakthroughTaskData *theLeakThrough = AecLeakthrough_GetTaskData();
    DEBUG_LOG("AecLeakthrough_GetMode. Current Leak-through mode is %u", theLeakThrough->leakthrough_mode);
    return theLeakThrough->leakthrough_mode;
}

void AecLeakthrough_SetNextMode(void)
{
    if(AecLeakthrough_IsLeakthroughEnabled())
    {
        leakthroughTaskData *theLeakThrough = AecLeakthrough_GetTaskData();
        leakthrough_mode_t mode = theLeakThrough->leakthrough_mode;
        mode++;

        /*! If the next leakthrough mode is more than the available modes reset the current mode to default. */
        if(mode >= MAX_NUM_LEAKTHROUGH_MODE)
        {
            theLeakThrough->leakthrough_mode = LEAKTHROUGH_MODE_1;
        }
        else
        {
            theLeakThrough->leakthrough_mode = mode;
        }

        AecLeakthrough_SetMode(theLeakThrough->leakthrough_mode);
    }
    else
    {
        DEBUG_LOG("Ignoring AecLeakthrough_SetNextMode() request as leakthrough is not enabled");
    }
}

/*! TODO Power On/Off functionality to be implemented as per need */
void AecLeakthrough_PowerOn(void)
{
    DEBUG_LOG("AecLeakthrough_PowerOn");
}

void AecLeakthrough_PowerOff(void)
{
    DEBUG_LOG("AecLeakthrough_PowerOff");
}

void AecLeakthrough_ClientRegister(Task client_task)
{
    leakthroughTaskData *theLeakthrough = AecLeakthrough_GetTaskData();
    TaskList_AddTask(theLeakthrough->client_tasks, client_task);
}

void AecLeakthrough_ClientUnregister(Task client_task)
{
    leakthroughTaskData *theLeakthrough = AecLeakthrough_GetTaskData();
    TaskList_RemoveTask(theLeakthrough->client_tasks, client_task);
}

bool AecLeakthrough_Init(Task init_task)
{
    UNUSED(init_task);

    DEBUG_LOG("AecLeakthrough_LeakthroughInit");

    leakthroughTaskData *theLeakThrough = AecLeakthrough_GetTaskData();
    memset(theLeakThrough, 0, sizeof(*theLeakThrough));

    theLeakThrough->task.handler = aecLeakthrough_msg_handler;
    theLeakThrough->client_tasks = TaskList_Create();
    theLeakThrough->leakthrough_status = FALSE;
    theLeakThrough->leakthrough_mode = LEAKTHROUGH_MODE_1;

    return TRUE;
}

void AecLeakthrough_PostInitSetup(void)
{
    StateProxy_EventRegisterClient(&AecLeakthrough_GetTaskData()->task, state_proxy_event_type_leakthrough);
}

#endif /* ENABLE_AEC_LEAKTHROUGH */
