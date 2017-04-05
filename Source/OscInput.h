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


#ifndef OscInput_h
#define OscInput_h

#include "../JuceLibraryCode/JuceHeader.h"

class MainContentComponent;

using namespace std;


class OscInput  :   private OSCReceiver,
                    private OSCReceiver::ListenerWithOSCAddress<OSCReceiver::RealtimeCallback>

{
public :
    OscInput(MainContentComponent* parent);
    ~OscInput();
    
    bool startConnection(int port);
    bool closeConnection();
    
private :
    void oscMessageReceived(const OSCMessage& message) override;
    MainContentComponent * mainParent;
    
};
#endif /* OscInput_h */
