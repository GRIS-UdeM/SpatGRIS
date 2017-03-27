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
    
    const unsigned int sizeInputs = client->inputsPort.size() ;
    const unsigned int sizeOutputs = client->outputsPort.size() ;
    
    //Get all buffer from all input - output
    jack_default_audio_sample_t * ins[sizeInputs];
    jack_default_audio_sample_t * outs[sizeOutputs];
    
    for (int i = 0; i < sizeInputs; i++) {
        ins[i] = (jack_default_audio_sample_t*)jack_port_get_buffer (client->inputsPort[i], nframes);
    }
    for (int i = 0; i < sizeOutputs; i++) {
        outs[i] = (jack_default_audio_sample_t*)jack_port_get_buffer (client->outputsPort[i], nframes);
    }
    
   

    //================ INPUTS ==========================================
    //Mute IN-----------------------------------
    for (int i = 0; i < sizeInputs; i++) {
        if(client->muteIn[i]){
            memset (ins[i], 0, sizeof (jack_default_audio_sample_t) * nframes);
        }
        if(client->soloIn[MaxInputs]){
            if(!client->soloIn[i]){
                memset (ins[i], 0, sizeof (jack_default_audio_sample_t) * nframes);
            }
        }
    }
    //---------------------------------------------
    float sumsIn[sizeInputs];
    fill(client->levelsIn, client->levelsIn+sizeInputs, -100.0f);
    fill(sumsIn, sumsIn+sizeInputs, 0.0f);
    
    for(int nF = 0; nF < nframes; ++nF) {
        for (int i = 0; i < sizeInputs; ++i) {
            sumsIn[i] +=  ins[i][nF] * ins[i][nF];
        }
    }
    for (int iSpeaker = 0; iSpeaker < sizeInputs; iSpeaker++) {
        client->levelsIn[iSpeaker] = sumsIn[iSpeaker]/nframes;
        
        //Basic Sound Transfert------------------(I -> O)
        if(iSpeaker < sizeOutputs){
            memcpy (outs[iSpeaker], ins[iSpeaker] , sizeof (jack_default_audio_sample_t) * nframes);
        }
    }
    
    
   
    //================ OUTPUTS =========================================
    //Mute Out--------------------------------
    for (int i = 0; i < sizeOutputs; i++) {
        if(client->muteOut[i]){
            memset (outs[i], 0, sizeof (jack_default_audio_sample_t) * nframes);
        }
        if(client->soloOut[MaxOutputs]){
            if(!client->soloOut[i]){
                memset (outs[i], 0, sizeof (jack_default_audio_sample_t) * nframes);
            }
        }
    }
    //-----------------------------------------
    float sumsOut[sizeOutputs];
    fill(client->levelsOut, client->levelsOut+sizeOutputs, -100.0f);
    fill(sumsOut, sumsOut+sizeOutputs, 0.0f);
    
    for(int nF = 0; nF < nframes; ++nF) {
        for (int i = 0; i < sizeInputs; ++i) {
            sumsOut[i] +=  outs[i][nF] * outs[i][nF];
        }
    }
    for (int iSpeaker = 0; iSpeaker < sizeInputs; iSpeaker++) {
        client->levelsOut[iSpeaker] = sumsOut[iSpeaker]/nframes;
    }
    
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

int graph_order_callback ( void * arg)
{
    printf ("graph_order_callback \n");
    return 0;
}

int xrun_callback ( void * arg)
{
    printf ("xrun_callback \n");
    return 0;
}

void jack_shutdown (void *arg)
{
    AlertWindow::showMessageBoxAsync(AlertWindow::WarningIcon, "FATAL ERROR", "Please check :\n - Buffer Size (1024)\n - Sample Rate(48000)\n - Inputs/Outputs");
    printf("\n===================\nFATAL ERROR JACK\n===================\n\n");
    exit(1);
}

int sample_rate_callback(jack_nframes_t nframes, void *arg)
{
    printf("sample_rate_callback : %" PRIu32 "\n", nframes);
    return 0;
}

void client_registration_callback(const char *name, int regist, void *arg)
{
    jackClientGris* client = (jackClientGris*)arg;
    printf("client_registration_callback : %s : " ,name);
    if(regist){
        client->nameClient.push_back(name);
        printf("saved\n");
    }else{
        for( vector<String>::iterator iter = client->nameClient.begin(); iter != client->nameClient.end(); ++iter )
        {
            if( *iter == String(name) )
            {
                client->nameClient.erase( iter );
                printf("deleted\n");
                break;
            }
        }
    }
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

void port_registration_callback ( jack_port_id_t a, int regist, void * arg)
{
    jackClientGris* client = (jackClientGris*)arg;
    printf("client_registration_callback : %" PRIu32 " : " ,a);
    if(regist){
        printf("saved \n");
    }else{
        printf("deleted\n");
    }
}


void port_connect_callback(jack_port_id_t a, jack_port_id_t b, int connect, void* arg)
{
    jackClientGris* client = (jackClientGris*)arg;
    printf("port_connect_callback : ");
    if(connect){
        //Stop Auto connection with system...
        if(!client->autoConnection){
            string nameClient = jack_port_name(jack_port_by_id(client->client,a));
            string tempN = jack_port_short_name(jack_port_by_id(client->client,a));
            nameClient =   nameClient.substr(0,nameClient.size()-(tempN.size()+1));
            if((nameClient != ClientName && nameClient!= ClientNameSys) || nameClient== ClientNameSys){
                jack_disconnect(client->client, jack_port_name(jack_port_by_id(client->client,a)), jack_port_name(jack_port_by_id(client->client,b)));
            }
        }
        printf("Connect ");
    }else{
        printf("Disconnect ");

    }
    printf("%" PRIu32 " <> %" PRIu32 "\n", a,b);
    return;
}


jackClientGris::jackClientGris() {
    
    //INIT variable and clear Array========================
    clientReady = false;
    autoConnection = false;
    nameClient =  vector<String >();
    
    fill(this->muteIn, this->muteIn+MaxInputs, false);
    fill(this->soloIn, this->soloIn+MaxInputs+1, false);
    fill(this->muteOut, this->muteOut+MaxOutputs, false);
    fill(this->soloOut, this->soloOut+MaxOutputs+1, false);
    
    
    this->inputsPort = vector<jack_port_t *>();
    this->outputsPort = vector<jack_port_t *>();
    //--------------------------------------------------
    //open a client connection to the JACK server. Start server if it is not running.
    //--------------------------------------------------
    jack_options_t  options = JackUseExactName;
    jack_status_t   status;
    
    client = jack_client_open (ClientName, options, &status, DriverNameSys);
    if (client == NULL) {
        
        printf("\nTry again...\n");

        options = JackServerName;
        client = jack_client_open (ClientName, options, &status, DriverNameSys);
        if (client == NULL) {
            printf("\n\n\n======jack_client_open() failed, status = 0x%2.0x\n", status);
            if (status & JackServerFailed) {
                printf("\n\n\n======Unable to connect to JACK server\n");
            }
        }
    }
    if (status & JackServerStarted) {
        printf("\n===================\njackdmp wasn't running so it was started\n===================\n");
    }
    if (status & JackNameNotUnique) {
        ClientName = jack_get_client_name(client);
        printf("\n\n\n======chosen name already existed, new unique name `%s' assigned\n", ClientName);
    }
    
    //--------------------------------------------------
    //register callback, ports
    //--------------------------------------------------
    jack_on_shutdown (client, jack_shutdown, this);
    jack_set_process_callback (client, process_audio, this);
    jack_set_client_registration_callback(client, client_registration_callback, this);
  
    jack_set_session_callback (client, session_callback, this);
    jack_set_port_connect_callback(client, port_connect_callback, this);
    jack_set_port_registration_callback(client, port_registration_callback, this);
    jack_set_sample_rate_callback (client, sample_rate_callback, this);
    
    jack_set_graph_order_callback(client, graph_order_callback, this);
    jack_set_xrun_callback(client, xrun_callback, this);
    //jack_set_latency_callback(client, latency_callback, this);
    
    jack_set_buffer_size(client, 1024);

    sampleRate = jack_get_sample_rate (client);
    bufferSize = jack_get_buffer_size (client);
    printf ("engine sample rate: %" PRIu32 "\n",sampleRate);
    printf ("engine buffer size: %" PRIu32 "\n",jack_get_buffer_size (client));

    //--------------------------------------------------
    // Activate client and connect the ports. Playback ports are "input" to the backend, and capture ports are "output" from it.
    //--------------------------------------------------
    if (jack_activate (client)) {
        printf("\n\n\n======cannot activate client");
        return;
    }
    
    
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

    
    //--------------------------------------------------
    //Print Inputs Ports available
    //--------------------------------------------------
    const char **ports = jack_get_ports (client, NULL, NULL, JackPortIsInput);
    if (ports == NULL) {
        printf("\n======NO Input PORTS\n");
        return;
    }
    numberInputs = 0;
    cout << newLine << "Ports I ================" << newLine << newLine;
    while (ports[numberInputs]){
        cout << ports[numberInputs] << newLine;
        numberInputs+=1;
    }
    jack_free (ports);
    cout << newLine << numberInputs <<" =====================" << newLine << newLine;
    
    
    //--------------------------------------------------
    //Print Outputs Ports available
    //--------------------------------------------------
    ports = jack_get_ports (client, NULL, NULL, JackPortIsOutput);
    if (ports == NULL) {
        printf("\n======NO Outputs PORTS\n");
        return;
    }
    numberOutputs = 0;
    cout << newLine << "Ports O ================" << newLine << newLine;
    while (ports[numberOutputs]){
        cout << ports[numberOutputs] << newLine;
        numberOutputs+=1;
    }
    jack_free (ports);
    cout << newLine << numberOutputs <<" =====================" << newLine << newLine;

    clientReady = true;
}

void jackClientGris::addRemoveInput(int number){
    
    if(number < this->inputsPort.size()){
        while(number < this->inputsPort.size()){
            jack_port_unregister(client, this->inputsPort.back());
            this->inputsPort.pop_back();
        }
    }else{
        while(number > this->inputsPort.size()){
            String nameIn = "input";
            nameIn+= String(this->inputsPort.size() +1);
            jack_port_t* newPort = jack_port_register(this->client,  nameIn.toUTF8(), JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput, 0);
            this->inputsPort.push_back(newPort);
        }
    }

}

bool jackClientGris::addOutput(){

    String nameOut = "output";
    nameOut+= String(this->outputsPort.size() +1);
    
    jack_port_t* newPort = jack_port_register(this->client,  nameOut.toUTF8(), JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput,  0);
    this->outputsPort.push_back(newPort);

    return true;
}


void jackClientGris::autoConnectClient()
{
    cout << jack_get_client_name(client) << endl;
    
    const char ** portsOut = jack_get_ports (client, NULL, JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput);
    const char ** portsIn = jack_get_ports (client, NULL, JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput);
    
    //Connect jackClientGris to system---------------------------------------------------
    int i=0;
    int j=0;
    while (portsOut[i]){
        string nameClient = getClientName(portsOut[i]);
        cout << jack_port_name(jack_port_by_name(client,portsOut[i])) << newLine;
        if(nameClient == ClientName)
        {
            while(portsIn[j]){
                nameClient = getClientName(portsIn[j]);
                if(nameClient == "system"){
                    jack_connect (client, portsOut[i] ,portsIn[j]);
                    j+=1;
                    break;
                }
                j+=1;
            }
        }
        i+=1;
    }
    
    //Connect other client to jackClientGris------------------------------------------
    i=0;
    j=0;
    autoConnection = true;
    while (portsOut[i]){
        string nameClient = getClientName(portsOut[i]);
        cout << jack_port_name(jack_port_by_name(client,portsOut[i])) << newLine;
        if(nameClient != ClientName && nameClient != "system")
        {
            while(portsIn[j]){
                nameClient = getClientName(portsIn[j]);
                if(nameClient == ClientName){
                    jack_connect (client, portsOut[i] ,portsIn[j]);
                    j+=1;
                    break;
                }
                j+=1;

            }
        }
        i+=1;
    }
    autoConnection = false;
    
    jack_free (portsIn);
    jack_free(portsOut);

   
}
bool jackClientGris::setBufferSize(int sizeB)
{
    return jack_set_buffer_size(this->client, sizeB);
}

string jackClientGris::getClientName(const char * port)
{
    if(port){
        string nameClient = jack_port_name(jack_port_by_name(client,port));
        string tempN = jack_port_short_name(jack_port_by_name(client,port));
        return  nameClient.substr(0,nameClient.size()-(tempN.size()+1));
    }return "";
}
jackClientGris::~jackClientGris() {
    jack_deactivate(client);
    for(int i = 0 ; i < this->inputsPort.size() ; i++){
        jack_port_unregister(client, this->inputsPort[i]);
    }
    
    for(int i = 0 ; i < this->outputsPort.size()  ; i++){
        jack_port_unregister(client, this->outputsPort[i]);
    }
    jack_client_close(client);
}
