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

#include <jack/jack.h>

#ifndef M_PI
#define M_PI  (3.14159265)
#endif

#define TABLE_SIZE   (200)

typedef struct {
    float sine[TABLE_SIZE];
    int left_phase;
    int right_phase;
}
paTestData;


int         process (jack_nframes_t nframes, void *arg);
static void signal_handler(int sig);
void        jack_shutdown (void *arg);

class jackClientGRIS {
public:
    jackClientGRIS();
    ~jackClientGRIS();
};
