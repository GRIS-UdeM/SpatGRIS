//
//  OscInput.h
//  spatServerGRIS
//
//  Created by GRIS on 2017-03-16.
//
//

#ifndef OscInput_h
#define OscInput_h

#include <stdio.h>
#include "../JuceLibraryCode/JuceHeader.h"

using namespace std;
class OscInput  :   private OSCReceiver,
                    private OSCReceiver::ListenerWithOSCAddress<OSCReceiver::RealtimeCallback>

{
public :
    OscInput(){
    }
    ~OscInput(){
        this->disconnect();
    }
             
    
    bool startConnection(int port){
        bool b = this->connect(port);
        this->addListener(this, "/pan/az");
        return b;
        
    }
    
    bool closeConnection(){
        return this->disconnect();
    }
    
private :
    void oscMessageReceived(const OSCMessage& message) override {
        string address = message.getAddressPattern().toString().toStdString();
        if(address == ("/pan/az")){
            for (int i = 0; i < message.size(); i++){
                if(i==0){
                    cout << message[i].getInt32();
                }
                else{
                    cout << message[i].getFloat32();
                }
                cout << " < ";
            }
            cout<<newLine;
        }
    }
    
};
#endif /* OscInput_h */
