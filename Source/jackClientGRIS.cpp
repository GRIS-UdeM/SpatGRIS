/*
 This file is part of spatServerGRIS.
 
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

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <signal.h>

#ifndef WIN32
#include <unistd.h>
#endif


#include "jackClientGRIS.h"

jack_port_t     *output_port1, *output_port2;
jack_client_t   *client;


jackClientGRIS::jackClientGRIS () {
    const char      *client_name = "jackClientGris";
    const char      *server_name = NULL;
    jack_options_t  options = JackNullOption;
    
    
    //fill wave table
    paTestData data;
    for(int i = 0; i < TABLE_SIZE; ++i) {
        data.sine[i] = 0.2 * (float) sin( ((double)i/(double)TABLE_SIZE) * M_PI * 2. );
    }
    data.left_phase = data.right_phase = 0;
    
    // open a client connection to the JACK server. Start server if it is not running.
    jack_status_t status;
    client = jack_client_open (client_name, options, &status, server_name);
    if (client == NULL) {
        fprintf (stderr, "jack_client_open() failed, status = 0x%2.0x\n", status);
        if (status & JackServerFailed) {
            fprintf (stderr, "Unable to connect to JACK server\n");
        }
        exit (1);
    }
    if (status & JackServerStarted) {
        fprintf (stderr, "\n===================\njackdmp wasn't running so it was started\n===================\n");
    }
    if (status & JackNameNotUnique) {
        client_name = jack_get_client_name(client);
        fprintf (stderr, "chosen name already existed, new unique name `%s' assigned\n", client_name);
    }
    
    // tell the JACK server to call `process()' whenever there is work to be done.
    jack_set_process_callback (client, process, &data);
    
    // tell the JACK server to call `jack_shutdown()' if it ever shuts down, either entirely, or if it just decides to stop calling us.
    jack_on_shutdown (client, jack_shutdown, 0);
    
    // create two ports
    output_port1 = jack_port_register (client, "output1", JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);
    output_port2 = jack_port_register (client, "output2", JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);
    
    if ((output_port1 == NULL) || (output_port2 == NULL)) {
        fprintf(stderr, "no more JACK ports available\n");
        exit (1);
    }
    
    // Tell the JACK server that we are ready to roll.  Our process() callback will start running now.
    if (jack_activate (client)) {
        fprintf (stderr, "cannot activate client");
        exit (1);
    }
    
    // Connect the ports.  You can't do this before the client is activated, because we can't make connections to clients
    // that aren't running.  Note the confusing (but necessary) orientation of the driver backend ports: playback ports are
    //"input" to the backend, and capture ports are "output" from it.
    const char **ports = jack_get_ports (client, NULL, NULL, JackPortIsPhysical|JackPortIsInput);
    if (ports == NULL) {
        fprintf(stderr, "no physical playback ports\n");
        exit (1);
    }
    
    if (jack_connect (client, jack_port_name (output_port1), ports[0])) {
        fprintf (stderr, "cannot connect output ports\n");
    }
    
    if (jack_connect (client, jack_port_name (output_port2), ports[1])) {
        fprintf (stderr, "cannot connect output ports\n");
    }
    
    jack_free (ports);
    
    // install a signal handler to properly quit jack client
    #ifdef WIN32
    signal(SIGINT,  signal_handler);
    signal(SIGABRT, signal_handler);
    signal(SIGTERM, signal_handler);
    #else
    signal(SIGQUIT, signal_handler);
    signal(SIGTERM, signal_handler);
    signal(SIGHUP,  signal_handler);
    signal(SIGINT,  signal_handler);
    #endif
    
    /* keep running until the Ctrl+C */
    
    while (1) {
        #ifdef WIN32
        Sleep(1000);
        #else
        sleep (1);
        #endif
    }
    
    jack_client_close (client);
    //    exit (0);


}



static void signal_handler(int sig) {
    jack_client_close(client);
    fprintf(stderr, "signal received, exiting ...\n");
    exit(0);
}



// The process callback for this JACK application is called in a special realtime thread once for each audio cycle.
int process (jack_nframes_t nframes, void *arg) {
    
    paTestData *data = (paTestData*)arg;
    
    jack_default_audio_sample_t *out1 = (jack_default_audio_sample_t*)jack_port_get_buffer (output_port1, nframes);
    jack_default_audio_sample_t *out2 = (jack_default_audio_sample_t*)jack_port_get_buffer (output_port2, nframes);
    
    for(int i = 0; i < nframes; ++i) {
        out1[i] = data->sine[data->left_phase];   /* left */
        out2[i] = data->sine[data->right_phase];  /* right */
        data->left_phase += 1;
        if (data->left_phase >= TABLE_SIZE){
            data->left_phase -= TABLE_SIZE;
        }
        data->right_phase += 3; /* higher pitch so we can distinguish left and right. */
        if(data->right_phase >= TABLE_SIZE){
            data->right_phase -= TABLE_SIZE;
        }
    }
    
    return 0;
}




//JACK calls this shutdown_callback if the server ever shuts down or decides to disconnect the client.
void jack_shutdown (void *arg) {
    fprintf(stderr, "\n==================\njackdmp was shutdown, shutting down client\n==================\n");
    exit (1);
}



