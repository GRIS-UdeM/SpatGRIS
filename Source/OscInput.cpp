/*
 This file is part of SpatGRIS2.

 Developers: Samuel Béland, Olivier Bélanger, Nicolas Masson

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

#include "OscInput.h"

#include "Input.h"
#include "MainComponent.h"

//==============================================================================
OscInput::~OscInput()
{
    this->disconnect();
}

//==============================================================================
bool OscInput::startConnection(int port)
{
    bool b = this->connect(port);
    this->addListener(this);
    return b;
}

//==============================================================================
void OscInput::oscBundleReceived(const juce::OSCBundle & bundle)
{
    for (auto & element : bundle) {
        if (element.isMessage())
            oscMessageReceived(element.getMessage());
        else if (element.isBundle())
            oscBundleReceived(element.getBundle());
    }
}

//==============================================================================
void OscInput::oscMessageReceived(const juce::OSCMessage & message)
{
    this->mMainContentComponent.setOscLogging(message);
    std::string address = message.getAddressPattern().toString().toStdString();
    if (message[0].isInt32()) {
        if (address == OSC_SPAT_SERV) {
            // int id, float azi [0, 2pi], float ele [0, pi], float azispan [0, 2],
            // float elespan [0, 0.5], float distance [0, 1], float gain [0, 1].
            unsigned int idS = message[0].getInt32();
            this->mMainContentComponent.getLockInputs().lock();
            if (this->mMainContentComponent.getListSourceInput().size() > idS) {
                this->mMainContentComponent.getListSourceInput()[idS]->updateValues(
                    message[1].getFloat32(),
                    message[2].getFloat32(),
                    message[3].getFloat32(),
                    message[4].getFloat32(),
                    this->mMainContentComponent.isRadiusNormalized() ? 1.0 : message[5].getFloat32(),
                    message[6].getFloat32(),
                    this->mMainContentComponent.getModeSelected());
                this->mMainContentComponent.updateInputJack(idS,
                                                            *this->mMainContentComponent.getListSourceInput()[idS]);
            }
            this->mMainContentComponent.getLockInputs().unlock();
        }

        else if (address == OSC_PAN_AZ) {
            // id, azim, elev, azimSpan, elevSpan, gain (Zirkonium artifact).
            unsigned int idS = message[0].getInt32();
            this->mMainContentComponent.getLockInputs().lock();
            if (this->mMainContentComponent.getListSourceInput().size() > idS) {
                this->mMainContentComponent.getListSourceInput()[idS]->updateValuesOld(message[1].getFloat32(),
                                                                                       message[2].getFloat32(),
                                                                                       message[3].getFloat32(),
                                                                                       message[4].getFloat32(),
                                                                                       message[5].getFloat32());
                this->mMainContentComponent.updateInputJack(idS,
                                                            *this->mMainContentComponent.getListSourceInput()[idS]);
            }
            this->mMainContentComponent.getLockInputs().unlock();
        }
    } else if (message[0].isString()) {
        // string "reset", int voice_to_reset.
        this->mMainContentComponent.getLockInputs().lock();
        if (message[0].getString().compare("reset") == 0) {
            unsigned int idS = message[1].getInt32();
            if (this->mMainContentComponent.getListSourceInput().size() > idS) {
                this->mMainContentComponent.getListSourceInput()[idS]->resetPosition();
            }
        }
        this->mMainContentComponent.getLockInputs().unlock();
    }
}
