/*
 This file is part of SpatGRIS2.

 Developers: Samuel B�land, Olivier B�langer, Nicolas Masson

 SpatGRIS2 is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 SpatGRIS2 is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with SpatGRIS2.  If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

#include <cstdint>

#ifdef WIN32
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
// typedef struct _jack_session_event;
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

// clients
jack_client_t * jackClientOpen(const char * client_name, jack_options_t options, jack_status_t * status, ...);
char * jack_get_client_name(jack_client_t * client);
int jack_deactivate(jack_client_t * client);
int jack_set_process_callback(jack_client_t * client, JackProcessCallback process_callback, void * arg);
int jack_set_port_connect_callback(jack_client_t *, JackPortConnectCallback connect_callback, void * arg);
jack_nframes_t jack_get_buffer_size(jack_client_t *);
float jack_cpu_load(jack_client_t * client);
jack_port_t * jack_port_register(jack_client_t * client,
                                 const char * port_name,
                                 const char * port_type,
                                 unsigned long flags,
                                 unsigned long buffer_size);
int jack_port_unregister(jack_client_t *, jack_port_t *);
void * jack_port_get_buffer(jack_port_t *, jack_nframes_t);
const char * jack_port_name(const jack_port_t * port);
const char * jack_port_short_name(const jack_port_t * port);
int jack_port_connected_to(const jack_port_t * port, const char * port_name);
int jack_connect(jack_client_t *, const char * source_port, const char * destination_port);
const char ** jack_get_ports(jack_client_t *,
                             const char * port_name_pattern,
                             const char * type_name_pattern,
                             unsigned long flags);
jack_port_t * jack_port_by_name(jack_client_t *, const char * port_name);
jack_port_t * jack_port_by_id(jack_client_t * client, jack_port_id_t port_id);
void jack_free(void * ptr);

// jack ctl
const JSList * jackctl_server_get_parameters(jackctl_server_t * server);

// JSList / SList
JSList const * jack_slist_next(JSList const *);
