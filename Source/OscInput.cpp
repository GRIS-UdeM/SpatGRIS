//
//  OscInput.cpp
//  spatServerGRIS
//
//  Created by GRIS on 2017-03-16.
//
//

#include "OscInput.h"
#include "MainComponent.h"
#include "Input.h"

OscInput::OscInput(MainContentComponent* parent){
    this->mainParent = parent;
}

OscInput::~OscInput(){
    this->disconnect();
}

bool OscInput::startConnection(int port){
    bool b = this->connect(port);
    this->addListener(this, "/pan/az");
    return b;
}

bool OscInput::closeConnection(){
    return this->disconnect();
}

void OscInput::oscMessageReceived(const OSCMessage& message) {
    string address = message.getAddressPattern().toString().toStdString();
    if(address == ("/pan/az") && message[0].isInt32()){
        
        int idS = message[0].getInt32();
        this->mainParent->getLockInputs()->lock();
        if(this->mainParent->getListSourceInput().size() > idS){
            this->mainParent->getListSourceInput()[idS]->updateValues(message[1].getFloat32(),
                                                                      message[2].getFloat32(),
                                                                      message[3].getFloat32(),
                                                                      message[4].getFloat32(),
                                                                      message[5].getFloat32());
            this->mainParent->updateInputJack(idS, *this->mainParent->getListSourceInput()[idS]);
        }
        this->mainParent->getLockInputs()->unlock();
    }
}
