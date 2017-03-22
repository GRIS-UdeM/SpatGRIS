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
    

    jack_default_audio_sample_t * outs[MaxOutputs+MaxInputs];
    for (int iSpeaker = 1; iSpeaker < MaxOutputs+MaxInputs-1; iSpeaker++) {
        outs[iSpeaker] = (jack_default_audio_sample_t*)jack_port_get_buffer ((jack_port_t*)iSpeaker, nframes);
    }
    /*for(int i = 0; i < nframes; ++i) {
        out1[i] = client->sine[client->left_phase];
        out2[i] = client->sine[client->right_phase];
        client->left_phase += 1;
        if (client->left_phase >= client->sine.size()){
            client->left_phase -= client->sine.size();
        }
        client->right_phase += 1; // higher pitch so we can distinguish left and right.
        if(client->right_phase >= client->sine.size()){
            client->right_phase -= client->sine.size();
        }
    }*/
    
    //==================================== DB METER STUFF ===========================================
    fill(client->levelsOut, client->levelsOut+MaxOutputs, -100.0f);
    float sums[MaxOutputs];
    fill(sums, sums+MaxOutputs, 0.0f);
    for(int i = 0; i < nframes; ++i) {
        for (int iSpeaker = 0; iSpeaker < MaxOutputs; iSpeaker++) {
            sums[iSpeaker] += outs[iSpeaker+1][i] * outs[iSpeaker+1][i];
        }
    }
    
    for (int iSpeaker = 0; iSpeaker < MaxOutputs; iSpeaker++) {
        client->levelsOut[iSpeaker] = 10.0f* log10( sqrt(sums[iSpeaker]/nframes));
    }
   /* for(int i = 0; i < nframes; ++i) {
        
        out1[i] = 0.0f;
        out2[i] = 0.0f;
        in1[i] = 0.0f;
        in2[i] = 0.0f;

    }*/
 /*          //envelope constants
        const float attack  = 0.05f;
        const float release = 100;
        const float ag      = powf(0.01f, 1000.f / (attack * client->sampleRate));
        const float rg      = powf(0.01f, 1000.f / (release * client->sampleRate));
        //for each speaker
    //get pointer to current spot in output buffer
    float *output = (jack_default_audio_sample_t*)jack_port_get_buffer (client->inputs[0], nframes);;
    float env = client->mLevels[0];
    
    //for each frame that are left to process
    for (unsigned int f = 0; f < client->bufferSize; f++) {
        float s = fabsf(out1[f]);
        float g = (s > env) ? ag : rg;
        env = g * env + (1.f - g) * 1;
    }

        for (int iSpeaker = 0; iSpeaker < 2; iSpeaker++) {
            client->mLevels[iSpeaker] = env;
            if(0==iSpeaker){
                cout << env<< endl;
            }
        }
    
    */
    //memcpy (in , out1, sizeof (jack_default_audio_sample_t) * nframes);
   // memcpy (in , out2, sizeof (jack_default_audio_sample_t) * nframes);
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
}

int sample_rate_callback(jack_nframes_t nframes, void *arg)
{
    printf("sample_rate_callback : %" PRIu32 "\n", nframes);
    return 0;
}

void latency_callback(jack_latency_callback_mode_t  mode, void *arg)
{
    switch (mode) {
        case JackCaptureLatency:
             printf("latency_callback : JackCaptureLatency %" PRIu32 "\n", mode);
            break;
         
        case JackPlaybackLatency:
            printf("latency_callback : JackPlaybackLatency %" PRIu32 "\n", mode);
            break;
        default:
            printf("latency_callback : ");
            break;
    }

}

void port_connect_callback(jack_port_id_t a, jack_port_id_t b, int connect, void* arg)
{
    printf("port_connect_callback : ");
    if(connect){
        printf("Connect ");
    }else{
        printf("Disconnect ");

    }
    printf("%" PRIu32 " <> %" PRIu32 "\n", a,b);
    
}


jackClientGris::jackClientGris() {
    clientReady = false;
    //--------------------------------------------------
    //open a client connection to the JACK server. Start server if it is not running.
    //--------------------------------------------------
    const char      **ports;
    const char      *client_name = "jackClientGris";
    const char      *server_name = "coreaudio";
    jack_options_t  options = JackUseExactName;
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
            //return;
        }
    }
    if (status & JackServerStarted) {
        fprintf (stderr, "\n===================\njackdmp wasn't running so it was started\n===================\n");
        //return;
    }
    if (status & JackNameNotUnique) {
        client_name = jack_get_client_name(client);
        fprintf (stderr, "\n\n\n======chosen name already existed, new unique name `%s' assigned\n", client_name);
        //return;
    }
    
    //--------------------------------------------------
    //register callback, ports
    //--------------------------------------------------
    jack_on_shutdown (client, jack_shutdown, 0);
    jack_set_process_callback (client, process_audio, this);
  
    jack_set_session_callback (client, session_callback, this);
    jack_set_port_connect_callback(client, port_connect_callback, this);
    jack_set_sample_rate_callback (client, sample_rate_callback, 0);
    //jack_set_latency_callback(client, latency_callback, this);
    
    jack_set_buffer_size(client, 1024);

    sampleRate = jack_get_sample_rate (client);
    bufferSize = jack_get_buffer_size (client);
    printf ("engine sample rate: %" PRIu32 "\n",sampleRate);
    printf ("engine buffer size: %" PRIu32 "\n",jack_get_buffer_size (client));

    //--------------------------------------------------
    //fill wave table.
    //--------------------------------------------------
    float fs = jack_get_sample_rate (client);
    float f  = 400.f / fs;
    float T  = 1/f;
    for(int i = 0; i < 10*T; ++i) {
        sine.push_back(0.2 * sin( i * M_PI * 2. * f ));
    }
    left_phase = right_phase = 0;
    
    this->inputs = vector<jack_port_t *>();
    for(int i = 0 ; i < MaxInputs ; i++){
        String nameOut = "input";
        nameOut+= String(i);
        this->inputs.push_back(jack_port_register(client,  nameOut.toUTF8(), JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput,  0));
    }
    
    //input_port   = jack_port_register (client,  "input",   JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput,  0);
    
    this->outputs = vector<jack_port_t *>();
    for(int i = 0 ; i < MaxOutputs ; i++){
        String nameOut = "output";
        nameOut+= String(i);
        this->outputs.push_back(jack_port_register(client,  nameOut.toUTF8(), JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput,  0));
    }
    

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
    for(int i = 0 ; i < MaxOutputs ; i++){
        if (jack_connect (client, jack_port_name (this->outputs[i]), ports[i])) {
            fprintf (stderr, "======     Cannot connect output ports %" PRIu32 "\n",i);
    
        }
    }
    
    
    ports = jack_get_ports (client, NULL, NULL, 0);
    for(int i = 1 ; i < MaxOutputs+MaxInputs ; i++){
        cout << ports[i] << newLine;
    }
    
    
    jack_free (ports);
    clientReady = true;
}


jackClientGris::~jackClientGris() {
    //jack_deactivate(client);
    for(int i = 0 ; i < MaxInputs ; i++){
        jack_port_unregister(client, this->inputs[i]);
    }
    
    for(int i = 0 ; i < MaxOutputs ; i++){
        jack_port_unregister(client, this->outputs[i]);
    }
    jack_client_close(client);
}
