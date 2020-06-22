/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file       
\brief      Module for grouping procedures in a list and executing them consecutively.

A procedure script is a way to associate a list of Procedures with a single
Goal. The script engine acts as a wrapper around the procedure list to allow
the goals engine to start / stop / cancel a procedure list in the same way as
a single Procedure.

The script engine executes a procedure script is executed in the following way:

* The procedures are executed one at a time.
* The procedures are executed in the order they are stored in the list.
* If a procedure returns a fail status the script engine stops processing the
  list and completes the associated Goal with a failure code.
** No further procedures in the list are run.
** Procedures which have already run successfully are not affected.
* If the goals engine cancels the Goal associated with the procedure script
  The currently running procedure is cancelled.
** When that procedure calls the cancel confirm function the script engine
   stops processing the procedure script and returns the cancel confirm status
   to the goals engine.
** No further procedures in the list are run.
** Procedures which have already run successfully are not affected.

*/

#ifndef SCRIPT_ENGINE_H
#define SCRIPT_ENGINE_H

#include "procedures.h"


/*! \brief Definition of a scripted sequence of procedures. */
typedef struct
{
    /*! Pointer to an array of procedure function structs for the procedures
        to be run through */
    const procedure_fns_t * const * script_procs;

    /*! Pointer to array of procedure data objects used as params for those
        procedures which require them. Entries may be #NO_DATA for procedures
        which take no parameters. */
    const Message* script_procs_data;

    /*! Number of procedures in the script. */
    size_t script_proc_list_size;
} procedure_script_t;


/*! Helper macro to use when defining a topology script.

    This macro can be used when a procedure has no extra data. */
#define NO_DATA (Message)NULL

/*! Helper macro used in definining a topology script.

    This macro selects the FN element of an entry */
#define XDEF_TOPOLOGY_SCRIPT_FNS(fns,data) &fns

/*! Helper macro used in definining a topology script.

    This macro selects the data element of an entry */
#define XDEF_TOPOLOGY_SCRIPT_DATA(fns,data) data


/*! Macro used to define a Topology script.

    The macro is used to define a script and creates the three tables necessary.
    Three tables are used to reduce space. Combining the function pointer and data
    in the same table will lead to padding, which is undesirable (if simply avoidable).

    An example of how to use the script is shown below. It is recommended to include
    the comment so that the script can be found.

    \param name The base name of the script. _script will be appended
    \param list A macro that takes a macro as a parameter and has a list
        of tuples using that macro. See the example

    \usage

    <PRE>
        #define MY_SCRIPT(ENTRY) \
            ENTRY(proc_allow_connection_over_le_fns,PROC_ALLOW_CONNECTION_OVER_LE_ENABLE), \
            ENTRY(proc_permit_bt_fns, PROC_PERMIT_BT_ENABLE), \
            ENTRY(proc_pair_peer_fns, NO_DATA),

        // Define the script pair_peer_script
        DEFINE_TOPOLOGY_SCRIPT(pair_peer,PEER_PAIR_SCRIPT);
    </PRE>
*/
#define DEFINE_TOPOLOGY_SCRIPT(name, list) \
    const procedure_fns_t * const name##_procs[] = { \
        list(XDEF_TOPOLOGY_SCRIPT_FNS) \
        }; \
    const Message name##_procs_data[] = { \
        list(XDEF_TOPOLOGY_SCRIPT_DATA) \
        }; \
    const procedure_script_t name##_script = { \
        name##_procs, \
        name##_procs_data, \
        ARRAY_DIM(name##_procs), \
    }

/*! \brief Start a scripted sequence of procedures.

    Along with the script and the procedure id associated with it the client
    must provide valid procedure start_cfm and complete_cfm callbacks. The
    script engine will inform the client when the script has started and
    completed via these callbacks.

    \note Currently only one scripted procedure can be running at any one time.
    If a client tries to start a script while another is in progess this
    function will panic.

    \param[in] client_task Task of the client starting the script.
    \param[in] script Pointer to the script to start.
    \param[in] proc Procdure id associated with the script.
    \param[in] proc_start_cfm_fn The client start cfm callback.
    \param[in] proc_complete_cfm_fn The client complete cfm callback.

    \return TRUE if the script was started ok, FALSE otherwise.
*/
bool ScriptEngine_StartScript(Task client_task,
                              const procedure_script_t* script,
                              procedure_id proc,
                              procedure_start_cfm_func_t proc_start_cfm_fn,
                              procedure_complete_func_t proc_complete_fn);

/*! \brief Cancel an in-progress scripted sequence of procedures.

    Cancel a scripted sequence of procedures that was started by #client_task.
    The #client_task is checked to make sure it matches the client_task that
    started the currently running script. If they are different this function
    will panic.

    The script engine will cancel the currently running procedure and then
    end processing the script. It does not try to cancel any procedures in the
    script that have already completed.

    After the current script is cancelled the script engine will be ready to
    start a new script.

    \param[in] client_task Task of the client that started the script.
    \param[in] proc_cancel_cfm_fn The client cancel cfm callback.

    \return TRUE if the script was cancelled, FALSE otherwise.
*/
bool ScriptEngine_CancelScript(Task script_client,
                               procedure_cancel_cfm_func_t proc_cancel_cfm_fn);

#endif /* SCRIPT_ENGINE_H */
