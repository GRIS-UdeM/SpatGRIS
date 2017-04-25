/*
 This file is part of spatServerGRIS.
 
 Developers: Nicolas Masson
 
 spatServerGRIS is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.
 
 spatServerGRIS is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with spatServerGRIS.  If not, see <http://www.gnu.org/licenses/>.
 */


#ifndef jackServerGRIS_h
#define jackServerGRIS_h

#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <stdio.h>
#include <math.h>

#include <jack/jack.h>
#include <jack/transport.h>
#include <jack/types.h>
#include <jack/session.h>
#include <jack/control.h>

#ifndef PRINT_SERVER
#define PRINT_SERVER 0
#endif


class jackServerGRIS {
public:
    jackctl_server_t *server;
    jackServerGRIS(unsigned int rateV = 48000);
    ~jackServerGRIS();
    
    
    jackctl_driver_t * jackctl_server_get_driver(jackctl_server_t *server, const char *driver_name)
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
    
    jackctl_internal_t * jackctl_server_get_internal(jackctl_server_t *server, const char *internal_name)
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
    
    JSList * jackctl_server_get_internal2(jackctl_server_t *server, const char *internal_name)
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
    
    
    
    jackctl_parameter_t * jackctl_get_parameter(const JSList * parameters_list, const char * parameter_name)
    {
        while (parameters_list)
        {
            #if PRINT_SERVER
            printf("\nparameter name = %s\n", jackctl_parameter_get_name((jackctl_parameter_t *)parameters_list->data));
            #endif
            if (strcmp(jackctl_parameter_get_name((jackctl_parameter_t *)parameters_list->data), parameter_name) == 0)
            {
                return (jackctl_parameter_t *)parameters_list->data;
            }
            parameters_list = jack_slist_next(parameters_list);
        }
        return NULL;
    }
    
    
    void print_value(union jackctl_parameter_value value, jackctl_param_type_t type)
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
    
    void print_parameters(const JSList * node_ptr)
    {
        while (node_ptr != NULL) {
            jackctl_parameter_t * parameter = (jackctl_parameter_t *)node_ptr->data;
            #if PRINT_SERVER
            printf("\nparameter name = %s\n", jackctl_parameter_get_name(parameter));
            #endif
            
            if(!strcmp(jackctl_parameter_get_name(parameter), "rate")){
                union jackctl_parameter_value value;
                value.ui = rateValue;
                value.i = rateValue;
                int i = jackctl_parameter_set_value(parameter, &value);
                #if PRINT_SERVER
                printf("%s ",i? "true" : "false");
                #endif
            }
            
            /*if(!strcmp(jackctl_parameter_get_name(parameter), "audio-ins")){
             union jackctl_parameter_value value;
             value.ui = 30;
             value.i = 30;
             printf("%s ",jackctl_parameter_set_value(parameter, &value)? "true" : "false");
             
             }
             
             if(!strcmp(jackctl_parameter_get_name(parameter), "audio-outs")){
             union jackctl_parameter_value value;
             value.ui = 30;
             value.i = 30;
             printf("%s ",jackctl_parameter_set_value(parameter, &value)? "true" : "false");
             
             }
             
             if(!strcmp(jackctl_parameter_get_name(parameter), "input-list")){
             union jackctl_parameter_value value;
             std::strncpy(value.str, "24",JACK_PARAM_STRING_MAX);
             printf("%s ",jackctl_parameter_set_value(parameter, &value)? "true" : "false");
             
             }
             
             if(!strcmp(jackctl_parameter_get_name(parameter), "output-list")){
             union jackctl_parameter_value value;
             std::strncpy(value.str, "24",JACK_PARAM_STRING_MAX);
             
             printf("%s ",jackctl_parameter_set_value(parameter, &value)? "true" : "false");
             
             }*/
            
            #if PRINT_SERVER
            printf("parameter id = %c\n", jackctl_parameter_get_id(parameter));
            printf("parameter short decs = %s\n", jackctl_parameter_get_short_description(parameter));
            printf("parameter long decs = %s\n", jackctl_parameter_get_long_description(parameter));
            print_value(jackctl_parameter_get_default_value(parameter), jackctl_parameter_get_type(parameter));
            #endif
            node_ptr = jack_slist_next(node_ptr);
        }
    }
    
    void print_driver(jackctl_driver_t * driver)
    {
        #if PRINT_SERVER
        printf("\n--------------------------\n");
        printf("driver = %s\n", jackctl_driver_get_name(driver));
        printf("-------------------------- \n");
        #endif
        print_parameters(jackctl_driver_get_parameters(driver));
    }
    
    void print_internal(jackctl_internal_t * internal)
    {
        #if PRINT_SERVER
        printf("\n-------------------------- \n");
        printf("internal = %s\n", jackctl_internal_get_name(internal));
        printf("-------------------------- \n");
        #endif
        print_parameters(jackctl_internal_get_parameters(internal));
    }
    
private :
    unsigned int rateValue;
};


#endif /* jackServerGRIS_h */
