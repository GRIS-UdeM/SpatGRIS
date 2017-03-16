//
//  OscInput.h
//  spatServerGRIS
//
//  Created by GRIS on 2017-03-16.
//
//

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
