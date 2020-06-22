/****************************************************************************
Copyright (c) 2019 Qualcomm Technologies International, Ltd.


FILE NAME
    app_handover_if.h

DESCRIPTION
    Header file for the Application Handover interface.

*/

/*!
\defgroup twm twm
\ingroup Application

\brief  Exposes interface to Application for relevant components to implement
        it for Handover operation.

\section Handover_Interface_intro INTRODUCTION

        Components that will be marshalled should internally
        implement the interface functions defined below
        for veto, marshal, unmarshal, and commit.
        These are then exposed through the app_handover_interface
        struct

\example Usage example

* A Component that needs to implement the application handover 
* interface should declare and define the necessary functions
* as specified by the function typedefs below. These functions 
* are private to the component and are exposed in the form of 
* app_handover_if only.
*
* Pointers to these functions are then held in an instance of
* the app_handover_if struct.

static bool component_Veto(void);
static bool component_Marshal(const bdaddr *bd_addr,
                         marshal_type_t type,
                         void **marshal_data);
static void component_Unmarshal(const bdaddr *bd_addr,
                         marshal_type_t type,
                         void *marshal_data);
static void component_Commit(bool newRole);


const app_handover_interface_t component_handover_if =  {
        &component_Veto,
        &component_Marshal,
        &component_Unmarshal,
        &component_Commit};

*
extern const app_handover_interface_t component_handover_if;

@{

*/


#ifndef APP_HANDOVER_IF_H_
#define APP_HANDOVER_IF_H_

#include <bdaddr_.h>
#include <hydra_macros.h>
#include <app/marshal/marshal_if.h>

/*! Return codes for unmarshalling interface */
typedef enum {
    /* unmarshalling has failed. */
    UNMARSHAL_FAILURE,
    /* unmarshalling is successfull and object can be freed. */
    UNMARSHAL_SUCCESS_FREE_OBJECT,
    /* unmarshalling is successfull and object is in use. Don't free the object. */
    UNMARSHAL_SUCCESS_DONT_FREE_OBJECT,
}app_unmarshal_status_t;

/*!
    \brief Component veto the Handover process

    Each component has a veto option over the handover process. Prior
    to handover commencing each component's veto function is called and
    it should check its internal state to determine if the
    handover should proceed.

    The veto check applies to all links

    \return TRUE if the component wishes to veto the handover attempt.

*/
typedef bool (*app_handover_veto_t)(void);

/*!
    \brief The function shall set marshal_obj to the address of the object to 
           be marshalled.

    \param[in] bd_addr      Bluetooth address of the link to be marshalled
                            \ref bdaddr
    \param[in] type         Type of the data to be marshalled \ref marshal_type_t
    \param[out] marshal_obj Holds address of data to be marshalled.
    \return TRUE: Required data has been copied to the marshal_obj.
            FALSE: No data is required to be marshalled. marshal_obj is set to NULL.

*/
typedef bool (*app_handover_marshal_t)(const bdaddr *bd_addr,
                                 marshal_type_t type,
                                 void **marshal_obj);


/*!
    \brief The function shall copy the unmarshal_obj associated to specific 
            marshal type \ref marshal_type_t

    \param[in] bd_addr      Bluetooth address of the link to be unmarshalled.
                            \ref bdaddr
    \param[in] type         Type of the unmarshalled data \ref marshal_type_t
    \param[in] unmarshal_obj Address of the unmarshalled object.
    \return UNMARSHAL_FAILURE: Unmarshalling failed
            UNMARSHAL_SUCCESS_FREE_OBJECT: Unmarshalling complete. Caller can free the unmarshal_obj.
            UNMARSHAL_SUCCESS_DONT_FREE_OBJECT: Unmarshalling complete. Caller cannot free the 
                unmarshal_obj, as component is using it.

*/
typedef app_unmarshal_status_t (*app_handover_unmarshal_t)(const bdaddr *bd_addr,
                                  marshal_type_t type,
                                  void *unmarshal_obj);

/*!
    \brief Component commits to the specified role

    The component should take any actions necessary to commit to the
    new role.

    \param[in] is_primary TRUE if device role is primary, else secondary

*/
typedef void (*app_handover_commit_t)( bool is_primary );

/*! \brief Structure with list of Marshal types.

    This data is supplied with registered handover interface by components registering
    with handover application.
*/
typedef struct {
    /*! List of marshal types */
    const uint8 *types;
    /*! Size of list */
    uint8 list_size;
} marshal_type_list_t;

/*! \brief Structure with handover interfaces for the application components.

    Components which implement the handover interface provide this data while
    registering with handover application. Then, during a handover trigger, the
    handover application uses the registred interfaces and marshal type list provided
    in this structure to perform Veto, Marshal, Unmarshal and Commit operations.
*/
typedef struct {
    /*! List of Marshal types marshaled/unmarshaled by this interface */
    const marshal_type_list_t *type_list;
    /*! Veto Interface */
    app_handover_veto_t Veto;
    /*! Marshaling Interface */
    app_handover_marshal_t Marshal;
    /*! Unmarshaling Interface */
    app_handover_unmarshal_t Unmarshal;
    /*! Commit Interface */
    app_handover_commit_t Commit;
} registered_handover_interface_t;

/*! Macro to register the handover interface */
#define REGISTER_HANDOVER_INTERFACE(NAME, TYPELIST, VETO, MARSHAL, UNMARSHAL, COMMIT) \
_Pragma("datasection handover_interface_registrations") \
const registered_handover_interface_t handover_interface_##NAME = \
    {TYPELIST, VETO, MARSHAL, UNMARSHAL, COMMIT}

/*! Macro to register the handover interface with no marshalling data */
#define REGISTER_HANDOVER_INTERFACE_NO_MARSHALLING(NAME, VETO, COMMIT) \
REGISTER_HANDOVER_INTERFACE(NAME, NULL, VETO, NULL, NULL, COMMIT)

/*! Linker defined consts referencing the location of the section containing
    the handover interface registrations.
*/
extern const registered_handover_interface_t handover_interface_registrations_begin[];
extern const registered_handover_interface_t handover_interface_registrations_end[];
#endif /* APP_HANDOVER_IF_H_ */
