//
//  jackServerGRIS.cpp
//  spatServerGRIS
//
//  Created by GRIS on 2017-03-21.
//
//

#include "jackServerGRIS.h"

bool on_device_acquire(const char *device_name)
{
    printf ("on_device_acquire %s \n", device_name);
    return true;
}

void on_device_release(const char *device_name)
{
    printf ("on_device_release %s \n", device_name);
}


jackServerGRIS::jackServerGRIS(){
    
    const JSList * parameters;
    const JSList * drivers;
    const JSList * internals;
    const JSList * node_ptr;

    const char* driver_name = "coreaudio";
    const char* client_name = "audioadapter";
    
    server = jackctl_server_create(on_device_acquire, on_device_release);
    parameters = jackctl_server_get_parameters(server);
    
   /* jackctl_parameter_t* param;
    union jackctl_parameter_value value;
    param = jackctl_get_parameter(parameters, "verbose");
    if (param != NULL) {
        value.b = true;
        jackctl_parameter_set_value(param, &value);
    }*/
    
    printf("\n========================== \n");
    printf("List of server parameters \n");
    printf("========================== \n");
    
    print_parameters(parameters);
    
    printf("\n========================== \n");
    printf("List of drivers \n");
    printf("========================== \n");
    
    drivers = jackctl_server_get_drivers_list(server);
    node_ptr = drivers;
    while (node_ptr != NULL) {
        print_driver((jackctl_driver_t *)node_ptr->data);
        node_ptr = jack_slist_next(node_ptr);
    }
    
    printf("\n========================== \n");
    printf("List of internal clients \n");
    printf("========================== \n");
    
    internals = jackctl_server_get_internals_list(server);
    node_ptr = internals;
    while (node_ptr != NULL) {
        print_internal((jackctl_internal_t *)node_ptr->data);
        node_ptr = jack_slist_next(node_ptr);
    }

    jackctl_server_open(server, jackctl_server_get_driver(server, driver_name));
    jackctl_server_start(server);
    jackctl_server_load_internal(server, jackctl_server_get_internal(server, client_name));
        
    const JSList * parameters2 = jackctl_server_get_parameters(server);
    print_parameters(parameters2);
    printf("\n========================== \n");
    printf("Jack Server Run \n");

}

jackServerGRIS::~jackServerGRIS(){
    jackctl_server_stop(server);
    jackctl_server_close(server);
    jackctl_server_destroy(server);
}
