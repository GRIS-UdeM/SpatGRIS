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

#pragma once

#include "macros.h"

DISABLE_WARNINGS
#include <JuceHeader.h>
ENABLE_WARNINGS

#include "JackMockup.h"

//==============================================================================
class JackServerGris
{
    unsigned int mRateValue;
    unsigned int mPeriodValue;
    jackctl_server_t * mServer;

public:
    //==============================================================================
    explicit JackServerGris(unsigned int rateV = 48000,
                            unsigned int periodV = 1024,
                            juce::String alsaOutputDevice = juce::String(),
                            int * errorCode = nullptr);
    //==============================================================================
    JackServerGris() = delete;
    ~JackServerGris();

    JackServerGris(JackServerGris const &) = delete;
    JackServerGris(JackServerGris &&) = delete;

    JackServerGris & operator=(JackServerGris const &) = delete;
    JackServerGris & operator=(JackServerGris &&) = delete;
    //==============================================================================
    // Only effective with alsa driver.
    [[nodiscard]] juce::Array<juce::String> getAvailableOutputDevices() const;

private:
    //==============================================================================
    JUCE_LEAK_DETECTOR(JackServerGris)
};
