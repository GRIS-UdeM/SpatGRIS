/*
 This file is part of SpatGRIS2.
 
 Developers: Olivier Belanger, Nicolas Masson
 
 SpatGRIS2 is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.
 
 SpatGRIS2 is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with SpatGRIS2.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef OSCINPUT_H
#define OSCINPUT_H

#include "../JuceLibraryCode/JuceHeader.h"

class MainContentComponent;

static const std::string OscPanAZ    = "/pan/az";
static const std::string OscSpatServ = "/spat/serv";

//==============================================================================
class OscInput : private juce::OSCReceiver,
                 private juce::OSCReceiver::Listener<juce::OSCReceiver::RealtimeCallback>

{
public :
    OscInput(MainContentComponent* parent);
    ~OscInput();
    //==============================================================================
    bool startConnection(int port);
    bool closeConnection();
private :
    //==============================================================================
    void oscMessageReceived(const OSCMessage& message) override;
    void oscBundleReceived(const OSCBundle& bundle) override;
    //==============================================================================
    MainContentComponent * mainParent;
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OscInput);
};

#endif /* OSCINPUT_H */
