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

#ifndef __jackClientGris__
#define __jackClientGris__

#include <stdlib.h>
#include <vector>
#include <stdio.h>
#include <math.h>

#include "../JuceLibraryCode/JuceHeader.h"


#include "jack/jack.h"
#include "jack/transport.h"
#include "jack/types.h"
#include "jack/session.h"



#ifndef M_PI
#define M_PI  (3.14159265)
#endif

//table size needs to fit an even number of periods. with a frequency of 1000hz and a sample rate of 44100, T = fs/f = 44100/1000
//#define TABLE_SIZE   (441)
using namespace std;


static unsigned int const MaxInputs = 64;
static unsigned int const MaxOutputs = 64;

class jackClientGris {
public:

    jack_client_t *client;

    vector<jack_port_t *> inputs;
    vector<jack_port_t *> outputs;

    
    vector<double> sine;
    vector<int> outPut;
    int left_phase;
    int right_phase;
    
    float levelsIn[MaxInputs];
    float levelsOut[MaxOutputs];

    unsigned int sampleRate;
    unsigned int bufferSize;
    
    bool isReady() { return clientReady; }
    float getCpuUsed() const { return jack_cpu_load(client); }
    float getLevelsIn(int index) const { return levelsIn[index]; }
    float getLevelsOut(int index) const { return levelsOut[index]; }

    
    jackClientGris();
    virtual ~jackClientGris();
    
    
    void addRemoveInput(int number);
    void addOutput();
    
private:
    bool clientReady;

    
};


#endif
