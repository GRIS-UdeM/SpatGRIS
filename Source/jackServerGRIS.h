/*
 This file is part of ServerGris.
 
 Developers: Olivier Belanger, Nicolas Masson
 
 ServerGris is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.
 
 ServerGris is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with ServerGris.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef JACKSERVERGRIS_H
#define JACKSERVERGRIS_H

#include <jack/jack.h>
#include <jack/transport.h>
#include <jack/types.h>
#include <jack/session.h>
#include <jack/control.h>

class jackServerGRIS {
public:    
    jackServerGRIS(unsigned int rateV = 48000, unsigned int periodV = 1024);
    ~jackServerGRIS();

    jackctl_server_t *server;

private :
    unsigned int rateValue;
    unsigned int periodValue;
};

#endif /* JACKSERVERGRIS_H */
