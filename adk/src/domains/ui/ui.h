/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\defgroup   ui_domain UI
\ingroup    domains
\brief      UI domain/component

    A note on terminology:

    Concept             | Definition
    ------------------- | -------------
    Logical Input       | A Logical Input is defined as a physical signal being asserted for a specific duration. E.g. PIO 1 is asserted for 1 second could be Logical Input 1, PIO 1 asserted for 3 seconds could then be Logical Input 2.
    UI Event            | A UI Event is an internal event representing a transient indication that needs to be provided to the User in a one-shot fashion. UI Events are raised by UI Indicators when some audio or visual indication is configured to occur when a specified system event is observed. For example: consider the case where the battery voltage goes below some configured threshold, this could result in an audio prompt indication to the user; "battery low".
    UI Input            | The UI Input is an internal event that may get generated following an occurrence of a logical input. Some example UI Inputs: "End Call", "Power On", "Volume Up". Where a UI Input is raised following a Logical Input, only one UI Input can be raised. I.e. there is a 1:1 relationship between Logical Inputs and UI Inputs, not a 1:N relationship. This is to simplify use cases and to prevent unintended consequences when configuring the Application UI.
    UI Provider         | A UI Provider is a component that provides contextual information about the state of the product that the user needs in order to be able to operate it. This contextual information is a vital part of the complete UI of the device. Example UI Providers are the Media Player and Telephony services.
    UI Provider Context | This is the actual contextual/state information that a specific UI Provider issues. E.g. for the Telephony service, possible contexts would be "Ringing", "In Call" etc.
    UI Indicator        | A UI Indicator is some part of the system that indicates status or feedback information to the User. For example this could be a Display, LEDs or Audio indications such as Tones and Prompts.

    These APIs are used by the Application, alongside UI Providers, UI Input
    Consumers and UI Provider Context Consumers to register/unregister themselves
    with the UI component and to handling incoming Logical Inputs.

    This module raises UI Inputs in response to Logical Input stimuli, according to
    the UI configuration. These UI Inputs are forwarded to all interested UI Input
    Consumers.
*/

#ifndef UI_H_
#define UI_H_

#include <message.h>

#include "domain_message.h"
#include "ui_inputs.h"
#include <rtime.h>

#define BAD_CONTEXT 0xFF

/*\{*/

/*! External messages */
typedef enum
{
    UI_PROVIDER_CONTEXT_UPDATED=UI_MESSAGE_BASE,
} ui_message_t;

typedef enum
{
    ui_indication_type_audio_prompt,
    ui_indication_type_audio_tone,
    ui_indication_type_led
} ui_indication_type_t;

/*! \brief UI configuration table row instance structure */
typedef struct
{
    unsigned logical_input;
    ui_providers_t ui_provider_id;
    unsigned ui_provider_context;
    ui_input_t ui_input;

} ui_config_table_content_t;

/*! \brief UI Providers Context callback */
typedef unsigned (*ui_provider_context_callback_t)(void);

/*! \brief UI Providers Context Consumer callback */
typedef Task (*ui_provider_ctxt_consumer_task_t)(void);

/*! \brief UI Input Consumer callback */
typedef Task (*ui_input_consumer_task_t)(void);

/*! \brief Inject UI Input callback */
typedef void (*inject_ui_input)(ui_input_t ui_input, uint32 delay);

/*! \brief Sniff UI Events received by UI */
typedef uint32 (*sniff_ui_event)(ui_indication_type_t indication_type, uint16 indication_index, uint32 time_to_play);

typedef struct
{
    ui_providers_t provider;
    unsigned context;
} UI_PROVIDER_CONTEXT_UPDATED_T;


/*! \brief Provides the UI Task to other components

    \return     Task - ui task.
*/
Task Ui_GetUiTask(void);

/*! \brief Register a UI Provider get context callback.

    This API is called by the various UI Providers in the system to register their
    get context callbacks with UI module.

    These get context callbacks are used by the UI module to query a UI Providers
    current context whenever a Logical Input to the system is detected. The UI module
    looks up the Logical Input and the UI Provider Context in its configuration table
    and determines if a UI Input should be raised. The UI Input can then be issued to
    any interested UI Input Consumers.

    \param ui_provider - The UI Provider which is registering its get context callback.
    \param ui_provider_context_callback - A function pointer to the get context callback.
*/
void Ui_RegisterUiProvider(ui_providers_t ui_provider, ui_provider_context_callback_t ui_provider_context_callback);

/*! \brief Unregister all UI Provider get context callbacks.

    This API is called by the Application to unregister all the UI Provider get
    context callbacks registered with UI module. This allows the memory associated
    with each get context callback, held by the UI component, to be freed.
*/
void Ui_UnregisterUiProviders(void);

/*! \brief Register a module as a UI Input Consumer

    This API is called by modules who wish to register as UI Input Consumers. The module
    provides its Task handler and a list of the UI Inputs Groups it is interested in.

    Once a module has registered with the UI component, if a UI Input is generated
    that belongs to a UI Input Message Group that the module is interested in, the
    UI Input will be sent to the Task handler the module registered using this API.

    \param ui_input_consumer_task - Task handler which will receive UI Input messages
    \param msg_groups_of_interest - A list of UI Input message groups in which the module is interested.
    \param num_msg_groups - The size of the modules interested message groups list.
*/
void Ui_RegisterUiInputConsumer(Task ui_input_consumer_task,
                                const message_group_t * msg_groups_of_interest,
                                unsigned num_msg_groups);

/*! \brief Register a UI Input Consumer for a single UI Input message group.

    This API is called by modules who wish to register as UI Input Consumers. The
    specified Task shall receive UI Inputs messages for the subscribed UI Input
    message group.

    \note This API is similar to Ui_RegisterUiInputConsumer, but for just a single
          message group. It is typically used in conjunction with the Message Broker
          component.

    \param task - Task handler which will receive UI Input messages
    \param group - The UI input message group of interest.
*/
void Ui_RegisterUiInputsMessageGroup(Task task, message_group_t group);

/*! \brief Register a module as a UI Provider Context Consumer

    This API is called by modules who want to subscribe for notifications about
    changes to a particular UI Providers context.

    The subscribing module provides its Task handler and the UI Provider it is
    interested in. Whenever the context of that UI Provider changes, a notification
    shall be sent to the registered Task handler.

    \param ui_provider - Id of the UI Provider in which context change notifications
                         are to be subscribed
    \param ui_provider_ctxt_consumer_task - The Task handler which will receive
                                            context change notifications.
*/
void Ui_RegisterContextConsumers(ui_providers_t ui_provider,
                                 Task ui_provider_ctxt_consumer_task);

/*! \brief Unregister all UI Provider Context Consumers

    This API is called by the Application to unregister all the UI Provider Context
    Consumers registered with UI module. This allows the memory associated with each
    registration, held by the UI component, to be freed.
*/
void Ui_UnregisterContextConsumers(void);

/*! \brief Notify that a UI Provider context has changed.

    This API is called by UI Provider modules to inform the UI component that its
    context has changed.

    \param ui_provider - Id of the UI Provider whose context has changed.
    \param latest_ctxt - latest context of the UI Provider.
*/
void Ui_InformContextChange(ui_providers_t ui_provider, unsigned latest_ctxt);

/*! \brief Inject a UI Input into the CAA framework.

    This API is used to inject a UI Input into the system. The UI Input will
    be raised as if a Logical Input had been detected and a matching UI Provider
    context had been found.

    \param ui_input - The UI Input to be injected
*/
void Ui_InjectUiInput(ui_input_t ui_input);

/*! \brief Inject a UI Input to the system, after a specified delay period.

    This API is used to inject a UI Input into the system, after the specified
    delay period.

    The UI Input will be raised as if a Logical Input had been detected and a
    matching UI Provider context had been found.

    The delay period may be required to support use cases where an event needs
    to be sent to a peer device and handled at the same time on both peers.
    Configuring the delay period allows the event to be raised at the same
    instant on both peers.

    \param ui_input - The UI Input to be injected
    \param delay - The delay in milliseconds before the message will be sent.
                   D_IMMEDIATE should be used to send immediately, not 0.
*/
void Ui_InjectUiInputWithDelay(ui_input_t ui_input, uint32 delay);

/*! \brief Raise a UI Event at the UI component for an event that needs to be indicated to the user.

    This API is used by UI Indicators to promote UI Events through the system.
    The UI Event can then be forwarded via Peer UI, if that component is present
    in the build. If the UI Event is forwarded, the time to play may be increased
    due to the time needed to propagate the event to the peer - this is the purpose
    of the return type - to provide the ammended time_to_play back to the originating
    UI Indicator.

    \param ui_event_type - the type of UI Event, e.g. audio prompt, audio tone etc
    \param indication_index - the index into the configuration table owned by the originating UI Indicator
    \param time_to_play - the time to play the indication in ms
    \return the updated time_to_play (i.e. factoring for transimssion to a peer device, for example)
*/
rtime_t Ui_RaiseUiEvent(ui_indication_type_t ui_event_type, uint16 indication_index, rtime_t time_to_play);

/*! \brief Notify the UI Component of a received UI Event

    This API is used by the Peer Service to notify UI Events to the local UI system, which have
    been forwarded from the remote peer device.

    \param ui_event_type - the type of UI Event, e.g. audio prompt, audio tone etc
    \param indication_index - the index into the configuration table owned by the originating UI Indicator
    \param time_to_play - the time to play the indication in ms
*/
void Ui_NotifyUiEvent(ui_indication_type_t ui_event_type, uint16 indication_index, rtime_t time_to_play);

/*! \brief Register a sniffer function to receive UI Events

    This API allows a module to register its own function pointer to observe
    UI Events that may be raised by any of the UI indicators in the system.

    One such use case for this is synchronisation of Peer UI Events, such as audio
    prompt playback.

    \param ui_sniff_func - The sniffer function which shall receive UI Events.
*/
void Ui_RegisterUiEventSniffer(sniff_ui_event ui_sniff_func);

/*! \brief Register an interceptor function to receive UI Inputs

    This mechanism allows a module to register its own function pointer to handle
    new UI Inputs being raised (either by the Ui_InjectUiInput() API or by virtue
    of a Logical Input being detected and matching a UI Provider context).

    If no function is registered the default handler will be to send UI Inputs to
    subscribing UI Input Consumers.

    \param ui_intercept_func - The interceptor function which shall receive UI inputs.

    \return The current UI interceptor function.
*/
inject_ui_input Ui_RegisterUiInputsInterceptor(inject_ui_input ui_intercept_func);

/*! \brief Initialise the UI component

    Called during Application start up to initialise the UI component.

    \return TRUE indicating success
*/
bool Ui_Init(Task init_task);

/*! \brief Configure the UI component with the mapping for Logical Inputs to UI Inputs.

    This API is used by the Application to provide the look up table which maps
    Logical Inputs and UI Provider contexts to UI Inputs.

    This configuration data must be provided before Logical Inputs can be processed
    by the UI component.

    \param config_table - The configuration table used by the UI to generate UI Inputs.
    \param config_size - The number of rows in the config_table.
*/
void Ui_SetConfigurationTable(const ui_config_table_content_t* config_table,
                              unsigned config_size);

/*! \brief Get a UI Provider's current context.

    \param ui_provider - The UI Provider which is to provide the context.
    \return the current context of the provider
*/
unsigned Ui_GetUiProviderContext(ui_providers_t ui_provider);

/*\}*/

#endif /* UI_H_ */
