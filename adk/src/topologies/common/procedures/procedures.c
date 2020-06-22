/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file       
\brief      Common procedure handling functions.
*/

#include "procedures.h"

#include <message.h>
#include <panic.h>
#include <vmtypes.h>


#define MAKE_DELAY_CFM_MESSAGE(TYPE)  TYPE##_T *message = PanicUnlessNew(TYPE##_T);

static void Procedures_DelayCfmHandleMessage(Task task, MessageId id, Message message);

typedef struct
{
    TaskData task;
} proceduresDelayCfmTaskData;

const proceduresDelayCfmTaskData procedures_delay_cfm = {Procedures_DelayCfmHandleMessage};

#define ProceduresDelayCfmGetTask() (&procedures_delay_cfm.task)

/*  \brief Internal message Ids for delayed procedure cfm functions.

    When sending a delayed cfm function call, the procedure id is added on to
    the base message Id for the type of cfm it is. This gives it a unique Id
    that can be used to identify which procedure the message is for based only
    on its MessageId.
*/
typedef enum
{
    /* The base Id for delayed complete_cfm function calls. */
    DELAY_COMPLETE_CFM = 0x100,

    /* The base Id for delayed cancel_cfm function calls. */
    DELAY_CANCEL_CFM = 0x200,
} delayCfmMessages;

typedef struct
{
    procedure_complete_func_t comp_fn;
    procedure_id proc;
    procedure_result_t result;
} DELAY_COMPLETE_CFM_T;

typedef struct
{
    procedure_cancel_cfm_func_t cancel_fn;
    procedure_id proc;
    procedure_result_t result;
} DELAY_CANCEL_CFM_T;

/* Check the base delayed cfm message ids allow the full range of
   procedure ids to be supported. */
STATIC_ASSERT(PROCEDURE_ID_MAX >= ((DELAY_CANCEL_CFM - DELAY_COMPLETE_CFM) - 1), Procedures_MaxProcIdBadness);

/* Determine if a MessageId represents a DELAY_COMPLETE_CFM or DELAY_CANCEL_CFM id. */
#define MESSAGE_ID_TO_DELAY_CFM_TYPE(id) ((id) & (DELAY_COMPLETE_CFM | DELAY_CANCEL_CFM))

/* Create a unique MessageId based on the procedure id and the type of cfm. */
#define PROC_ID_TO_DELAY_CFM_MESSAGE_ID(proc_id, base_message_id) ((base_message_id) + (proc_id))


void Procedures_DelayedCompleteCfmCallback(procedure_complete_func_t comp_fn,
                                           procedure_id proc, procedure_result_t result)
{
    PanicFalse(PROCEDURE_ID_MAX >= proc);

    MAKE_DELAY_CFM_MESSAGE(DELAY_COMPLETE_CFM);
    message->comp_fn = comp_fn;
    message->proc = proc;
    message->result = result;
    MessageSend(ProceduresDelayCfmGetTask(), PROC_ID_TO_DELAY_CFM_MESSAGE_ID(proc, DELAY_COMPLETE_CFM), message);
}

void Procedures_DelayedCancelCfmCallback(procedure_cancel_cfm_func_t cancel_fn,
                                         procedure_id proc, procedure_result_t result)
{
    PanicFalse(PROCEDURE_ID_MAX >= proc);

    MAKE_DELAY_CFM_MESSAGE(DELAY_CANCEL_CFM);
    message->cancel_fn = cancel_fn;
    message->proc = proc;
    message->result = result;
    MessageSend(ProceduresDelayCfmGetTask(), PROC_ID_TO_DELAY_CFM_MESSAGE_ID(proc, DELAY_CANCEL_CFM), message);

    /* The procedure is not active any more so cancel any DELAY_COMPLETE_CFM
       that may be in-flight. */
    MessageCancelAll(ProceduresDelayCfmGetTask(), PROC_ID_TO_DELAY_CFM_MESSAGE_ID(proc, DELAY_COMPLETE_CFM));
}

static void Procedures_DelayCfmHandleMessage(Task task, MessageId id, Message message)
{
    UNUSED(task);

    delayCfmMessages type = MESSAGE_ID_TO_DELAY_CFM_TYPE(id);

    switch (type)
    {
    case DELAY_COMPLETE_CFM:
        {
            DELAY_COMPLETE_CFM_T* cfm = (DELAY_COMPLETE_CFM_T*)message;
            cfm->comp_fn(cfm->proc, cfm->result);
        }
        break;

    case DELAY_CANCEL_CFM:
        {
            DELAY_CANCEL_CFM_T* cfm = (DELAY_CANCEL_CFM_T*)message;
            cfm->cancel_fn(cfm->proc, cfm->result);
        }
        break;
    }
}
