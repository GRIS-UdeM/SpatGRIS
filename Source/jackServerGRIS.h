//
//  jackServerGRIS.h
//  spatServerGRIS
//
//  Created by GRIS on 2017-03-21.
//
//

#ifndef jackServerGRIS_h
#define jackServerGRIS_h

#include <stdio.h>

#include <stdlib.h>
#include <vector>
#include <stdio.h>
#include <math.h>

#include "jack/jack.h"
#include "jack/transport.h"
#include "jack/types.h"
#include "jack/session.h"
#include <jack/control.h>

static jackctl_driver_t * jackctl_server_get_driver(jackctl_server_t *server, const char *driver_name)
{
    const JSList * node_ptr = jackctl_server_get_drivers_list(server);
    
    while (node_ptr) {
        if (strcmp(jackctl_driver_get_name((jackctl_driver_t *)node_ptr->data), driver_name) == 0) {
            
            return (jackctl_driver_t *)node_ptr->data;
        }
        node_ptr = jack_slist_next(node_ptr);
    }
    
    return NULL;
}

static jackctl_internal_t * jackctl_server_get_internal(jackctl_server_t *server, const char *internal_name)
{
    const JSList * node_ptr = jackctl_server_get_internals_list(server);
    
    while (node_ptr) {
        if (strcmp(jackctl_internal_get_name((jackctl_internal_t *)node_ptr->data), internal_name) == 0) {
            return (jackctl_internal_t *)node_ptr->data;
        }
        node_ptr = jack_slist_next(node_ptr);
    }
    
    return NULL;
}

static JSList * jackctl_server_get_internal2(jackctl_server_t *server, const char *internal_name)
{
    const JSList * node_ptr = jackctl_server_get_internals_list(server);
    
    while (node_ptr) {
        if (strcmp(jackctl_internal_get_name((jackctl_internal_t *)node_ptr->data), internal_name) == 0) {
            return (JSList *)node_ptr;
        }
        node_ptr = jack_slist_next(node_ptr);
    }
    
    return NULL;
}



static jackctl_parameter_t * jackctl_get_parameter(const JSList * parameters_list, const char * parameter_name)
{
    while (parameters_list)
    {
        printf("\nparameter name = %s\n", jackctl_parameter_get_name((jackctl_parameter_t *)parameters_list->data));

        if (strcmp(jackctl_parameter_get_name((jackctl_parameter_t *)parameters_list->data), parameter_name) == 0)
        {
            return (jackctl_parameter_t *)parameters_list->data;
        }
        parameters_list = jack_slist_next(parameters_list);
    }
    return NULL;
}


static void print_value(union jackctl_parameter_value value, jackctl_param_type_t type)
{
    switch (type) {
            
        case JackParamInt:
            printf("parameter value = %d\n", value.i);
            break;
            
        case JackParamUInt:
            printf("parameter value = %u\n", value.ui);
            break;
            
        case JackParamChar:
            printf("parameter value = %c\n", value.c);
            break;
            
        case JackParamString:
            printf("parameter value = %s\n", value.str);
            break;
            
        case JackParamBool:
            printf("parameter value = %d\n", value.b);
            break;
    }
}

static void print_parameters(const JSList * node_ptr)
{
    while (node_ptr != NULL) {
        jackctl_parameter_t * parameter = (jackctl_parameter_t *)node_ptr->data;
        printf("\nparameter name = %s\n", jackctl_parameter_get_name(parameter));
        if(!strcmp(jackctl_parameter_get_name(parameter), "rate")){
            union jackctl_parameter_value value;
            value.ui = 48000;
            printf("%i ",jackctl_parameter_set_value(parameter, &value));

        }
        printf("parameter id = %c\n", jackctl_parameter_get_id(parameter));
        printf("parameter short decs = %s\n", jackctl_parameter_get_short_description(parameter));
        printf("parameter long decs = %s\n", jackctl_parameter_get_long_description(parameter));
        print_value(jackctl_parameter_get_default_value(parameter), jackctl_parameter_get_type(parameter));
        node_ptr = jack_slist_next(node_ptr);
    }
}

static void print_driver(jackctl_driver_t * driver)
{
    printf("\n--------------------------\n");
    printf("driver = %s\n", jackctl_driver_get_name(driver));
    printf("-------------------------- \n");
    print_parameters(jackctl_driver_get_parameters(driver));
}

static void print_internal(jackctl_internal_t * internal)
{
    printf("\n-------------------------- \n");
    printf("internal = %s\n", jackctl_internal_get_name(internal));
    printf("-------------------------- \n");
    print_parameters(jackctl_internal_get_parameters(internal));
}

static void usage()
{
    fprintf (stderr, "\n"
             "usage: jack_server_control \n"
             "              [ --driver OR -d driver_name ]\n"
             "              [ --client OR -c client_name ]\n"
             );
}

/*
static void jack_info ( const char * format )
{
    printf("\n%s \n", format);

}*/


class jackServerGRIS {
public:
    jackctl_server_t *server;
        jackctl_sigmask_t * sigmask;
    jackServerGRIS();
    ~jackServerGRIS();
    
private:

};


#endif /* jackServerGRIS_h */
