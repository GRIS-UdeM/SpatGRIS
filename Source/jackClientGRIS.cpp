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

#include "jackClientGRIS.h"
#include <cmath>

int simple_quit = 0;

static int process_audio (jack_nframes_t nframes, void* arg) {
    
    jackClientGris* client = (jackClientGris*)arg;
    
    /*jack_default_audio_sample_t *out1 = (jack_default_audio_sample_t*)jack_port_get_buffer (client->output_port1, nframes);
    jack_default_audio_sample_t *out2 = (jack_default_audio_sample_t*)jack_port_get_buffer (client->output_port2, nframes);
    
    for(int i = 0; i < nframes; ++i) {
        out1[i] = client->sine[client->left_phase];
        out2[i] = client->sine[client->right_phase];
        client->left_phase += 1;
        if (client->left_phase >= client->sine.size()){
            client->left_phase -= client->sine.size();
        }
        client->right_phase += 2; // higher pitch so we can distinguish left and right. 
        if(client->right_phase >= client->sine.size()){
            client->right_phase -= client->sine.size();
        }
    }*/
   
    
    return 0;
}


void session_callback (jack_session_event_t *event, void *arg)
{
    jackClientGris* client = (jackClientGris*)arg;
    
    char retval[100];
    printf ("session notification\n");
    printf ("path %s, uuid %s, type: %s\n", event->session_dir, event->client_uuid, event->type == JackSessionSave ? "save" : "quit");
    
    
    snprintf (retval, 100, "jack_simple_session_client %s", event->client_uuid);
    event->command_line = strdup (retval);
    
    jack_session_reply(client->client, event);
    
    if (event->type == JackSessionSaveAndQuit) {
        simple_quit = 1;
    }
    
    jack_session_event_free (event);
}

void jack_shutdown (void *arg)
{
    fprintf (stdout, "\nBye jack\n");
    exit (1);
}

jackClientGris::jackClientGris() {
    clientReady = false;
    //--------------------------------------------------
    //open a client connection to the JACK server. Start server if it is not running.
    //--------------------------------------------------
    const char      **ports;
    const char      *client_name = "jackClientGris";
    const char      *server_name = "coreaudio";
    jack_options_t  options = JackNullOption;
    jack_status_t   status;
    
    client = jack_client_open (client_name, options, &status, server_name);
    if (client == NULL) {
        
        fprintf (stderr, "\nTry again...\n");
        const char      *server_name = "coreaudio";
        jack_options_t  options = JackServerName;
        client = jack_client_open (client_name, options, &status, server_name);
        if (client == NULL) {
            fprintf (stderr, "\n\n\n======jack_client_open() failed, status = 0x%2.0x\n", status);
            if (status & JackServerFailed) {
                fprintf (stderr, "\n\n\n======Unable to connect to JACK server\n");
            }
            return;
        }
    }
    if (status & JackServerStarted) {
        fprintf (stderr, "\n===================\njackdmp wasn't running so it was started\n===================\n");
        return;
    }
    if (status & JackNameNotUnique) {
        client_name = jack_get_client_name(client);
        fprintf (stderr, "\n\n\n======chosen name already existed, new unique name `%s' assigned\n", client_name);
        return;
    }
    
    //--------------------------------------------------
    //fill wave table.
    //--------------------------------------------------
    float fs = jack_get_sample_rate (client);
    float f  = 1000.f / fs;
    float T  = 1/f;
    for(int i = 0; i < 10*T; ++i) {
        sine.push_back(0.2 * sin( i * M_PI * 2. * f ));
    }
    left_phase = right_phase = 0;
    
    //--------------------------------------------------
    //register callback, ports
    //--------------------------------------------------
    jack_set_process_callback (client, process_audio, this);
    jack_on_shutdown (client, jack_shutdown, 0);
    jack_set_session_callback (client, session_callback, this);
    
    printf ("engine sample rate: %" PRIu32 "\n",jack_get_sample_rate (client));
    
    input_port   = jack_port_register (client,  "input",   JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput,  0);
    output_port1 = jack_port_register (client,  "output1", JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);
    output_port2 = jack_port_register (client,  "output2", JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);

    
    //--------------------------------------------------
    // Activate client and connect the ports. Playback ports are "input" to the backend, and capture ports are "output" from it.
    //--------------------------------------------------
    if (jack_activate (client)) {
        fprintf(stderr, "\n\n\n======cannot activate client");
        return;
    }
    ports = jack_get_ports (client, NULL, NULL, JackPortIsPhysical|JackPortIsInput);
    if (ports == NULL) {
        fprintf(stderr, "\n\n\n======no physical playback ports\n");
        return;
    }
    if (jack_connect (client, jack_port_name (output_port1), ports[0])) {
        fprintf (stderr, "\n\n\n======cannot connect output ports\n");
        return;
    }
    if (jack_connect (client, jack_port_name (output_port2), ports[1])) {
        fprintf (stderr, "\n\n\n======cannot connect output ports\n");
        return;
    }
    
    jack_free (ports);
    clientReady = true;
}


jackClientGris::~jackClientGris() {
    jack_deactivate(client);
    jack_port_unregister(client, input_port);
    jack_port_unregister(client, output_port1);
    jack_port_unregister(client, output_port2);
    jack_client_close(client);
    
}
