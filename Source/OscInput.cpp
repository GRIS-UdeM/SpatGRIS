/*
 This file is part of SpatGRIS.

 Developers: Samuel Béland, Olivier Bélanger, Nicolas Masson

 SpatGRIS is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 SpatGRIS is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with SpatGRIS.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "OscInput.h"

#include "InputModel.h"
#include "MainComponent.h"

//==============================================================================
OscInput::~OscInput()
{
    disconnect();
}

//==============================================================================
bool OscInput::startConnection(int port)
{
    bool b = connect(port);
    addListener(this);
    return b;
}

//==============================================================================
void OscInput::oscBundleReceived(const juce::OSCBundle & bundle)
{
    for (auto const & element : bundle) {
        if (element.isMessage())
            oscMessageReceived(element.getMessage());
        else if (element.isBundle())
            oscBundleReceived(element.getBundle());
    }
}

//==============================================================================
void OscInput::oscMessageReceived(const juce::OSCMessage & message)
{
    mMainContentComponent.setOscLogging(message);
    auto const address{ message.getAddressPattern().toString().toStdString() };
    if (message[0].isInt32()) {
        if (address == OSC_SPAT_SERV) {
            // int id, float azi [0, 2pi], float ele [0, pi], float azispan [0, 2],
            // float elespan [0, 0.5], float distance [0, 1], float gain [0, 1].
            auto const idS{ message[0].getInt32() };
            juce::ScopedLock const lock{ mMainContentComponent.getInputsLock() };
            if (mMainContentComponent.getSourceInputs().size() > idS) {
                mMainContentComponent.getSourceInputs()[idS]->updateValues(
                    radians_t{ message[1].getFloat32() },
                    radians_t{ message[2].getFloat32() },
                    message[3].getFloat32(),
                    message[4].getFloat32(),
                    mMainContentComponent.isRadiusNormalized() ? 1.0f : message[5].getFloat32(),
                    message[6].getFloat32(),
                    mMainContentComponent.getModeSelected());
                mMainContentComponent.updateSourceData(idS, *mMainContentComponent.getSourceInputs()[idS]);
            }
        }

        else if (address == OSC_PAN_AZ) {
            // id, azim, elev, azimSpan, elevSpan, gain (Zirkonium artifact).
            auto const idS{ message[0].getInt32() };
            juce::ScopedLock const lock{ mMainContentComponent.getInputsLock() };
            if (mMainContentComponent.getSourceInputs().size() > idS) {
                mMainContentComponent.getSourceInputs()[idS]->updateValuesOld(message[1].getFloat32(),
                                                                              message[2].getFloat32(),
                                                                              message[3].getFloat32(),
                                                                              message[4].getFloat32(),
                                                                              message[5].getFloat32());
                mMainContentComponent.updateSourceData(idS, *mMainContentComponent.getSourceInputs()[idS]);
            }
        }
    } else if (message[0].isString()) {
        // string "reset", int voice_to_reset.
        juce::ScopedLock const lock{ mMainContentComponent.getInputsLock() };
        if (message[0].getString().compare("reset") == 0) {
            auto const idS{ message[1].getInt32() };
            if (mMainContentComponent.getSourceInputs().size() > idS) {
                mMainContentComponent.getSourceInputs()[idS]->resetPosition();
            }
        }
    }
}
