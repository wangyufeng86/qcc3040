/*!
\copyright  Copyright (c) 2019-2020 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file       earbud_handover.c
\brief      This module implements handover interface (handover_interface_t) and
            acts as an aggregator for all application components which require
            handover.
*/
#ifdef INCLUDE_MIRRORING
#include "earbud_handover.h"
#include "earbud_handover_marshal_typedef.h"
#include "earbud_sm_handover.h"

#include <app_handover_if.h>
#include <handover_if.h>
#include <handover_profile.h>
#include <domain_marshal_types.h>
#include <service_marshal_types.h>
#include <marshal_common.h>
#include <mirror_profile_protected.h>
#include <avrcp.h>
#include <a2dp.h>
#include <hfp.h>
#include "kymera.h"
#include <bdaddr_.h>
#include <panic.h>
#include <marshal.h>
#include <logging.h>
#include <stdlib.h>

#define EB_HANDOVER_DEBUG_VERBOSE_LOG DEBUG_LOG_VERBOSE

/* All the marshal type descriptors to be used by Apps P1 (Earbud application)
   marshalling.
   
   Formed as a hierarchy of marshal type descriptors following the layered
   application framework:-
     - Common marshal types
     - Domain marshal types
     - Service marshal types
     - [No Topology layer types exist]
     - Application marshal types
 */
#define EXPAND_AS_TYPE_DEFINITION(type) (const marshal_type_descriptor_t *)&marshal_type_descriptor_##type,
const marshal_type_descriptor_t * const  mtd_handover_app[] = {
    MARSHAL_COMMON_TYPES_TABLE(EXPAND_AS_TYPE_DEFINITION)
    AV_MARSHAL_TYPES_TABLE(EXPAND_AS_TYPE_DEFINITION)
    A2DP_MARSHAL_TYPES_TABLE(EXPAND_AS_TYPE_DEFINITION)
    AVRCP_MARSHAL_TYPES_TABLE(EXPAND_AS_TYPE_DEFINITION)
    CONNECTION_MANAGER_LIST_MARSHAL_TYPES_TABLE(EXPAND_AS_TYPE_DEFINITION)
    HFP_PROFILE_MARSHAL_TYPES_TABLE(EXPAND_AS_TYPE_DEFINITION)
    BT_DEVICE_MARSHAL_TYPES_TABLE(EXPAND_AS_TYPE_DEFINITION)
    BT_DEVICE_HANDOVER_MARSHAL_TYPES_TABLE(EXPAND_AS_TYPE_DEFINITION)
    STATE_PROXY_MARSHAL_TYPES_TABLE(EXPAND_AS_TYPE_DEFINITION)
    EARBUD_HANDOVER_MARSHAL_TYPES_TABLE(EXPAND_AS_TYPE_DEFINITION)
};

/******************************************************************************
 * Local Function Prototypes
 ******************************************************************************/

/* handover interface static function declarations */
static bool earbudHandover_Veto(void);
static bool earbudHandover_Marshal(const tp_bdaddr *addr, uint8 *buffer, uint16 buffer_size, uint16 *written);
static bool earbudHandover_Unmarshal(const tp_bdaddr *addr, const uint8 *buffer, uint16 buffer_size, uint16 *consumed);
static void earbudHandover_Commit(const tp_bdaddr *tp_bd_addr, const bool role);
static void earbudHandover_Complete(const bool role);
static void earbudHandover_Abort(void);
static const registered_handover_interface_t * getNextInterface(void);

/******************************************************************************
 * Local Structure Definitions
 ******************************************************************************/
/*! \brief Stores unmarshalled data received from Primary earbud */
typedef struct {
    /* Pointer to unmarshalled data */
    void * data;
    /* Marshal Type of unmarshalled data */
    uint8 type;
    /* Result of unmarshalling */
    app_unmarshal_status_t unmarshalling_status;
} handover_app_unmarshal_data_t;

/*! \brief Handover context maintains the handover state for the application */
typedef struct {
    /* Marshaling State */
    enum {
        MARSHAL_STATE_UNINITIALIZED,
        MARSHAL_STATE_INITIALIZED,
        MARSHAL_STATE_MARSHALING,
        MARSHAL_STATE_UNMARSHALING
    } marshal_state;

    /* Marshaller/Unmarshaller instance */
    union {
        marshaller_t marshaller;
        unmarshaller_t unmarshaller;
    } marshal;

    /* List of handover interfaces registered by application components */
    const registered_handover_interface_t *interfaces;
    /* Size of registered handover interface list */
    uint8 interfaces_len;
    /* Pointer to the interface currently in use for marshaling or unmarshaling */
    const registered_handover_interface_t *curr_interface;
    /* Index of current marshal type undergoing marshalling */
    const uint8 *curr_type;
    /* List of Unmarshalled objects received from Primary earbud */
    handover_app_unmarshal_data_t * unmarshal_data_list;
    /* Index to the next free entry in unmarshalled data list */
    uint8 unmarshal_data_list_free_index;
    /* Size of unmarshal data list. This can be less than interfaces_len since not all clients will marshal data */
    uint8 unmarshal_data_list_size;
    /* Device being handed-over */
    tp_bdaddr tp_bd_addr;
} handover_app_context_t;

/******************************************************************************
 * Local Declarations
 ******************************************************************************/

/* Handover interface */
const handover_interface application_handover_interface =
{
    &earbudHandover_Veto,
    &earbudHandover_Marshal,
    &earbudHandover_Unmarshal,
    &earbudHandover_Commit,
    &earbudHandover_Complete,    
    &earbudHandover_Abort
};

/* Handover interfaces for all P1 components */
const handover_interface * p1_ho_interfaces[] = {
    &connection_handover_if,
    &a2dp_handover_if,
    &avrcp_handover,
    &hfp_handover_if,
    &application_handover_interface,
    &mirror_handover_if,
    &kymera_a2dp_mirror_handover_if
};

/* Application context instance */
static handover_app_context_t handover_app;
#define earbudHandover_Get() (&handover_app)

/******************************************************************************
 * Local Function Definitions
 ******************************************************************************/
/*! \brief Deinitilaize list of unmarshalled data */
static void deinitializeUnmarshalDataList(void)
{
    handover_app_context_t * app_data = earbudHandover_Get();
    uint8 index;

    /* Initialize list data to NULL */
    for (index = 0; 
         index < app_data->unmarshal_data_list_free_index; 
         index++)
    {
        /* If we have null data, free_index is probably incorrect. Panic!! */
        PanicNull(app_data->unmarshal_data_list[index].data);

        if(app_data->unmarshal_data_list[index].unmarshalling_status != UNMARSHAL_SUCCESS_DONT_FREE_OBJECT)
        {            
            free(app_data->unmarshal_data_list[index].data);
        }
    }

    app_data->unmarshal_data_list_free_index = 0;
    free(app_data->unmarshal_data_list);    
    app_data->unmarshal_data_list = NULL;
}

/*! \brief Create and initialize list of unmarshalled data
    \note Size of list is equal to the number of clients which have data to be marshalled.
    List is expected to be initialized during Unmarshalling and Deinitialized on
    Commit or Abort.    
*/
static void initializeUnmarshalDataList(void)
{
    handover_app_context_t * app_data = earbudHandover_Get();
    uint8 number_of_marshal_types = 0;
    const registered_handover_interface_t * curr_inf;

    /* Get number of interfaces which have data to be marshalled */
    for(curr_inf = app_data->interfaces;
        curr_inf < (app_data->interfaces + app_data->interfaces_len);
        curr_inf++)
    {
        /* We only need interfaces which have valid Marshal Types. */
        if(curr_inf->type_list)
            number_of_marshal_types += curr_inf->type_list->list_size;
    }

    app_data->unmarshal_data_list = (handover_app_unmarshal_data_t *)PanicNull(calloc(number_of_marshal_types, sizeof(handover_app_unmarshal_data_t)));
    app_data->unmarshal_data_list_size = number_of_marshal_types;
    app_data->unmarshal_data_list_free_index = 0;
}

/*! \brief Find the registered interface which handles a given marshal type.
    Only used during unmarshaling procedure.
    \param[in] type Marshal type for which handover interface is to be searched.
    \returns Pointer to handover interface (if found), or NULL.
             Refer \ref registered_handover_interface_t.
*/
static const registered_handover_interface_t * earbudHandover_GetInterfaceForType(uint8 type)
{
    handover_app_context_t * app_data = earbudHandover_Get();
    const registered_handover_interface_t * curr_inf = NULL;
    /* For all registered interfaces */
    for(curr_inf = app_data->interfaces;
        curr_inf && (curr_inf < (app_data->interfaces + app_data->interfaces_len));
        curr_inf++)
    {
        if (curr_inf->type_list)
        {
            const uint8 *curr_type;
            /* For all types in current interface's type_list */
            for(curr_type = curr_inf->type_list->types;
                curr_type < (curr_inf->type_list->types + curr_inf->type_list->list_size);
                curr_type++)
            {
                if(*curr_type == type)
                {
                    return curr_inf;
                }
            }
        }
    }

    return NULL;
}

/*! \brief Destroy marshaller/unmarshaller instances, if any. */
static void earbudHandover_DeinitializeMarshaller(void)
{
    handover_app_context_t * app_data = earbudHandover_Get();
    EB_HANDOVER_DEBUG_VERBOSE_LOG("earbudHandover_DeinitializeMarshaller");
    if(app_data->marshal_state == MARSHAL_STATE_MARSHALING)
    {
        if(app_data->marshal.marshaller)
        {
            MarshalDestroy(app_data->marshal.marshaller, FALSE);
        }
        app_data->marshal.unmarshaller = NULL;
    }

    if(app_data->marshal_state == MARSHAL_STATE_UNMARSHALING)
    {
        if(app_data->marshal.unmarshaller)
        {
            UnmarshalDestroy(app_data->marshal.unmarshaller, FALSE);
        }
        app_data->marshal.unmarshaller = NULL;
        
        /* Remove unmarshalling data */
        deinitializeUnmarshalDataList();
    }

    app_data->marshal_state = MARSHAL_STATE_INITIALIZED;
}

/*! \brief Perform veto on all registered components.
    \returns TRUE if any registerd component vetos, FALSE otherwise.
*/
static bool earbudHandover_Veto(void)
{
    handover_app_context_t * app_data = earbudHandover_Get();
    const registered_handover_interface_t *curr_inf = NULL;
    bool veto = FALSE;


    for(curr_inf = app_data->interfaces;
        (veto == FALSE) && (curr_inf < (app_data->interfaces + app_data->interfaces_len));
        curr_inf++)
    {
        veto = curr_inf->Veto();
    }

    DEBUG_LOG_INFO("earbudHandover_Veto result %d", veto);

    return veto;
}

/*Get the next registered interface which has non Null Marshal TypeList*/
static const registered_handover_interface_t * getNextInterface(void)
{
    handover_app_context_t * app_data = earbudHandover_Get();
    const registered_handover_interface_t *curr_inf = NULL;
    const registered_handover_interface_t *start_inf = app_data->interfaces;

    if(app_data->curr_interface)
    {
        /*Start from next to current interface*/
        start_inf = app_data->curr_interface + 1;
    }


    for(curr_inf = start_inf;
        curr_inf < (app_data->interfaces + app_data->interfaces_len);
        curr_inf++)
    {
        /*We only need interfaces which have valid Marshal Types.*/
        if(curr_inf->type_list)
            return curr_inf;
    }

    return NULL;
}

/*! \brief Commit the roles on all registered components.
    \param[in] tp_bd_addr Bluetooth address of the connected device / handset.
    \param[in] role TRUE if primary, FALSE otherwise
*/
static void earbudHandover_Commit(const tp_bdaddr *tp_bd_addr, const bool role)
{
    earbudHandover_Get()->tp_bd_addr = *tp_bd_addr;
    UNUSED(role);
}

static void earbudHandover_ClientUnmarshal(const tp_bdaddr *tp_bd_addr)
{
    handover_app_context_t * app_data = earbudHandover_Get();
    const registered_handover_interface_t * curr_inf;
    handover_app_unmarshal_data_t * unmarshal_data;
    app_unmarshal_status_t result;
    uint8 index;

    PanicFalse(app_data->marshal_state == MARSHAL_STATE_UNMARSHALING);
    for (index = 0;
        index < app_data->unmarshal_data_list_free_index;
        index++)
    {
        /* Call unmarshal and store the result */
        unmarshal_data = &app_data->unmarshal_data_list[index];
        curr_inf = earbudHandover_GetInterfaceForType(unmarshal_data->type);
        result = curr_inf->Unmarshal(&tp_bd_addr->taddr.addr, unmarshal_data->type, unmarshal_data->data);
        PanicFalse(result > UNMARSHAL_FAILURE);
        app_data->unmarshal_data_list[index].unmarshalling_status = result;
        EB_HANDOVER_DEBUG_VERBOSE_LOG("earbud Handover Client update complete for type: %d", unmarshal_data->type);
    }
}

/*! \brief Commit the roles on all registered components.
    \param[in] primary TRUE if primary, FALSE otherwise
*/
static void earbudHandover_Complete(const bool primary)
{
    handover_app_context_t * app_data = earbudHandover_Get();
    DEBUG_LOG_INFO("earbudHandover_Complete");

    if (primary)
    {
        earbudHandover_ClientUnmarshal(&app_data->tp_bd_addr);
    }

    const const registered_handover_interface_t *curr_inf = app_data->interfaces;
    for(curr_inf = app_data->interfaces;
        curr_inf && (curr_inf < (app_data->interfaces + app_data->interfaces_len));
        curr_inf++)
    {
        curr_inf->Commit(primary);
    }

    if (primary)
    {
        appLinkPolicyForceUpdatePowerTable(&app_data->tp_bd_addr.taddr.addr);
    }

    /* Complete is the final interface invoked during handover. Cleanup now. */
    earbudHandover_DeinitializeMarshaller();
}

/*! \brief Handover application's marshaling interface.

    \note Possible cases:
    1. written is not incremented and return value is FALSE: this means buffer is insufficient.
    2. written < buffer_size and return value is FALSE. This means buffer is insufficient.
    3. written <= buffer_size and return value is TRUE. This mean marshaling is complete.
    4. written is not incremented and return value is TRUE. This means marshaling not required.

    \param[in] addr address of handset.
    \param[in] buffer input buffer with data to be marshalled.
    \param[in] buffer_size size of input buffer.
    \param[out] written number of bytes consumed during marshalling.
    \returns TRUE, if marshal types from all registered interfaces have been marshalled
                  successfully. In this case we destory the marshaling instance and set
                  the state back to MARSHAL_STATE_INITIALIZED.
             FALSE, if the buffer is insufficient for marshalling.
 */
static bool earbudHandover_Marshal(const tp_bdaddr *addr, uint8 *buffer, uint16 buffer_size, uint16 *written)
{
    handover_app_context_t * app_data = earbudHandover_Get();
    bool marshalled = TRUE;
    void *data = NULL;

    PanicFalse(addr);
    PanicFalse(buffer);
    PanicFalse(written);
    PanicFalse(app_data->marshal_state == MARSHAL_STATE_INITIALIZED ||
               app_data->marshal_state == MARSHAL_STATE_MARSHALING);

    EB_HANDOVER_DEBUG_VERBOSE_LOG("earbudHandover_Marshal");
    /* Create marshaller if not done yet */
    if(app_data->marshal_state == MARSHAL_STATE_INITIALIZED)
    {
        app_data->curr_interface = NULL;
        app_data->curr_interface = getNextInterface();
        if(app_data->curr_interface)
        {
            app_data->marshal_state = MARSHAL_STATE_MARSHALING;
            app_data->marshal.marshaller = MarshalInit(mtd_handover_app, NUMBER_OF_EARBUD_APP_MARSHAL_OBJECT_TYPES);
            PanicFalse(app_data->marshal.marshaller);
            app_data->curr_type = app_data->curr_interface->type_list->types;
        }
        else
        {
            /* No marshal types registered, return success */
            return TRUE;
        }
    }

    MarshalSetBuffer(app_data->marshal.marshaller, buffer, buffer_size);
    /* For all remaining registered interfaces. */
    while(marshalled && (app_data->curr_interface))
    {
        /* For all remaining types in current interface. */
        while(app_data->curr_type < (app_data->curr_interface->type_list->types + app_data->curr_interface->type_list->list_size))
        {
            if(app_data->curr_interface->Marshal(&addr->taddr.addr, *app_data->curr_type, &data))
            {
                if(Marshal(app_data->marshal.marshaller, data, *app_data->curr_type))
                {
                    EB_HANDOVER_DEBUG_VERBOSE_LOG("earbudHandover_Marshal - Marshalling successfull for type: %d", *app_data->curr_type);
                }
                else
                {
                    /* Insufficient buffer for marshalling. */
                    DEBUG_LOG_WARN("earbudHandover_Marshal - Insufficient buffer for type: %d!", *app_data->curr_type);
                    marshalled = FALSE;
                    break;
                }
            }
            else
            {
                /* Nothing to marshal for this type. Continue ahead. */
            }

            /* Move to next Marshal type for current interface */
            app_data->curr_type++;
        }

        if(marshalled)
        {
            app_data->curr_interface = getNextInterface();
            if (app_data->curr_interface)
            {
                /* All types for current interface marshalled.
                 * Re-initialize app_data->curr_type, from next interface.
                 */
                app_data->curr_type = app_data->curr_interface->type_list->types;
            }
        }
    }

    *written = MarshalProduced(app_data->marshal.marshaller);
    return marshalled;
}

/*! \brief Handover application's unmarshaling interface.

    \note Possible cases,
    1. consumed is not incremented and return value is FALSE. This means buffer is
    insufficient to unmarshal.
    2. consumed < buffer_size and return value is FALSE. Need more data from caller.
    3. consumed == buffer_size and return value is TRUE. This means all data
      unmarshalled successfully. There could still be more data with caller
      in which case this function is invoked again.

    \param[in] addr address of handset.
    \param[in] buffer input buffer with data to be unmarshalled.
    \param[in] buffer_size size of input buffer.
    \param[out] consumed number of bytes consumed during unmarshalling.
    \returns TRUE, if all types have been successfully unmarshalled.
             FALSE, if buffer is insufficient for unmarshalling.
 */
static bool earbudHandover_Unmarshal(const tp_bdaddr *addr, const uint8 *buffer, uint16 buffer_size, uint16 *consumed)
{
    handover_app_context_t * app_data = earbudHandover_Get();
    bool unmarshalled = TRUE;
    handover_app_unmarshal_data_t * data = NULL;
    
    UNUSED(addr);

    PanicFalse(buffer);
    PanicFalse(buffer_size);
    PanicFalse(consumed);
    PanicFalse(app_data->marshal_state == MARSHAL_STATE_INITIALIZED ||
               app_data->marshal_state == MARSHAL_STATE_UNMARSHALING);

    EB_HANDOVER_DEBUG_VERBOSE_LOG("earbudHandover_Unmarshal");
    /* Create unmarshaller on first call */
    if(app_data->marshal_state == MARSHAL_STATE_INITIALIZED)
    {
        /* Unexpected unmarshal call, as no type handlers exist */
        PanicFalse(getNextInterface());

        app_data->marshal_state = MARSHAL_STATE_UNMARSHALING;
        app_data->marshal.unmarshaller = UnmarshalInit(mtd_handover_app, NUMBER_OF_EARBUD_APP_MARSHAL_OBJECT_TYPES);
        PanicNull(app_data->marshal.unmarshaller);
        
        /*Initialize list of unmarshalled data*/
        initializeUnmarshalDataList();
    }

    UnmarshalSetBuffer(app_data->marshal.unmarshaller, buffer, buffer_size);

    /* Loop until all types are extracted from buffer */
    while(unmarshalled && (*consumed < buffer_size))
    {
        data = &app_data->unmarshal_data_list[app_data->unmarshal_data_list_free_index];
        PanicFalse(app_data->unmarshal_data_list_free_index < app_data->unmarshal_data_list_size);

        if(Unmarshal(app_data->marshal.unmarshaller, &data->data, &data->type))
        {
            const registered_handover_interface_t * curr_inf = earbudHandover_GetInterfaceForType(data->type);
            /* Could not find interface for marshal_type */
            if(curr_inf == NULL)
            {
                DEBUG_LOG_ERROR("earbudHandover_Unmarshal - Could not find interface for type: %d", data->type);
                Panic();
            }
                       
            app_data->unmarshal_data_list_free_index++;
            *consumed = UnmarshalConsumed(app_data->marshal.unmarshaller);
            EB_HANDOVER_DEBUG_VERBOSE_LOG("earbudHandover_Unmarshal - Unmarshalling successfull for type: %d, Index: %d, Consumed: %d", 
                                          data->type,
                                          app_data->unmarshal_data_list_free_index - 1,
                                          *consumed);
        }
        else
        {
            /* No types found. Buffer has incomplete data, ask for remaining. */
            DEBUG_LOG_WARN("earbudHandover_Unmarshal - Incomplete data for unmarshaling!");
            unmarshalled = FALSE;
        }
    }

    return unmarshalled;
}

/*! \brief Handover application's Abort interface
 *
 *  Cleans up the marshaller instance.
 */
static void earbudHandover_Abort(void)
{
    earbudHandover_DeinitializeMarshaller();
}

bool EarbudHandover_Init(Task init_task)
{
    UNUSED(init_task);
    handover_app_context_t * app_data = earbudHandover_Get();
    unsigned handover_registrations_array_dim;
    EB_HANDOVER_DEBUG_VERBOSE_LOG("EarbudHandover_Init");

    memset(app_data, 0, sizeof(*app_data));

    /* Register handover interfaces with earbud handover application. */
    handover_registrations_array_dim = (unsigned)handover_interface_registrations_end -
                                       (unsigned)handover_interface_registrations_begin;
    PanicFalse((handover_registrations_array_dim % sizeof(registered_handover_interface_t)) == 0);
    handover_registrations_array_dim /= sizeof(registered_handover_interface_t);

    if(handover_registrations_array_dim)
    {
        app_data->interfaces = handover_interface_registrations_begin;
        app_data->interfaces_len = handover_registrations_array_dim;
    }

    /* Register handover interfaces with profile. We Panic if registration fails as handover will
     * not be possible without registration. Registration could only fail if handover profile is not
     * initialized prior to registration.
     */
    PanicFalse(HandoverProfile_RegisterHandoverClients(p1_ho_interfaces, sizeof(p1_ho_interfaces)/sizeof(p1_ho_interfaces[0])));
    app_data->marshal_state = MARSHAL_STATE_INITIALIZED;
    return TRUE;
}
#endif
