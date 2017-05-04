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

#include "jackClientGRIS.h"


//=========================================================================================
//MUTE SOLO MasterGainOut and NOISE
//=========================================================================================
static void muteSoloVuMeterIn(jackClientGris & jackCli, jack_default_audio_sample_t ** ins, const jack_nframes_t &nframes, const unsigned int &sizeInputs){
    //Mute & solo --------------------------------
    for (int i = 0; i < sizeInputs; ++i) {
        if(jackCli.muteIn[i]){
            memset (ins[i], 0, sizeof (jack_default_audio_sample_t) * nframes);
        }
        if(jackCli.soloIn[MaxInputs]){
            if(!jackCli.soloIn[i]){
                memset (ins[i], 0, sizeof (jack_default_audio_sample_t) * nframes);
            }
        }
    }
    
    //Vu meter-----------------------------------
    float sumsIn[sizeInputs];
    fill(jackCli.levelsIn, jackCli.levelsIn+sizeInputs, -60.0f);
    fill(sumsIn, sumsIn+sizeInputs, 0.0f);
    
    for (int i = 0; i < sizeInputs; ++i) {
        for(int nF = 0; nF < nframes; ++nF) {
            sumsIn[i] +=  ins[i][nF] * ins[i][nF];
        }
        jackCli.levelsIn[i] = sumsIn[i]/nframes;
    }
    
}

static void muteSoloVuMeterGainOut(jackClientGris & jackCli, jack_default_audio_sample_t ** outs, const jack_nframes_t &nframes, const unsigned int &sizeOutputs, const float mGain = 1.0f){
    //Mute & solo --------------------------------
    for (int i = 0; i < sizeOutputs; ++i) {
        if(jackCli.muteOut[i]){
            memset (outs[i], 0, sizeof (jack_default_audio_sample_t) * nframes);
        }
        if(jackCli.soloOut[MaxOutputs]){
            if(!jackCli.soloOut[i]){
                memset (outs[i], 0, sizeof (jack_default_audio_sample_t) * nframes);
            }
        }
    }
    
    //Vu meter-----------------------------------
    float sumsOut[sizeOutputs];
    fill(jackCli.levelsOut, jackCli.levelsOut+sizeOutputs, -60.0f);
    fill(sumsOut, sumsOut+sizeOutputs, 0.0f);
    

    for (int i = 0; i < sizeOutputs; ++i) {
        for(int nF = 0; nF < nframes; ++nF) {
            //Gain volume
            outs[i][nF] *= mGain;
            //cout << outs[i][nF] << newLine;
            sumsOut[i] +=  outs[i][nF] * outs[i][nF];
        }
        jackCli.levelsOut[i] = sumsOut[i]/nframes;
    }

}

static void addNoiseSound(jackClientGris & jackCli, jack_default_audio_sample_t ** ins, const jack_nframes_t &nframes, const unsigned int &sizeInputs){
    for(int nF = 0; nF < nframes; ++nF) {
        for (int i = 0; i < sizeInputs; i++) {
            if(i%2==0){
                ins[i][nF] += jackCli.sineNoise[jackCli.left_phase];
            }else{
                ins[i][nF] += jackCli.sineNoise[jackCli.right_phase];
            }
        }
        jackCli.left_phase += 1;
        if (jackCli.left_phase >= jackCli.sineNoise.size()){
            jackCli.left_phase -= jackCli.sineNoise.size();
        }
        jackCli.right_phase += 2; // higher pitch so we can distinguish left and right.
        if(jackCli.right_phase >= jackCli.sineNoise.size()){
            jackCli.right_phase -= jackCli.sineNoise.size();
        }
    }
}

//=========================================================================================
//FREE VOLUME PROCESS (Basic)
//=========================================================================================
static void processFreeVolume(jackClientGris & jackCli, jack_default_audio_sample_t ** ins, jack_default_audio_sample_t ** outs, const jack_nframes_t &nframes, const unsigned int &sizeInputs, const unsigned int &sizeOutputs){
    
    float outputX, outputY, dx, dy, da;
    unsigned int i,o,f;
    
    //Basix Free volume Spat---------------------------------------
    for (o = 0; o < sizeOutputs; ++o) {
        
        outputX = jackCli.listSpeakerOut[o].x;
        outputY = jackCli.listSpeakerOut[o].z;
        
        //Process Input 1-------------------------------------
        dx = jackCli.listSourceIn[0].x - outputX;
        dy = jackCli.listSourceIn[0].z - outputY;
        da = sqrtf(dx*dx + dy*dy);
        
        if (da > 1.0f) da = 1.0f;
        if (da < 0.1f) da = 0.1f;
        
        da = -log10f(da);
        for (f = 0; f < nframes; ++f){
            outs[o][f] = da * ins[0][f];        //Diff Input 1
        }
        
        //Process Other Input -----------------------------------
        for (i = 1; i < sizeInputs; ++i) {
            
            dx = jackCli.listSourceIn[i].x - outputX;
            dy = jackCli.listSourceIn[i].z - outputY;
            da = sqrtf(dx*dx + dy*dy);
            
            if (da > 1.0f) da = 1.0f;
            if (da < 0.1f) da = 0.1f;
            
            da = -log10f(da);
            for (f = 0; f < nframes; ++f){
                outs[o][f] += da * ins[i][f];   //Other input
            }
        }
    }
}


//=========================================================================================
//MASTER PROCESS
//=========================================================================================
static int process_audio (jack_nframes_t nframes, void* arg) {

    jackClientGris* jackCli = (jackClientGris*)arg;
    
    //================ Return if user edit speaker ==============================
    if(!jackCli->processBlockOn){
        for (int i = 0; i < jackCli->outputsPort.size(); ++i) {
            memset (((jack_default_audio_sample_t*)jack_port_get_buffer (jackCli->outputsPort[i], nframes)), 0, sizeof (jack_default_audio_sample_t) * nframes);
            jackCli->levelsOut[i] = -60.0f;
        }
        return 0;
    }
    
    //================ LOAD BUFFER ============================================
    const unsigned int sizeInputs = (unsigned int)jackCli->inputsPort.size() ;
    const unsigned int sizeOutputs = (unsigned int)jackCli->outputsPort.size() ;
    
    
    //Get all buffer from all input - output
    jack_default_audio_sample_t * ins[sizeInputs];
    jack_default_audio_sample_t * outs[sizeOutputs];
    
    for (int i = 0; i < sizeInputs; i++) {
        ins[i] = (jack_default_audio_sample_t*)jack_port_get_buffer (jackCli->inputsPort[i], nframes);
    }
    for (int i = 0; i < sizeOutputs; i++) {
        outs[i] = (jack_default_audio_sample_t*)jack_port_get_buffer (jackCli->outputsPort[i], nframes);
    }
    
    
    //NoiseSound-----------------------------------------------
    if(jackCli->noiseSound){
        addNoiseSound(*jackCli, ins, nframes, sizeInputs);
    }

    //================ INPUTS ===============================================
    muteSoloVuMeterIn(*jackCli, ins, nframes, sizeInputs);
    //---------------------------------------------

    
    
    //================ PROCESS ==============================================
    
    switch ((ModeSpatEnum)jackCli->modeSelected){
        case FreeBasic:
            //Basix Free volume Spat---------------------------------------
            processFreeVolume(*jackCli, ins, outs, nframes, sizeInputs, sizeOutputs);
            break;
            
        case VBap:
            break;
            
        case DBap:
            break;
            
        case HRTF:
            break;
        
        default:
            jassertfalse;
            break;
    }

    
    //Basic Sound Transfert (I -> O) --------------------------------
    /*for (int iSpeaker = 0; iSpeaker < sizeInputs; iSpeaker++) {
        if(iSpeaker < sizeOutputs){
            memcpy (outs[iSpeaker], ins[iSpeaker] , sizeof (jack_default_audio_sample_t) * nframes);
        }
    }*/
    
    
    //================ OUTPUTS ==============================================
    muteSoloVuMeterGainOut(*jackCli, outs, nframes, sizeOutputs, jackCli->masterGainOut);
    //-----------------------------------------
    
    
    jackCli->overload = false;
    return 0;
}


//=========================================================================================
//CALLBACK FUNCTION
//=========================================================================================
void session_callback (jack_session_event_t *event, void *arg)
{
    jackClientGris* jackCli = (jackClientGris*)arg;
    
    char retval[100];
    printf ("session notification\n");
    printf ("path %s, uuid %s, type: %s\n", event->session_dir, event->client_uuid, event->type == JackSessionSave ? "save" : "quit");
    
    snprintf (retval, 100, "jack_simple_session_client %s", event->client_uuid);
    event->command_line = strdup (retval);
    
    jack_session_reply(jackCli->client, event);
    
    jack_session_event_free (event);
}

int graph_order_callback ( void * arg)
{
    jackClientGris* jackCli = (jackClientGris*)arg;
    jackCli->updateClientPortAvailable();
    printf ("graph_order_callback \n");
    return 0;
}

int xrun_callback ( void * arg)
{
    jackClientGris* jackCli = (jackClientGris*)arg;
    jackCli->overload = true;
    printf ("xrun_callback \n");
    return 0;
}

void jack_shutdown (void *arg)
{
    AlertWindow::showMessageBoxAsync(AlertWindow::WarningIcon, "FATAL ERROR", "Please check :\n - Buffer Size\n - Sample Rate\n - Inputs/Outputs");
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
    jackClientGris* jackCli = (jackClientGris*)arg;
    printf("client_registration_callback : %s : " ,name);
    if(regist){
        Client cli;
        cli.name = name;

        jackCli->listClient.push_back(cli);
        printf("saved\n");
    }else{
        for( vector<Client>::iterator iter = jackCli->listClient.begin(); iter != jackCli->listClient.end(); ++iter )
        {
            if( iter->name == String(name) )
            {
                jackCli->listClient.erase( iter );
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

void port_registration_callback (jack_port_id_t a, int regist, void * arg)
{
    //jackClientGris* jackCli = (jackClientGris*)arg;
    printf("client_registration_callback : %" PRIu32 " : " ,a);
    if(regist){
        printf("saved \n");
    }else{
        printf("deleted\n");
    }
}


void port_connect_callback(jack_port_id_t a, jack_port_id_t b, int connect, void* arg)
{
    jackClientGris* jackCli = (jackClientGris*)arg;
    printf("port_connect_callback : ");
    if(connect){
        //Stop Auto connection with system...
        if(!jackCli->autoConnection){
            string nameClient = jack_port_name(jack_port_by_id(jackCli->client,a));
            string tempN = jack_port_short_name(jack_port_by_id(jackCli->client,a));
            nameClient =   nameClient.substr(0,nameClient.size()-(tempN.size()+1));
            if((nameClient != ClientName && nameClient!= ClientNameSys) || nameClient== ClientNameSys){
                jack_disconnect(jackCli->client, jack_port_name(jack_port_by_id(jackCli->client,a)), jack_port_name(jack_port_by_id(jackCli->client,b)));
            }
        }
        printf("Connect ");
    }else{
        printf("Disconnect ");
    }
    printf("%" PRIu32 " <> %" PRIu32 "\n", a,b);
    return;
}




//=================================================================================================================
// jackClientGris
//=================================================================================================================
jackClientGris::jackClientGris(unsigned int bufferS) {
    
    //INIT variable and clear Array========================
    this->noiseSound = false;
    this->clientReady = false;
    this->autoConnection = false;
    this->overload = false;
    this->masterGainOut = 1.0f;
    this->processBlockOn = true;
    this->modeSelected = FreeBasic;
    
    this->listClient = vector<Client>();
    /*this->listSourceIn = vector<SourceIn>();
    this->listSpeakerOut = vector<SpeakerOut>();
    
    SourceIn si;
    for(int i = 0; i < MaxInputs; ++i){
        this->listSourceIn.push_back(si);
    }
    SpeakerOut so;
    for(int i = 0; i < MaxOutputs; ++i){
        this->listSpeakerOut.push_back(so);
    }*/
    
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
    
    printf("\n========================== \n");
    printf("Start Jack Client \n");
    printf("========================== \n");
    
    this->client = jack_client_open (ClientName, options, &status, DriverNameSys);
    if (this->client == NULL) {
        
        printf("\nTry again...\n");

        options = JackServerName;
        this->client = jack_client_open (ClientName, options, &status, DriverNameSys);
        if (this->client == NULL) {
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
        ClientName = jack_get_client_name(this->client);
        printf("\n\n\n======chosen name already existed, new unique name `%s' assigned\n", ClientName);
    }
    
    //--------------------------------------------------
    //register callback, ports
    //--------------------------------------------------
    jack_on_shutdown                        (this->client, jack_shutdown, this);
    jack_set_process_callback               (this->client, process_audio, this);
    jack_set_client_registration_callback   (this->client, client_registration_callback, this);
  
    jack_set_session_callback               (this->client, session_callback, this);
    jack_set_port_connect_callback          (this->client, port_connect_callback, this);
    jack_set_port_registration_callback     (this->client, port_registration_callback, this);
    jack_set_sample_rate_callback           (this->client, sample_rate_callback, this);
    
    jack_set_graph_order_callback           (this->client, graph_order_callback, this);
    jack_set_xrun_callback                  (this->client, xrun_callback, this);
    jack_set_latency_callback               (this->client, latency_callback, this);
    
    //Default buffer size 1024
    jack_set_buffer_size(this->client, bufferS);
    
    sampleRate = jack_get_sample_rate (this->client);
    bufferSize = jack_get_buffer_size (this->client);
    
    printf ("engine sample rate: %" PRIu32 "\n", sampleRate);
    printf ("engine buffer size: %" PRIu32 "\n", jack_get_buffer_size(this->client));
    
    
    //--------------------------------------------------
    //fill wave table.
    //--------------------------------------------------
    float fs = jack_get_sample_rate (this->client);
    float f  = 400.f / fs;
    float T  = 1/f;
    for(int i = 0; i < 10*T; ++i) {
        this->sineNoise.push_back(0.2 * sin( i * M_PI * 2. * f ));
    }
    this->left_phase = this->right_phase = 0;

    
    //--------------------------------------------------
    //Print Inputs Ports available
    //--------------------------------------------------
    const char **ports = jack_get_ports (this->client, NULL, NULL, JackPortIsInput);
    if (ports == NULL) {
        printf("\n======NO Input PORTS\n");
        return;
    }
    this->numberInputs = 0;
    cout << newLine << "Ports I ================" << newLine << newLine;
    while (ports[this->numberInputs]){
        cout << ports[this->numberInputs] << newLine;
        this->numberInputs+=1;
    }
    jack_free (ports);
    cout << newLine << this->numberInputs <<" =====================" << newLine << newLine;
    
    
    //--------------------------------------------------
    //Print Outputs Ports available
    //--------------------------------------------------
    ports = jack_get_ports (client, NULL, NULL, JackPortIsOutput);
    if (ports == NULL) {
        printf("\n======NO Outputs PORTS\n");
        return;
    }
    this->numberOutputs = 0;
    cout << newLine << "Ports O ================" << newLine << newLine;
    while (ports[this->numberOutputs]){
        cout << ports[this->numberOutputs] << newLine;
        this->numberOutputs+=1;
    }
    jack_free (ports);
    cout << newLine << this->numberOutputs <<" =====================" << newLine << newLine;

    
    //--------------------------------------------------
    // Activate client and connect the ports. Playback ports are "input" to the backend, and capture ports are "output" from it.
    //--------------------------------------------------
    if (jack_activate (this->client)) {
        printf("\n\n\n======cannot activate client");
        return;
    }
    
    printf("\n========================== \n");
    printf("Jack Client Run \n");
    printf("========================== \n");
    
    this->clientReady = true;
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
    
    connectedGristoSystem();
}

void jackClientGris::clearOutput(){
    int outS = (int)this->outputsPort.size();
    for(int i = 0; i < outS; i++){
        jack_port_unregister(client, this->outputsPort.back());
        this->outputsPort.pop_back();
    }
}

bool jackClientGris::addOutput(){
    String nameOut = "output";
    nameOut+= String(this->outputsPort.size() +1);
    
    jack_port_t* newPort = jack_port_register(this->client,  nameOut.toUTF8(), JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput,  0);
    this->outputsPort.push_back(newPort);
    connectedGristoSystem();
    return true;
}

void jackClientGris::removeOutput(int number){
    jack_port_unregister(client, this->outputsPort.at(number));
    this->outputsPort.erase(this->outputsPort.begin()+number);
}

void jackClientGris::connectedGristoSystem(){
    const char ** portsOut = jack_get_ports (this->client, NULL, JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput);
    const char ** portsIn = jack_get_ports (this->client, NULL, JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput);
    
    int i=0;
    int j=0;
    //DisConnect jackClientGris to system---------------------------------------------------
    while (portsOut[i]){
        if(getClientName(portsOut[i]) == ClientName)    //jackClient
        {
            j=0;
            while(portsIn[j]){
                if(getClientName(portsIn[j]) == ClientNameSys){ //system
                    jack_disconnect(this->client, portsOut[i] ,portsIn[j]);
                }
                j+=1;
            }
        }
        i+=1;
    }
    
    i=0;
    j=0;
    //Connect jackClientGris to system---------------------------------------------------
    while (portsOut[i]){
        if(getClientName(portsOut[i]) == ClientName)    //jackClient
        {
            while(portsIn[j]){
                if(getClientName(portsIn[j]) == ClientNameSys){ //system
                    jack_connect (this->client, portsOut[i] ,portsIn[j]);
                    j+=1;
                    break;
                }
                j+=1;
            }
        }
        i+=1;
    }
    jack_free(portsIn);
    jack_free(portsOut);

}


void jackClientGris::autoConnectClient()
{
    connectedGristoSystem();
    
    const char ** portsOut = jack_get_ports (this->client, NULL, JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput);
    const char ** portsIn = jack_get_ports (this->client, NULL, JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput);
    
    int i=0;
    int j=0;
    int startJ = 0;
    int endJ = 0;
    
    //Connect other client to jackClientGris------------------------------------------
    this->autoConnection = true;
    
    for (auto&& cli : this->listClient)
    {
        i=0;
        j=0;
        
        String nameClient = cli.name;
        startJ = cli.portStart-1;
        endJ = cli.portEnd;
        while (portsOut[i]){
            if(nameClient == getClientName(portsOut[i]))
            {
                //cout << jack_port_name(jack_port_by_name(client,portsOut[i])) << newLine;
                while(portsIn[j]){
                    if(getClientName(portsIn[j]) == ClientName){
                        if(j>= startJ && j<endJ){
                            jack_connect (this->client, portsOut[i] ,portsIn[j]);
                            cli.connected = true;
                            j+=1;
                            break;
                        }else{
                            j+=1;
                        }
                    }else{
                        j+=1;
                        startJ+=1;
                        endJ+=1;
                    }
                }
            }
            i+=1;
        }
    }
    
    this->autoConnection = false;
    
    jack_free(portsIn);
    jack_free(portsOut);
}

void jackClientGris::connectionClient(String name, bool connect){
    const char ** portsOut = jack_get_ports (this->client, NULL, JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput);
    const char ** portsIn = jack_get_ports (this->client, NULL, JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput);

    int i=0;
    int j=0;
    int startJ = 0;
    int endJ = 0;
    bool conn = false;
    this->updateClientPortAvailable();
    //Disconencted Client------------------------------------------------
    while (portsOut[i]){
        if(getClientName(portsOut[i]) == name)
        {
            j = 0;
            while(portsIn[j]){
                if(getClientName(portsIn[j]) == ClientName){ //jackClient
                    jack_disconnect(this->client, portsOut[i] ,portsIn[j]);
                }
                j+=1;
            }
        }
        i+=1;
    }
    for (auto&& cli : this->listClient)
    {
        if(cli.name == name){
            cli.connected = false;
        }
    }
    if(!connect){ return ; }
    //Connect other client to jackClientGris------------------------------------------
    this->autoConnection = true;
    for (auto&& cli : this->listClient)
    {
        i=0;
        j=0;
        String nameClient = cli.name;
        startJ = cli.portStart-1;
        endJ = cli.portEnd;
        while (portsOut[i]){
            if(nameClient == name && nameClient == getClientName(portsOut[i]))
            {
                //cout << jack_port_name(jack_port_by_name(client,portsOut[i])) << newLine;
                while(portsIn[j]){
                    if(getClientName(portsIn[j]) == ClientName){
                        if(j>= startJ && j<endJ){
                            jack_connect (this->client, portsOut[i] ,portsIn[j]);
                            conn = true;
                            j+=1;
                            break;
                        }else{
                            j+=1;
                        }
                    }else{
                        j+=1;
                        startJ+=1;
                        endJ+=1;
                    }
                }
                cli.connected = conn;
            }
            i+=1;
        }
    }
    
    this->autoConnection = false;
    
    jack_free(portsIn);
    jack_free(portsOut);
}


string jackClientGris::getClientName(const char * port)
{
    if(port){
        string nameClient = jack_port_name(jack_port_by_name(this->client, port));
        string tempN = jack_port_short_name(jack_port_by_name(this->client, port));
        return  nameClient.substr(0,nameClient.size()-(tempN.size()+1));
    }return "";
}

void jackClientGris::updateClientPortAvailable(){
    const char ** portsOut = jack_get_ports (this->client, NULL, JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput);
    int i = 0;
    for (auto&& cli : this->listClient)
    {
        cli.portAvailable = 0;
    }
    while (portsOut[i]){
        string nameCli = getClientName(portsOut[i]);
        if(nameCli != ClientName &&  nameCli != ClientNameSys){
            for (auto&& cli : this->listClient)
            {
                if(cli.name == nameCli){
                    cli.portAvailable+=1;
                }
            }
        }
        i++;
    }
    jack_free(portsOut);
}

unsigned int jackClientGris::getPortStartClient(String nameClient)
{
    for (auto&& it : this->listClient)
    {
        if(it.name==nameClient){
            return it.portStart;
        }
    }
    return 0;
}
jackClientGris::~jackClientGris() {
    jack_deactivate(this->client);
    for(int i = 0 ; i < this->inputsPort.size() ; i++){
        jack_port_unregister(this->client, this->inputsPort[i]);
    }
    
    for(int i = 0 ; i < this->outputsPort.size()  ; i++){
        jack_port_unregister(this->client, this->outputsPort[i]);
    }
    jack_client_close(this->client);
}
