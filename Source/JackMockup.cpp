#if !USE_JACK

    #include "JackMockup.h"

    #include <JuceHeader.h>

    #include "AudioManager.h"

//==============================================================================
int jack_set_session_callback(jack_client_t * client, JackSessionCallback session_callback, void * arg)
{
    jassertfalse;
    return -1;
}

//==============================================================================
int jack_session_reply(jack_client_t * client, jack_session_event_t * event)
{
    jassertfalse;
    return -1;
}

//==============================================================================
void jack_session_event_free(jack_session_event_t * event)
{
    jassertfalse;
}

//==============================================================================
jack_client_t * jackClientOpen(const char * client_name, jack_options_t options, jack_status_t * status, ...)
{
    jassertfalse;
    return nullptr;
}

//==============================================================================
int jack_client_close(jack_client_t * client)
{
    jassertfalse;
    return -1;
}

//==============================================================================
char * jack_get_client_name(jack_client_t * client)
{
    jassertfalse;
    return nullptr;
}

//==============================================================================
int jack_activate(jack_client_t * client)
{
    jassertfalse;
    return -1;
}

//==============================================================================
int jack_deactivate(jack_client_t * client)
{
    jassertfalse;
    return -1;
}

//==============================================================================
void jack_on_shutdown(jack_client_t * client, JackShutdownCallback function, void * arg)
{
    jassertfalse;
}

//==============================================================================
int jack_set_process_callback(jack_client_t * client, JackProcessCallback process_callback, void * arg)
{
    jassertfalse;
    return -1;
}

//==============================================================================
int jack_set_client_registration_callback(jack_client_t *,
                                          JackClientRegistrationCallback registration_callback,
                                          void * arg)
{
    jassertfalse;
    return -1;
}

//==============================================================================
int jack_set_port_registration_callback(jack_client_t *, JackPortRegistrationCallback registration_callback, void * arg)
{
    jassertfalse;
    return -1;
}

//==============================================================================
int jack_set_port_connect_callback(jack_client_t *, JackPortConnectCallback connect_callback, void * arg)
{
    jassertfalse;
    return -1;
}

//==============================================================================
int jack_set_graph_order_callback(jack_client_t *, JackGraphOrderCallback graph_callback, void *)
{
    jassertfalse;
    return -1;
}

//==============================================================================
int jack_set_xrun_callback(jack_client_t *, JackXRunCallback xrun_callback, void * arg)
{
    jassertfalse;
    return -1;
}

//==============================================================================
jack_nframes_t jack_get_sample_rate(jack_client_t *)
{
    jassertfalse;
    return 0u;
}

//==============================================================================
jack_nframes_t jack_get_buffer_size(jack_client_t *)
{
    jassertfalse;
    return 0u;
}

//==============================================================================
float jack_cpu_load(jack_client_t * client)
{
    jassertfalse;
    return 0.0f;
}

//==============================================================================
jack_port_t * jack_port_register(jack_client_t * client,
                                 const char * port_name,
                                 const char * port_type,
                                 unsigned long flags,
                                 unsigned long buffer_size)
{
    jassertfalse;
    return nullptr;
}

//==============================================================================
int jack_port_unregister(jack_client_t *, jack_port_t *)
{
    jassertfalse;
    return -1;
}

//==============================================================================
void * jack_port_get_buffer(jack_port_t *, jack_nframes_t)
{
    jassertfalse;
    return nullptr;
}

//==============================================================================
const char * jack_port_name(const jack_port_t * port)
{
    jassertfalse;
    return nullptr;
}

//==============================================================================
const char * jack_port_short_name(const jack_port_t * port)
{
    jassertfalse;
    return nullptr;
}

//==============================================================================
int jack_port_connected_to(const jack_port_t * port, const char * port_name)
{
    jassertfalse;
    return -1;
}

//==============================================================================
int jack_connect(jack_client_t *, const char * source_port, const char * destination_port)
{
    jassertfalse;
    return -1;
}

int jack_disconnect(jack_client_t *, const char * source_port, const char * destination_port)
{
    jassertfalse;
    return -1;
}

//==============================================================================
const char **
    jack_get_ports(jack_client_t *, const char * port_name_pattern, const char * type_name_pattern, unsigned long flags)
{
    jassertfalse;
    return nullptr;
}

//==============================================================================
jack_port_t * jack_port_by_name(jack_client_t *, const char * port_name)
{
    jassertfalse;
    return nullptr;
}

//==============================================================================
jack_port_t * jack_port_by_id(jack_client_t * client, jack_port_id_t port_id)
{
    jassertfalse;
    return nullptr;
}

//==============================================================================
void jack_free(void * ptr)
{
    jassertfalse;
}

//==============================================================================
jackctl_server_t * jackctl_server_create(bool (*on_device_acquire)(const char * device_name),
                                         void (*on_device_release)(const char * device_name))
{
    return nullptr;
}

//==============================================================================
void jackctl_server_destroy(jackctl_server_t * server)
{
    jassertfalse;
}

//==============================================================================
bool jackctl_server_start(jackctl_server_t * server, jackctl_driver_t * driver)
{
    jassertfalse;
    return false;
}

//==============================================================================
bool jackctl_server_stop(jackctl_server_t * server)
{
    jassertfalse;
    return false;
}

//==============================================================================
bool jackctl_server_open(jackctl_server_t *, jackctl_driver_t *)
{
    jassertfalse;
    return false;
}

//==============================================================================
void jackctl_server_close(jackctl_server_t *)
{
    jassertfalse;
}

//==============================================================================
const JSList * jackctl_server_get_drivers_list(jackctl_server_t * server)
{
    jassertfalse;
    return nullptr;
}

//==============================================================================
const JSList * jackctl_server_get_parameters(jackctl_server_t * server)
{
    jassertfalse;
    return nullptr;
}

//==============================================================================
const JSList * jackctl_server_get_internals_list(jackctl_server_t * server)
{
    jassertfalse;
    return nullptr;
}

//==============================================================================
bool jackctl_server_load_internal(jackctl_server_t * server, jackctl_internal_t * internal)
{
    jassertfalse;
    return false;
}

//==============================================================================
const char * jackctl_driver_get_name(jackctl_driver_t * driver)
{
    jassertfalse;
    return nullptr;
}

//==============================================================================
const JSList * jackctl_driver_get_parameters(jackctl_driver_t * driver)
{
    jassertfalse;
    return nullptr;
}

//==============================================================================
const char * jackctl_internal_get_name(jackctl_internal_t * internal)
{
    jassertfalse;
    return nullptr;
}

//==============================================================================
const JSList * jackctl_internal_get_parameters(jackctl_internal_t * internal)
{
    jassertfalse;
    return nullptr;
}

//==============================================================================
const char * jackctl_parameter_get_name(jackctl_parameter_t * parameter)
{
    jassertfalse;
    return nullptr;
}

//==============================================================================
const char * jackctl_parameter_get_short_description(jackctl_parameter_t * parameter)
{
    jassertfalse;
    return nullptr;
}

//==============================================================================
const char * jackctl_parameter_get_long_description(jackctl_parameter_t * parameter)
{
    jassertfalse;
    return nullptr;
}

//==============================================================================
jackctl_param_type_t jackctl_parameter_get_type(jackctl_parameter_t * parameter)
{
    jassertfalse;
    return {};
}

//==============================================================================
char jackctl_parameter_get_id(jackctl_parameter_t * parameter)
{
    jassertfalse;
    return {};
}

//==============================================================================
bool jackctl_parameter_set_value(jackctl_parameter_t * parameter, const jackctl_parameter_value * value_ptr)
{
    jassertfalse;
    return false;
}

//==============================================================================
jackctl_parameter_value jackctl_parameter_get_default_value(jackctl_parameter_t * parameter)
{
    jassertfalse;
    return {};
}

//==============================================================================
JSList const * jack_slist_next(JSList const *)
{
    jassertfalse;
    return nullptr;
}

#endif