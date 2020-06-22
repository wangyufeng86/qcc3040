/****************************************************************************
Copyright (c) 2004 - 2019 Qualcomm Technologies International, Ltd.


FILE NAME
    init.h
    
DESCRIPTION

*/

#ifndef    CONNECTION_INIT_H_
#define    CONNECTION_INIT_H_


/* Connection Library components. Used to control initialization */
typedef enum
{
    connectionInit          = 0,
    connectionInitDm,
    connectionInitRfc,
    connectionInitL2cap,
    connectionInitUdp,
    connectionInitTcp,
    connectionInitSdp,
	connectionInitVer,
    connectionInitSm,
    connectionInitComplete
}connectionInitState;

/* Initialization timeout period */
#ifdef CONNECTION_DEBUG_LIB
#define INIT_TIMEOUT    (15000)
#else
#define INIT_TIMEOUT    (10000)
#endif /* CONNECTION_DEBUG_LIB */

/****************************************************************************

NAME    
    connectionHandleInternalInit    

DESCRIPTION
    This function is called to control the initialization process.  To avoid race
    conditions at initialization, the process is serialized.

RETURNS
    void
*/
void connectionHandleInternalInit(connectionInitState state);


/****************************************************************************

NAME    
    connectionSendInternalInitCfm    

DESCRIPTION
    This function is called to send a CL_INTERNAL_INIT_CFM message to the 
    Connection Library task

RETURNS
    void
*/
void connectionSendInternalInitCfm(connectionInitState state);


/****************************************************************************

NAME    
    connectionSendInitCfm

DESCRIPTION
    This function is called from the main Connection Library task handler to 
    indicate to the Client application the result of the request to initialize
    the Connection Library.  The application task is passed in as the first
    parameter

RETURNS
    void
*/
void connectionSendInitCfm(Task task, connection_lib_status state, cl_dm_bt_version version);

/****************************************************************************
NAME
    connectionGetInitState

DESCRIPTION
    Get the initialization status of the Connection library

RETURNS
    TRUE if library initialized
*/
bool connectionGetInitState( void );

/****************************************************************************
NAME
    connectionGetLockState

DESCRIPTION
    Get the system lock state of the Connection library

RETURNS
    FALSE if no locks held
*/
bool connectionGetLockState( void );




#endif    /* CONNECTION_INIT_H_ */
