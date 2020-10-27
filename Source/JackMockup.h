#pragma once

#include <cstdint>

#ifndef __APPLE__
using pthread_t = int;
using sigset_t = int;
#endif

// macros
#define JACK_DEFAULT_AUDIO_TYPE "32 bit float mono audio"
#define JACK_PARAM_STRING_MAX 127
#define JACK_PARAM_MAX (JackParamBool + 1)

// enums
enum JackOptions {
    JackNullOption = 0x00,
    JackNoStartServer = 0x01,
    JackUseExactName = 0x02,
    JackServerName = 0x04,
    JackLoadName = 0x08,
    JackLoadInit = 0x10,
    JackSessionID = 0x20
};

enum JackStatus {
    JackFailure = 0x01,
    JackInvalidOption = 0x02,
    JackNameNotUnique = 0x04,
    JackServerStarted = 0x08,
    JackServerFailed = 0x10,
    JackServerError = 0x20,
    JackNoSuchClient = 0x40,
    JackLoadFailure = 0x80,
    JackInitFailure = 0x100,
    JackShmFailure = 0x200,
    JackVersionError = 0x400,
    JackBackendError = 0x800,
    JackClientZombie = 0x1000
};

enum JackLatencyCallbackMode { JackCaptureLatency, JackPlaybackLatency };

enum JackPortFlags {
    JackPortIsInput = 0x1,
    JackPortIsOutput = 0x2,
    JackPortIsPhysical = 0x4,
    JackPortCanMonitor = 0x8,
    JackPortIsTerminal = 0x10
};

enum jackctl_param_type_t { JackParamInt = 1, JackParamUInt, JackParamChar, JackParamString, JackParamBool };

enum JackSessionEventType { JackSessionSave = 1, JackSessionSaveAndQuit = 2, JackSessionSaveTemplate = 3 };
enum JackSessionFlags { JackSessionSaveError = 0x01, JackSessionNeedTerminal = 0x02 };

// typedefs
typedef uint64_t jack_uuid_t;
typedef int32_t jack_shmsize_t;
typedef uint32_t jack_nframes_t;
typedef uint64_t jack_time_t;
typedef jack_uuid_t jack_intclient_t;
typedef struct _jack_port jack_port_t;
typedef uint32_t jack_port_id_t;
// typedef pthread_t jack_native_thread_t;
typedef enum JackOptions jack_options_t;
typedef enum JackStatus jack_status_t;
typedef enum JackLatencyCallbackMode jack_latency_callback_mode_t;
typedef void (*JackLatencyCallback)(jack_latency_callback_mode_t mode, void * arg);
typedef struct _jack_latency_range jack_latency_range_t;
typedef int (*JackProcessCallback)(jack_nframes_t nframes, void * arg);
typedef void (*JackThreadInitCallback)(void * arg);
typedef int (*JackGraphOrderCallback)(void * arg);
typedef int (*JackXRunCallback)(void * arg);
typedef int (*JackBufferSizeCallback)(jack_nframes_t nframes, void * arg);
typedef int (*JackSampleRateCallback)(jack_nframes_t nframes, void * arg);
typedef void (*JackPortRegistrationCallback)(jack_port_id_t port, int /*register*/, void * arg);
typedef void (*JackPortRenameCallback)(jack_port_id_t port, const char * old_name, const char * new_name, void * arg);
typedef void (*JackClientRegistrationCallback)(const char * name, int /*register*/, void * arg);
typedef void (*JackPortConnectCallback)(jack_port_id_t a, jack_port_id_t b, int connect, void * arg);
typedef void (*JackFreewheelCallback)(int starting, void * arg);
typedef void * (*JackThreadCallback)(void * arg);
typedef void (*JackShutdownCallback)(void * arg);
typedef void (*JackInfoShutdownCallback)(jack_status_t code, const char * reason, void * arg);
typedef float jack_default_audio_sample_t;
typedef struct _jack_session_event;
typedef enum JackSessionEventType jack_session_event_type_t;
typedef enum JackSessionFlags jack_session_flags_t;
typedef struct jackctl_driver jackctl_driver_t;
typedef struct jackctl_internal jackctl_internal_t;
typedef struct jackctl_parameter jackctl_parameter_t;

struct jackctl_server_t {
};

struct jack_client_t {
};

struct jack_session_event_t {
    jack_session_event_type_t type;
    const char * session_dir;
    const char * client_uuid;
    char * command_line;
    jack_session_flags_t flags;
    uint32_t future;
};

struct JSList {
    void * data;
    JSList * next;
};

union jackctl_parameter_value {
    uint32_t ui;
    int32_t i;
    char c;
    char str[JACK_PARAM_STRING_MAX + 1];
    bool b;
};

typedef void (*JackSessionCallback)(jack_session_event_t * event, void * arg);

typedef void (*JackSessionCallback)(jack_session_event_t * event, void * arg);

// functions

int jack_set_session_callback(jack_client_t * client, JackSessionCallback session_callback, void * arg);
int jack_session_reply(jack_client_t * client, jack_session_event_t * event);
void jack_session_event_free(jack_session_event_t * event);
// char * jack_client_get_uuid(jack_client_t * client);

// clients
jack_client_t * jackClientOpen(const char * client_name, jack_options_t options, jack_status_t * status, ...);
// jack_client_t * jack_client_new(const char * client_name);
int jack_client_close(jack_client_t * client);
// int jack_client_name_size(void);
char * jack_get_client_name(jack_client_t * client);
// char * jack_get_uuid_for_client_name(jack_client_t * client, const char * name);
// char * jack_get_client_name_by_uuid(jack_client_t * client, const char * uuid);
// int jack_internal_client_new(const char * client_name, const char * load_name, const char * load_init);
// void jack_internal_client_close(const char * client_name);
int jack_activate(jack_client_t * client);
int jack_deactivate(jack_client_t * client);
// jack_native_thread_t jack_client_thread_id(jack_client_t *);
// int jack_is_realtime(jack_client_t * client);
// jack_nframes_t jack_thread_wait(jack_client_t *, int status);
// jack_nframes_t jack_cycle_wait(jack_client_t * client);
// void jack_cycle_signal(jack_client_t * client, int status);
// int jack_set_process_thread(jack_client_t * client, JackThreadCallback fun, void * arg);
// int jack_set_thread_init_callback(jack_client_t * client, JackThreadInitCallback thread_init_callback, void * arg);
void jack_on_shutdown(jack_client_t * client, JackShutdownCallback function, void * arg);
// void jack_on_info_shutdown(jack_client_t * client, JackInfoShutdownCallback function, void * arg);
int jack_set_process_callback(jack_client_t * client, JackProcessCallback process_callback, void * arg);
// int jack_set_freewheel_callback(jack_client_t * client, JackFreewheelCallback freewheel_callback, void * arg);
// int jack_set_buffer_size_callback(jack_client_t * client, JackBufferSizeCallback bufsize_callback, void * arg);
// int jack_set_sample_rate_callback(jack_client_t * client, JackSampleRateCallback srate_callback, void * arg);
int jack_set_client_registration_callback(jack_client_t *,
                                          JackClientRegistrationCallback registration_callback,
                                          void * arg);
int jack_set_port_registration_callback(jack_client_t *,
                                        JackPortRegistrationCallback registration_callback,
                                        void * arg);
// int jack_set_port_rename_callback(jack_client_t *, JackPortRenameCallback rename_callback, void * arg);
int jack_set_port_connect_callback(jack_client_t *, JackPortConnectCallback connect_callback, void * arg);
int jack_set_graph_order_callback(jack_client_t *, JackGraphOrderCallback graph_callback, void *);
int jack_set_xrun_callback(jack_client_t *, JackXRunCallback xrun_callback, void * arg);
// int jack_set_latency_callback(jack_client_t *, JackLatencyCallback latency_callback, void *);
// int jack_set_freewheel(jack_client_t * client, int onoff);
// int jack_set_buffer_size(jack_client_t * client, jack_nframes_t nframes);
jack_nframes_t jack_get_sample_rate(jack_client_t *);
jack_nframes_t jack_get_buffer_size(jack_client_t *);
// int jack_engine_takeover_timebase(jack_client_t *);
float jack_cpu_load(jack_client_t * client);
jack_port_t * jack_port_register(jack_client_t * client,
                                 const char * port_name,
                                 const char * port_type,
                                 unsigned long flags,
                                 unsigned long buffer_size);
int jack_port_unregister(jack_client_t *, jack_port_t *);
void * jack_port_get_buffer(jack_port_t *, jack_nframes_t);
const char * jack_port_name(const jack_port_t * port);
// jack_uuid_t jack_port_uuid(const jack_port_t * port);
const char * jack_port_short_name(const jack_port_t * port);
// int jack_port_flags(const jack_port_t * port);
// const char * jack_port_type(const jack_port_t * port);
// int jack_port_is_mine(const jack_client_t *, const jack_port_t * port);
// int jack_port_connected(const jack_port_t * port);
int jack_port_connected_to(const jack_port_t * port, const char * port_name);
// const char ** jack_port_get_connections(const jack_port_t * port);
// const char ** jack_port_get_all_connections(const jack_client_t * client, const jack_port_t * port);
// int jack_port_tie(jack_port_t * src, jack_port_t * dst);
// int jack_port_untie(jack_port_t * port);
// int jack_port_set_name(jack_port_t * port, const char * port_name);
// int jack_port_rename(jack_client_t * client, jack_port_t * port, const char * port_name);
// int jack_port_set_alias(jack_port_t * port, const char * alias);
// int jack_port_unset_alias(jack_port_t * port, const char * alias);
// int jack_port_get_aliases(const jack_port_t * port, char * const aliases[2]);
// int jack_port_request_monitor(jack_port_t * port, int onoff);
// int jack_port_request_monitor_by_name(jack_client_t * client, const char * port_name, int onoff);
// int jack_port_ensure_monitor(jack_port_t * port, int onoff);
// int jack_port_monitoring_input(jack_port_t * port);
int jack_connect(jack_client_t *, const char * source_port, const char * destination_port);
int jack_disconnect(jack_client_t *, const char * source_port, const char * destination_port);
// int jack_port_disconnect(jack_client_t *, jack_port_t *);
// int jack_port_name_size(void);
// int jack_port_type_size(void);
// size_t jack_port_type_get_buffer_size(jack_client_t * client, const char * port_type);
// void jack_port_set_latency(jack_port_t *, jack_nframes_t);
// void jack_port_get_latency_range(jack_port_t * port, jack_latency_callback_mode_t mode, jack_latency_range_t *
// range); void jack_port_set_latency_range(jack_port_t * port, jack_latency_callback_mode_t mode, jack_latency_range_t
// * range); int jack_recompute_total_latencies(jack_client_t *); jack_nframes_t jack_port_get_latency(jack_port_t *
// port); jack_nframes_t jack_port_get_total_latency(jack_client_t *, jack_port_t * port); int
// jack_recompute_total_latency(jack_client_t *, jack_port_t * port);
const char ** jack_get_ports(jack_client_t *,
                             const char * port_name_pattern,
                             const char * type_name_pattern,
                             unsigned long flags);
jack_port_t * jack_port_by_name(jack_client_t *, const char * port_name);
jack_port_t * jack_port_by_id(jack_client_t * client, jack_port_id_t port_id);
// jack_nframes_t jack_frames_since_cycle_start(const jack_client_t *);
// jack_nframes_t jack_frame_time(const jack_client_t *);
// jack_nframes_t jack_last_frame_time(const jack_client_t * client);
// int jack_get_cycle_times(const jack_client_t * client,
//                         jack_nframes_t * current_frames,
//                         jack_time_t * current_usecs,
//                         jack_time_t * next_usecs,
//                         float * period_usecs);
// jack_time_t jack_frames_to_time(const jack_client_t * client, jack_nframes_t);
// jack_nframes_t jack_time_to_frames(const jack_client_t * client, jack_time_t);
// jack_time_t jack_get_time(void);
// void jack_set_error_function(void (*func)(const char *));
// void jack_set_info_function(void (*func)(const char *));
void jack_free(void * ptr);

// jack ctl
// sigset_t jackctl_setup_signals(unsigned int flags);
// void jackctl_wait_signals(sigset_t signals);
jackctl_server_t * jackctl_server_create(bool (*on_device_acquire)(const char * device_name),
                                         void (*on_device_release)(const char * device_name));
void jackctl_server_destroy(jackctl_server_t * server);
bool jackctl_server_start(jackctl_server_t * server, jackctl_driver_t * driver = nullptr);
bool jackctl_server_stop(jackctl_server_t * server);
bool jackctl_server_open(jackctl_server_t *, jackctl_driver_t *);
void jackctl_server_close(jackctl_server_t *);
const JSList * jackctl_server_get_drivers_list(jackctl_server_t * server);
const JSList * jackctl_server_get_parameters(jackctl_server_t * server);
const JSList * jackctl_server_get_internals_list(jackctl_server_t * server);
bool jackctl_server_load_internal(jackctl_server_t * server, jackctl_internal_t * internal);
// bool jackctl_server_unload_internal(jackctl_server_t * server, jackctl_internal_t * internal);
// bool jackctl_server_add_slave(jackctl_server_t * server, jackctl_driver_t * driver);
// bool jackctl_server_remove_slave(jackctl_server_t * server, jackctl_driver_t * driver);
// bool jackctl_server_switch_master(jackctl_server_t * server, jackctl_driver_t * driver);
const char * jackctl_driver_get_name(jackctl_driver_t * driver);
const JSList * jackctl_driver_get_parameters(jackctl_driver_t * driver);
const char * jackctl_internal_get_name(jackctl_internal_t * internal);
const JSList * jackctl_internal_get_parameters(jackctl_internal_t * internal);
const char * jackctl_parameter_get_name(jackctl_parameter_t * parameter);
const char * jackctl_parameter_get_short_description(jackctl_parameter_t * parameter);
const char * jackctl_parameter_get_long_description(jackctl_parameter_t * parameter);
jackctl_param_type_t jackctl_parameter_get_type(jackctl_parameter_t * parameter);
char jackctl_parameter_get_id(jackctl_parameter_t * parameter);
// bool jackctl_parameter_is_set(jackctl_parameter_t * parameter);
// bool jackctl_parameter_reset(jackctl_parameter_t * parameter);
// union jackctl_parameter_value jackctl_parameter_get_value(jackctl_parameter_t * parameter);
bool jackctl_parameter_set_value(jackctl_parameter_t * parameter, const union jackctl_parameter_value * value_ptr);
union jackctl_parameter_value jackctl_parameter_get_default_value(jackctl_parameter_t * parameter);
// bool jackctl_parameter_has_range_constraint(jackctl_parameter_t * parameter);
// bool jackctl_parameter_has_enum_constraint(jackctl_parameter_t * parameter);
// uint32_t jackctl_parameter_get_enum_constraints_count(jackctl_parameter_t * parameter);
// union jackctl_parameter_value jackctl_parameter_get_enum_constraint_value(jackctl_parameter_t * parameter,
//                                                                          uint32_t index);
// const char * jackctl_parameter_get_enum_constraint_description(jackctl_parameter_t * parameter, uint32_t index);
// void jackctl_parameter_get_range_constraint(jackctl_parameter_t * parameter,
//                                            union jackctl_parameter_value * min_ptr,
//                                            union jackctl_parameter_value * max_ptr);
// bool jackctl_parameter_constraint_is_strict(jackctl_parameter_t * parameter);
// bool jackctl_parameter_constraint_is_fake_value(jackctl_parameter_t * parameter);
// void jack_error(const char * format, ...);
// void jack_info(const char * format, ...);
// void jack_log(const char * format, ...);

// JSList / SList
JSList const * jack_slist_next(JSList const *);
jackctl_parameter_t * jackctl_get_parameter(const JSList * parameters_list, const char * parameter_name);
jackctl_driver_t * jackctl_server_get_driver(jackctl_server_t * server, const char * driver_name);
jackctl_internal_t * jackctl_server_get_internal(jackctl_server_t * server, const char * internal_name);
