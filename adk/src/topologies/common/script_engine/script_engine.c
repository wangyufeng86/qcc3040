/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file       
\brief      
*/

#include "script_engine.h"
#include "procedures.h"

#include <logging.h>

#include <panic.h>



#define NEXT_SCRIPT_STEP                (TwsTopProcScriptGetTaskData()->next_step)
#define SCRIPT_SIZE(script)             (script->script_proc_list_size)
#define ALL_PROCEDURES_COMPLETE(script) (NEXT_SCRIPT_STEP == SCRIPT_SIZE(script))

#define SCRIPT_PROCEDURES(script)       (((script)->script_procs))
#define SCRIPT_PROCEDURES_DATA(script)  (((script)->script_procs_data))

#define NEXT_PROCEDURE(script)          ((SCRIPT_PROCEDURES(script)[NEXT_SCRIPT_STEP]))
#define NEXT_PROCEDURE_DATA(script)     ((SCRIPT_PROCEDURES_DATA(script)[NEXT_SCRIPT_STEP]))

#define CURRENT_SCRIPT_STEP_VALID(script)   ((NEXT_SCRIPT_STEP < SCRIPT_SIZE((script))))
#define CURRENT_SCRIPT_STEP_IS_END(script)  ((NEXT_SCRIPT_STEP == SCRIPT_SIZE((script))))

/*! */
typedef enum
{
    script_engine_idle,
    script_engine_active,
    script_engine_cancelling,
} script_engine_state_t;

/*! Context data for script engine module. */
typedef struct
{
    /*! Current state of the script engine module */
    script_engine_state_t state;

    /*! Index of the next step to run */
    int next_step;

    /*! Pointer to the currently running script. */
    const procedure_script_t* script;

    /*! Procedure id associated with the running script. */
    procedure_id proc;

    /*! Client that started the current script. */
    Task script_client;
    
    /*! Client procedure callbacks to call when a script starts, completes
        or is cancelled. */
    procedure_start_cfm_func_t start_fn;
    procedure_complete_func_t complete_fn;
    procedure_cancel_cfm_func_t cancel_fn;
} ScriptEngineTaskData;


ScriptEngineTaskData tws_topology_proc_script_engine;

#define TwsTopProcScriptGetTaskData()   (&tws_topology_proc_script_engine)


static void scriptEngine_ProcStartCfm(procedure_id proc, procedure_result_t result);
static void scriptEngine_ProcCompleteCfm(procedure_id proc, procedure_result_t result);
static void scriptEngine_ProcCancelCfm(procedure_id proc, procedure_result_t result);


/******************************************************************************
 * Script engine progress helpers
 *****************************************************************************/

/*! \brief Reset the state of the script engine.

    This function clears the internal script data only. For example, it
    does not call any of the client callbacks.
*/
static void scriptEngine_EngineReset(void)
{
    ScriptEngineTaskData* td = TwsTopProcScriptGetTaskData();
    td->state = script_engine_idle;
    td->next_step = 0;
    td->script = NULL;
}

/*! \brief Run the next procedure in a script

    If the end of the script has been reached inform the client that started
    the script by calling the complete callback that it passed in.

*/
static void scriptEngine_StartCurrentStep(void)
{
    ScriptEngineTaskData* td = TwsTopProcScriptGetTaskData();

    if (CURRENT_SCRIPT_STEP_VALID(td->script))
    {
        DEBUG_LOG("scriptEngine_StartCurrentStep calling start %p", NEXT_PROCEDURE(td->script)->proc_start_fn);
        NEXT_PROCEDURE(td->script)->proc_start_fn(td->script_client,
                                                 scriptEngine_ProcStartCfm,
                                                 scriptEngine_ProcCompleteCfm,
                                                 NEXT_PROCEDURE_DATA(td->script));
    }
    else if (CURRENT_SCRIPT_STEP_IS_END(td->script))
    {
        DEBUG_LOG("scriptEngine_StartCurrentStep script %p success", td->script);

        /* script finished with no failures, inform client */
        scriptEngine_EngineReset();
        td->complete_fn(td->proc, procedure_result_success);
    }
    else
    {
        /* The next_step has gone past the end of the script - this should never happen */
        Panic();
    }
}

/******************************************************************************
 * Script engine procedure callbacks
 *****************************************************************************/

/*! \brief Procedure completion callback for a procedure within a script

    \note We expect all procedures to at least start successfully - so this
    will panic if the procedure fails to start.

    \todo Future use expected to track in-progress procedures for smarter goal resolution

    \param[in] proc Id of the procedure that was started.
    \param[in] result Result code the procedure returned after starting.
*/
static void scriptEngine_ProcStartCfm(procedure_id proc, procedure_result_t result)
{
    ScriptEngineTaskData* td = TwsTopProcScriptGetTaskData();

    DEBUG_LOG("scriptEngine_EngineStartCfm script %p proc %d, result %u", td->script, proc, result);
    
    if (result != procedure_result_success)
    {
        DEBUG_LOG("scriptEngine_EngineStartCfm step %u for script %p failed to start",
                   td->next_step, td->script);
        Panic();
    }
}

/*! \brief Procedure completion callback for a procedure within a script

    Process the completion of the given procedure and then run the next
    procedure in the script.

    If this was the last procedure in the script call the client complete
    callback to inform the client the script is finished.

    If the procedure completed with an error pass this back to the client in
    the client complete callback and end processing of the current script.

    \param[in] proc Id of the procedure that has completed.
    \param[in] result Result code the procedure returned after completion.
*/
static void scriptEngine_ProcCompleteCfm(procedure_id proc, procedure_result_t result)
{
    ScriptEngineTaskData* td = TwsTopProcScriptGetTaskData();
    DEBUG_LOG("scriptEngine_EngineCompleteCfm proc %d, result %u", proc, result);

    /* Only process if we're active */
    if (td->state == script_engine_active)
    {
        switch (result)
        {
            case procedure_result_success:
                NEXT_SCRIPT_STEP++;
                scriptEngine_StartCurrentStep();
                return;

            case procedure_result_timeout:
                {
                    /* Script step timed out. Finish the script and forward the timeout status. 
                       If there is an associted timeout event the goals handler will
                       generate this */
                    DEBUG_LOG("scriptEngine_EngineCompleteCfm step %u timed out, ending script", td->next_step);

                    scriptEngine_EngineReset();
                    td->complete_fn(td->proc, procedure_result_timeout);
                }
                return;

            case procedure_result_failed:
                {
                    /* Script step failed. Finish the script and forward the failed status. 
                       If there is an associted failed event the goals handler will
                       generate this else panics */
                    DEBUG_LOG("scriptEngine_EngineCompleteCfm step %u failed", td->next_step);
                    scriptEngine_EngineReset();
                    td->complete_fn(td->proc, procedure_result_failed);
                }
                return;
        }
        /* No default case in the switch as compilers and static analysis will
           report if an addition proc_result_t enum is added. But we do Panic()
           in case those warnings are missed or ignored.
         */
        Panic();
    }
}

/*! \brief Procedure cancel callback for a procedure within a script

    Inform the client the procedure was cancelled by calling the client
    cancel confirm callback and passing through thre result code.

    Cancelling a procedure within a script is treated as the same as cancelling
    the entire script, so end processing of the current script.

    \note A procedure that is part of a script should only ever be cancelled
    by calling #ScriptEngine_CancelScript.

    \param[in] proc Id of the procedure that has completed.
    \param[in] result Result code the procedure returned after cancellation.
*/
static void scriptEngine_ProcCancelCfm(procedure_id proc, procedure_result_t result)
{
    ScriptEngineTaskData* td = TwsTopProcScriptGetTaskData();
    DEBUG_LOG("scriptEngine_EngineCancelCfm proc %d, result %u", proc, result);
    
    PanicFalse(td->state == script_engine_cancelling);
    
    /* clean up the script engine */
    scriptEngine_EngineReset();

    /* callback with result of cancellation */
    td->cancel_fn(td->proc, result);
}

/******************************************************************************
 * Public functions
 *****************************************************************************/

/*! \brief Start a scripted sequence of procedures. */
bool ScriptEngine_StartScript(Task client_task,
                              const procedure_script_t* script,
                              procedure_id proc,
                              procedure_start_cfm_func_t proc_start_cfm_fn,
                              procedure_complete_func_t proc_complete_fn)
{
    ScriptEngineTaskData* td = TwsTopProcScriptGetTaskData();

    DEBUG_LOG("scriptEngine_EngineStart script %p proc %d", script, proc);

    PanicFalse(td->state == script_engine_idle);

    td->start_fn = proc_start_cfm_fn;
    td->complete_fn = proc_complete_fn;
    td->proc = proc;
    td->script_client = client_task;

    td->state = script_engine_active;
    td->next_step = 0;
    td->script = script;

    scriptEngine_StartCurrentStep();

    return TRUE;
}

/*! \brief Cancel an in-progress scripted sequence of procedures. */
bool ScriptEngine_CancelScript(Task script_client,
                               procedure_cancel_cfm_func_t proc_cancel_cfm_fn)
{
    ScriptEngineTaskData* td = TwsTopProcScriptGetTaskData();

    DEBUG_LOG("scriptEngine_EngineCancel client %x script %p step %u",
                        script_client, td->script, td->next_step);

    PanicFalse(td->script_client == script_client);

    /* can only cancel a script in progress */
    if (td->state != script_engine_active)
    {
        return FALSE;
    }

    /* prevent any further start of script entries */
    td->state = script_engine_cancelling;

    /* remember the cancel comfirm callback */
    td->cancel_fn = proc_cancel_cfm_fn;

    /* cancel the current step */
    NEXT_PROCEDURE(td->script)->proc_cancel_fn(scriptEngine_ProcCancelCfm);

    return TRUE;
}
