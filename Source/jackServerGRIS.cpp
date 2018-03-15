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
#include "jackServerGRIS.h"
#include "jackClientGRIS.h"

static bool jack_server_log_print = false;

static void jack_server_log(const char* format, ...) {
    if (jack_server_log_print) {
        char buffer[256];
        va_list args;
        va_start(args, format);
        vsprintf(buffer, format, args);
        va_end(args);
        printf("%s", buffer);
    }
}

bool on_device_acquire(const char *device_name)
{
    jack_server_log("on_device_acquire %s \n", device_name);
    return true;
}

void on_device_release(const char *device_name)
{
    jack_server_log("on_device_release %s \n", device_name);
}


jackServerGRIS::jackServerGRIS(unsigned int rateV){
    
    this->rateValue = rateV;
    const JSList * parameters;
    const JSList * drivers;
    const JSList * internals;
    const JSList * node_ptr;

    this->server = jackctl_server_create(on_device_acquire, on_device_release);
    parameters = jackctl_server_get_parameters(this->server);
    
    /* FIXME
     * How to set jack's verbose mode to off?
     */
    jackctl_parameter_t* param;
    union jackctl_parameter_value value;
    
    
    param = jackctl_get_parameter(parameters, "verbose");
    if (param != NULL) {
        value.b = false;
        jackctl_parameter_set_value(param, &value);
    }
    
    /*jackctl_parameter_t* param;
    union jackctl_parameter_value value;
    param = jackctl_get_parameter(parameters, "self-connect-mode");
    if (param != NULL) {
        value.b = false;
        jackctl_parameter_set_value(param, &value);
    }*/

    /*param = jackctl_get_parameter(parameters, "sync");
    if (param != NULL) {
        value.b = true;
        jackctl_parameter_set_value(param, &value);
    }*/
    
    
    jack_server_log("\n========================== \n");
    jack_server_log("List of server parameters \n");
    jack_server_log("========================== \n");
    
    print_parameters(parameters);
    
    jack_server_log("\n========================== \n");
    jack_server_log("List of drivers \n");
    jack_server_log("========================== \n");
    
    drivers = jackctl_server_get_drivers_list(this->server);
    node_ptr = drivers;
    while (node_ptr != NULL) {
        print_driver((jackctl_driver_t *)node_ptr->data);
        node_ptr = jack_slist_next(node_ptr);
    }
    
    #if PRINT_SERVER
    jack_server_log("\n========================== \n");
    jack_server_log("List of internal clients \n");
    jack_server_log("========================== \n");
    #endif
    
    internals = jackctl_server_get_internals_list(this->server);
    node_ptr = internals;
    while (node_ptr != NULL) {
        print_internal((jackctl_internal_t *)node_ptr->data);
        node_ptr = jack_slist_next(node_ptr);
    }
    
    jack_server_log("\n========================== \n");
    jack_server_log("Start Jack Server \n");
    jack_server_log("========================== \n");

    jackctl_server_open(this->server, jackctl_server_get_driver(this->server, DriverNameSys));
    jackctl_server_start(this->server);
    jackctl_server_load_internal(this->server, jackctl_server_get_internal(this->server, ClientNameSys));
    
    #if PRINT_SERVER
    const JSList * parameters2 = jackctl_server_get_parameters(server);
    print_parameters(parameters2);
    #endif
    
    jack_server_log("\n========================== \n");
    jack_server_log("Jack Server Run \n");
    jack_server_log("========================== \n");
    

}

jackServerGRIS::~jackServerGRIS(){
    jackctl_server_stop(this->server);
    jackctl_server_close(this->server);
    jackctl_server_destroy(this->server);
}
