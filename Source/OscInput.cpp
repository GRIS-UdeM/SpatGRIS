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

#include "OscInput.h"
#include "MainComponent.h"
#include "Input.h"

OscInput::OscInput(MainContentComponent* parent) {
    this->mainParent = parent;
}

OscInput::~OscInput() {
    this->disconnect();
}

bool OscInput::startConnection(int port) {
    bool b = this->connect(port);
    this->addListener(this, OscPanAZ.c_str());
    this->addListener(this, OscSpatServ.c_str());
    return b;
}

bool OscInput::closeConnection() {
    return this->disconnect();
}

void OscInput::oscMessageReceived(const OSCMessage& message) {
    string address = message.getAddressPattern().toString().toStdString();
    if (message[0].isInt32()) {
        if (address == OscSpatServ) {
            // int id, float azi [0, 2pi], float ele [0, pi], float azispan [0, 2],
            // float elespan [0, 0.5], float radius [0, 1], float gain [0, 1].
            unsigned int idS = message[0].getInt32();
            this->mainParent->getLockInputs()->lock();
            if (this->mainParent->getListSourceInput().size() > idS) {
                this->mainParent->getListSourceInput()[idS]->updateValues(message[1].getFloat32(),
                                                                          message[2].getFloat32(),
                                                                          message[3].getFloat32(),
                                                                          message[4].getFloat32(),
                                                                          this->mainParent->isRadiusNormalized() ? 1.0 : message[5].getFloat32(),
                                                                          message[6].getFloat32(),
                                                                          this->mainParent->getModeSelected());
                this->mainParent->updateInputJack(idS, *this->mainParent->getListSourceInput()[idS]);
            }
            this->mainParent->getLockInputs()->unlock();
        }
        
        else if (address == OscPanAZ) {
            //id, azim, elev, azimSpan, elevSpan, gain (Zirkonium artifact).
            unsigned int idS = message[0].getInt32();
            this->mainParent->getLockInputs()->lock();
            if (this->mainParent->getListSourceInput().size() > idS) {
                this->mainParent->getListSourceInput()[idS]->updateValuesOld(message[1].getFloat32(),
                                                                             message[2].getFloat32(),
                                                                             message[3].getFloat32(),
                                                                             message[4].getFloat32(),
                                                                             message[5].getFloat32());
                this->mainParent->updateInputJack(idS, *this->mainParent->getListSourceInput()[idS]);
            }
            this->mainParent->getLockInputs()->unlock();
        }
    } else if (message[0].isString()) {
        // string "reset", int voice_to_reset.
        this->mainParent->getLockInputs()->lock();
        if (message[0].getString().compare("reset") == 0) {
            unsigned int idS = message[1].getInt32();
            if (this->mainParent->getListSourceInput().size() > idS) {
                this->mainParent->getListSourceInput()[idS]->resetPosition();
            }
        }
        this->mainParent->getLockInputs()->unlock();
    }
}
