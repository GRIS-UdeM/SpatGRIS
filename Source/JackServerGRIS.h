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

#pragma once

#include "macros.h"

DISABLE_WARNINGS
#include <JuceHeader.h>
ENABLE_WARNINGS

#if USE_JACK
    #include <jack/control.h>
    #include <jack/jack.h>
    #include <jack/session.h>
    #include <jack/transport.h>
    #include <jack/types.h>
#else
    #include "JackMockup.h"
#endif

//==============================================================================
class JackServerGris
{
public:
    JackServerGris(unsigned int rateV = 48000,
                   unsigned int periodV = 1024,
                   String alsaOutputDevice = String(),
                   int * errorCode = nullptr);
    ~JackServerGris();
    //==============================================================================
    // Only effective with alsa driver.
    juce::Array<juce::String> getAvailableOutputDevices() const;
    //==============================================================================
    jackctl_server_t * server;

private:
    //==============================================================================
    unsigned int rateValue;
    unsigned int periodValue;
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(JackServerGris);
};
