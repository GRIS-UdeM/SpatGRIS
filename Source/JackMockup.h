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
enum JackPortFlags {
    JackPortIsInput = 0x1,
    JackPortIsOutput = 0x2,
    JackPortIsPhysical = 0x4,
    JackPortCanMonitor = 0x8,
    JackPortIsTerminal = 0x10
};

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
typedef float jack_default_audio_sample_t;
// typedef struct _jack_session_event;
typedef enum JackSessionEventType jack_session_event_type_t;
typedef enum JackSessionFlags jack_session_flags_t;
typedef struct jackctl_driver jackctl_driver_t;
typedef struct jackctl_internal jackctl_internal_t;
typedef struct jackctl_parameter jackctl_parameter_t;

struct jack_client_t {
};

// clients
const char ** jack_get_ports(jack_client_t *,
                             const char * port_name_pattern,
                             const char * type_name_pattern,
                             unsigned long flags);
jack_port_t * jack_port_by_id(jack_client_t * client, jack_port_id_t port_id);
void jack_free(void * ptr);
