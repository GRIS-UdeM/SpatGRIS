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
            source_index_t const sourceIndex{ message[0].getInt32() };
            juce::ScopedLock const lock{ mMainContentComponent.getCriticalSection() };
            auto const & data{ mMainContentComponent.getData() };
            if (data.project.sources.contains(sourceIndex)) {
                radians_t const azimuth{ message[1].getFloat32() };
                radians_t const zenith{ message[2].getFloat32() };
                auto const azimuthSpan{ message[3].getFloat32() };
                auto const zenithSpan{ message[4].getFloat32() };
                auto const length{ message[5].getFloat32() };
                [[maybe_unused]] auto const gain{ message[6].getFloat32() };

                PolarVector const vector{ azimuth, zenith, length };

                mMainContentComponent.handleSourcePositionChanged(, vector, azimuthSpan, zenithSpan);
            }
        }

        else if (address == OSC_PAN_AZ) {
            // id, azim, elev, azimSpan, elevSpan, gain (Zirkonium artifact).

            // DEPRECATED
            jassertfalse;
        }
    } else if (message[0].isString()) {
        if (message[0].getString().compare("reset") == 0) {
            // string "reset", int voice_to_reset.
            source_index_t const sourceIndex{ message[1].getInt32() };
            mMainContentComponent.resetSourcePosition(sourceIndex);
        }
    }
}
