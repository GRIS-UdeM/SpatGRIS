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

#ifndef OSCINPUT_H
#define OSCINPUT_H

#include "../JuceLibraryCode/JuceHeader.h"

class MainContentComponent;

using namespace std;

static const string OscPanAZ    = "/pan/az";
static const string OscSpatServ = "/spat/serv";

class OscInput : private OSCReceiver,
                 private OSCReceiver::Listener<OSCReceiver::RealtimeCallback>

{
public :
    OscInput(MainContentComponent* parent);
    ~OscInput();
    
    bool startConnection(int port);
    bool closeConnection();
    
private :
    void oscMessageReceived(const OSCMessage& message) override;
    void oscBundleReceived(const OSCBundle& bundle) override;

    MainContentComponent * mainParent;
    
};

#endif /* OSCINPUT_H */
