/*
 This file is part of ServerGris.
 
 Developers: Olivier Belanger, Nicolas Masson
 
 ServerGris is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.
 
 ServerGris is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with ServerGris.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <stdarg.h>
#include <stdio.h>
#include "ServerGrisConstants.h"
#include "jackServerGRIS.h"
#include "jackClientGRIS.h"

static bool jack_server_log_print = false;

// Jack server utilities.

static void jack_server_log(const char *format, ...) {
    if (jack_server_log_print) {
        char buffer[256];
        va_list args;
        va_start(args, format);
        vsprintf(buffer, format, args);
        va_end(args);
        printf("%s", buffer);
    }
}

static void print_value(union jackctl_parameter_value value, jackctl_param_type_t type) {
    switch (type) {
        case JackParamInt:
            jack_server_log("parameter value = %d\n", value.i);
            break;
        case JackParamUInt:
            jack_server_log("parameter value = %u\n", value.ui);
            break;
        case JackParamChar:
            jack_server_log("parameter value = %c\n", value.c);
            break;
        case JackParamString:
            jack_server_log("parameter value = %s\n", value.str);
            break;
        case JackParamBool:
            jack_server_log("parameter value = %d\n", value.b);
            break;
    }
}

static void print_parameters(const JSList *node_ptr) {
    while (node_ptr != NULL) {
        jackctl_parameter_t *parameter = (jackctl_parameter_t *)node_ptr->data;
        jack_server_log("\nparameter name = %s\n", jackctl_parameter_get_name(parameter));
        if (!jackctl_parameter_get_id(parameter))
            jack_server_log("parameter id = \n");
        else
            jack_server_log("parameter id = %c\n", jackctl_parameter_get_id(parameter));
        jack_server_log("parameter short decs = %s\n", jackctl_parameter_get_short_description(parameter));
        jack_server_log("parameter long decs = %s\n", jackctl_parameter_get_long_description(parameter));
        print_value(jackctl_parameter_get_default_value(parameter), jackctl_parameter_get_type(parameter));
        node_ptr = jack_slist_next(node_ptr);
    }
}

static void print_driver(jackctl_driver_t *driver) {
    jack_server_log("Jack driver = %s\n", jackctl_driver_get_name(driver));
    print_parameters(jackctl_driver_get_parameters(driver));
}

static void print_internal(jackctl_internal_t *internal) {
    jack_server_log("Jack internal = %s\n", jackctl_internal_get_name(internal));
    print_parameters(jackctl_internal_get_parameters(internal));
}

static bool on_device_acquire(const char *device_name) {
    jack_server_log("on_device_acquire %s \n", device_name);
    return true;
}

static void on_device_release(const char *device_name) {
    jack_server_log("on_device_release %s \n", device_name);
}

static jackctl_parameter_t * jackctl_get_parameter(const JSList *parameters_list, const char *parameter_name) {
    while (parameters_list) {
        if (strcmp(jackctl_parameter_get_name((jackctl_parameter_t *)parameters_list->data), parameter_name) == 0) {
            return (jackctl_parameter_t *)parameters_list->data;
        }
        parameters_list = jack_slist_next(parameters_list);
    }
    return NULL;
}

static jackctl_driver_t * jackctl_server_get_driver(jackctl_server_t *server, const char *driver_name) {
    const JSList * node_ptr = jackctl_server_get_drivers_list(server);
    while (node_ptr) {
        if (strcmp(jackctl_driver_get_name((jackctl_driver_t *)node_ptr->data), driver_name) == 0) {
            return (jackctl_driver_t *)node_ptr->data;
        }
        node_ptr = jack_slist_next(node_ptr);
    }
    return NULL;
}

static jackctl_internal_t * jackctl_server_get_internal(jackctl_server_t *server, const char *internal_name) {
    const JSList * node_ptr = jackctl_server_get_internals_list(server);
    while (node_ptr) {
        if (strcmp(jackctl_internal_get_name((jackctl_internal_t *)node_ptr->data), internal_name) == 0) {
            return (jackctl_internal_t *)node_ptr->data;
        }
        node_ptr = jack_slist_next(node_ptr);
    }
    return NULL;
}

// Jack server class definition.

jackServerGRIS::jackServerGRIS(unsigned int rateV, unsigned int periodV, int *errorCode) {
    this->rateValue = rateV;
    this->periodValue = periodV;
    const JSList *parameters;
    const JSList *driverParams;
    const JSList *drivers;
    const JSList *internals;
    const JSList *node_ptr;

    this->server = jackctl_server_create(on_device_acquire, on_device_release);

    if (this->server) {
        parameters = jackctl_server_get_parameters(this->server);

        jackctl_parameter_t *param;
        union jackctl_parameter_value value;

        // Turn off Jack verbose mode.
        param = jackctl_get_parameter(parameters, "verbose");
        if (param != NULL) {
            value.b = false;
            jackctl_parameter_set_value(param, &value);
        }

        jack_server_log("\nList of server parameters \n");
        jack_server_log("========================= \n");
        
        print_parameters(parameters);
        
        jack_server_log("\nList of drivers \n");
        jack_server_log("=============== \n");
        
        drivers = jackctl_server_get_drivers_list(this->server);
        node_ptr = drivers;
        while (node_ptr != NULL) {
            print_driver((jackctl_driver_t *)node_ptr->data);

            driverParams = jackctl_driver_get_parameters((jackctl_driver_t *)node_ptr->data);
            // Set sampling rate.
            param = jackctl_get_parameter(driverParams, "rate");
            if (param != NULL) {
                value.ui = value.i = this->rateValue;
                jackctl_parameter_set_value(param, &value);
            }
            // Set buffer size.
            param = jackctl_get_parameter(driverParams, "period");
            if (param != NULL) {
                value.ui = value.i = this->periodValue;
                jackctl_parameter_set_value(param, &value);
            }

            node_ptr = jack_slist_next(node_ptr);
        }
        
        jack_server_log("\nList of internal clients \n");
        jack_server_log("======================== \n");
        
        internals = jackctl_server_get_internals_list(this->server);
        node_ptr = internals;
        while (node_ptr != NULL) {
            print_internal((jackctl_internal_t *)node_ptr->data);
            node_ptr = jack_slist_next(node_ptr);
        }
        
        jack_server_log("\nStart Jack Server \n");
        jack_server_log("================= \n");

        if (jackctl_server_open(this->server, jackctl_server_get_driver(this->server, DriverNameSys))) {
            if (jackctl_server_start(this->server)) {
                jackctl_server_load_internal(this->server, jackctl_server_get_internal(this->server, ClientNameSys));
            } else {
                *errorCode = 3;
            }
        } else {
            *errorCode = 2;
        }
    } else {
        *errorCode = 1;
    }
}

jackServerGRIS::~jackServerGRIS(){
    if (this->server != nullptr) {
        jackctl_server_stop(this->server);
        jackctl_server_close(this->server);
        jackctl_server_destroy(this->server);
    }
}
