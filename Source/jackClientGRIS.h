/*
 Copyright (C) 2001 Paul Davis
 
 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU Lesser General Public License as published by
 the Free Software Foundation; either version 2.1 of the License, or
 (at your option) any later version.
 
 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU Lesser General Public License for more details.
 
 You should have received a copy of the GNU Lesser General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef __jackClientGris__
#define __jackClientGris__

#include <stdlib.h>
#include <vector>
#include <stdio.h>
#include <math.h>

#include "jack/jack.h"
#include "jack/transport.h"

#ifndef M_PI
#define M_PI  (3.14159265)
#endif

//table size needs to fit an even number of periods. with a frequency of 1000hz and a sample rate of 44100, T = fs/f = 44100/1000
//#define TABLE_SIZE   (441)

class jackClientGris {
public:
    jack_client_t *client;
    jack_port_t *input_port, *output_port1, *output_port2;
    
    //    float sine[TABLE_SIZE];
    std::vector<double> sine;
    int left_phase;
    int right_phase;
    
    jackClientGris();
    virtual ~jackClientGris();
    
};


#endif
