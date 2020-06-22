/****************************************************************************
Copyright (c) 2019 Qualcomm Technologies International, Ltd.


FILE NAME
    handover_if.h

DESCRIPTION
    Header file for the TWS Mirroring Handover interface library.

*/

/*!
\defgroup twsm twsm
\ingroup vm_libs

\brief  Interface TWS Mirroring Handover API.

\section Handover_Interface_intro INTRODUCTION

        Libraries that will be marshalled should internally
        implement the interface functions defined below
        for veto, marshal, unmarshal, commit, complete and abort.
        These are then exposed through the handover_interface
        struct

\example Usage example

* A library that needs to implement the handover interface
* should declare and define the necessary functions as
* specified by the function typedefs below.  They should
* be declared internal to the library and not exposed
* by the external API.
*
* Pointers to these functions are then held in an instance of
* the handover_interface struct, this struct is then exposed
* by the library API.

* library_handover.c

static bool libraryVeto(void);
static bool libraryMarshal(const tp_bdaddr *tp_bd_addr,
                       uint8 *buf,
                       uint16 length,
                       uint16 *written);
static bool libraryUnmarshal(const tp_bdaddr *tp_bd_addr,
                         const uint8 *buf,
                         uint16 length,
                         uint16 *consumed);
static void libraryHandoverCommit(const tp_bdaddr *tp_bd_addr, const bool newRole);
static void libraryHandoverComplete( const bool newRole );
static void libraryHandoverAbort( void );


const handover_interface library_handover_if =  {
        &libraryVeto,
        &libraryMarshal,
        &libraryUnMarshal,
        &libraryHandoverCommit,
        &libraryHandoverComplete,
        &libraryHandoverAbort};


* library.h - main library header file
*
extern const handover_interface library_handover_if;

@{

*/


#ifndef HANDOVER_IF_H_
#define HANDOVER_IF_H_

#include <bdaddr_.h>
#include <library.h>
#include <message.h>


/*!
    \brief Module veto the Handover process

    Each module has a veto option over the handover process. Prior
    to handover commencing each module's veto function is called and
    it should check its internal state to determine if the
    handover should proceed.

    The veto check applies to all links

    \return TRUE if the module wishes to veto the handover attempt.

*/
typedef bool (*handover_veto)( void );

/*!
    \brief Marshal the data associated with the specified connection

    \param tp_bd_addr Bluetooth address of the link to be marshalled
    \param buf Address to which the marshaller will write the marshalled byte
           stream.
    \param length space in the marshal byte stream buffer.
    \param[out] written number of bytes written to the buffer.
    \return TRUE if module marshalling complete, otherwise FALSE

*/
typedef bool (*handover_marshal)(const tp_bdaddr *tp_bd_addr,
                                 uint8 *buf,
                                 uint16 length,
                                 uint16 *written);



/*!
    \brief Unmarshal the data associated with the specified connection

    \param tp_bd_addr Bluetooth address of the link to be unmarshalled
    \param buf Address of the byte stream to be unmarshalled.
    \param length amount of data in the marshal byte stream buffer.
    \param[out] consumed the number of bytes written to the buffer
    \return TRUE if module unmarshalling complete, otherwise FALSE

*/
typedef bool (*handover_unmarshal)(const tp_bdaddr *tp_bd_addr,
                                  const uint8 *buf,
                                  uint16 length,
                                  uint16 *consumed);

/*!
    \brief Module performs time-critical actions to commit to the specified role.

    This function would be invoked once for each connected device.
    The library should perform time-critical actions to commit to the new role.

    \param tp_bd_addr Bluetooth address of the connected device.
    \param is_primary TRUE if TWS primary role requested, else
                      secondary

    \return void

*/
typedef void (*handover_commit)(const tp_bdaddr *tp_bd_addr, const bool is_primary);

/*!
    \brief Module performs pending actions and completes the transition to 
    the specified new role.

    This function will be invoked only once during the handover procedure.
    The library should perform pending actions and transition to the new role.

    \param is_primary TRUE if TWS primary role requested, else
                      secondary

    \return void

*/
typedef void (*handover_complete)( const bool is_primary );

/*!
    \brief Abort the Handover process

    Module should abort the handover process and free any memory
    associated with the marshalling process.

    The abort operation applies to all connections being marshalled
    (i.e. the abort operation is not per-device)

    \return void

*/
typedef void (*handover_abort)( void );

/*!
    @brief Structure of handover interface function pointers

    Each library module should expose a const version of this
    struct in it's main header file.  This negates the need to
    expose each of the individual handover interface functions.

*/
typedef struct
{
    handover_veto       pFnVeto;        /*!< Pointer to the module's handover_veto function */
    handover_marshal    pFnMarshal;     /*!< Pointer to the module's handover_marshal function */
    handover_unmarshal  pFnUnmarshal;   /*!< Pointer to the module's handover_unmarshal function */
    handover_commit     pFnCommit;      /*!< Pointer to the module's handover_commit function */
    handover_complete   pFnComplete;    /*!< Pointer to the module's handover_complete function */
    handover_abort      pFnAbort;       /*!< Pointer to the module's handover_abort function */
} handover_interface;

#endif /* HANDOVER_IF_H_ */
