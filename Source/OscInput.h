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

class MainContentComponent;

static const std::string OSC_PAN_AZ = "/pan/az";
static const std::string OSC_SPAT_SERV = "/spat/serv";

//==============================================================================
class OscInput final
    : private juce::OSCReceiver
    , private juce::OSCReceiver::Listener<juce::OSCReceiver::RealtimeCallback>
{
    MainContentComponent & mMainContentComponent;

public:
    //==============================================================================
    explicit OscInput(MainContentComponent & parent) : mMainContentComponent(parent) {}
    //==============================================================================
    OscInput() = delete;
    ~OscInput() override;

    OscInput(OscInput const &) = delete;
    OscInput(OscInput &&) = delete;

    OscInput & operator=(OscInput const &) = delete;
    OscInput & operator=(OscInput &&) = delete;
    //==============================================================================
    bool startConnection(int port);
    bool closeConnection() { return this->disconnect(); }

private:
    //==============================================================================
    void oscMessageReceived(juce::OSCMessage const & message) override;
    void oscBundleReceived(juce::OSCBundle const & bundle) override;
    //==============================================================================
    JUCE_LEAK_DETECTOR(OscInput)
};
