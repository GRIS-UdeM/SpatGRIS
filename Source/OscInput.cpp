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

#include "OscInput.hpp"

#include "MainComponent.hpp"

static juce::String const SPAT_GRIS_OSC_ADDRESS = "/spat/serv";
static juce::String const LEGACY_SPAT_GRIS_OSC_ADDRESS = "/pan/az";

//==============================================================================
OscInput::~OscInput()
{
    disconnect();
}

//==============================================================================
bool OscInput::startConnection(int const port)
{
    auto const b = connect(port);
    addListener(this);
    return b;
}

//==============================================================================
void OscInput::processSourcePositionMessage(juce::OSCMessage const & message) const noexcept
{
    // int id, float azi [0, 2pi], float ele [0, pi], float azispan [0, 2],
    // float elespan [0, 0.5], float distance [0, 1], float gain [0, 1].
    source_index_t const sourceIndex{ message[0].getInt32() + 1 };
    jassert(LEGAL_SOURCE_INDEX_RANGE.contains(sourceIndex));
    if (!LEGAL_SOURCE_INDEX_RANGE.contains(sourceIndex)) {
        return;
    }
    auto const azimuth{ HALF_PI - radians_t{ message[1].getFloat32() }.balanced() };
    auto const zenith{ HALF_PI - radians_t{ message[2].getFloat32() } };
    auto const azimuthSpan{ message[3].getFloat32() / 2.0f };
    jassert(azimuthSpan >= 0.0f && azimuthSpan <= 1.0f);
    auto const zenithSpan{ message[4].getFloat32() * 2.0f };
    jassert(zenithSpan >= 0.0f && zenithSpan <= 1.0f);
    auto const length{ message[5].getFloat32() };

    [[maybe_unused]] auto const gain{ message[6].getFloat32() };

    mMainContentComponent.handleSourcePositionChanged(sourceIndex, azimuth, zenith, length, azimuthSpan, zenithSpan);
}

//==============================================================================
void OscInput::processSourceResetPositionMessage(juce::OSCMessage const & message) noexcept
{
    if (message[0].getString().compare("reset") == 0) {
        // string "reset", int voice_to_reset.
        source_index_t const sourceIndex{ message[1].getInt32() };
        mMainContentComponent.resetSourcePosition(sourceIndex);
    }
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
OscInput::MessageType OscInput::getMessageType(juce::OSCMessage const & message) noexcept
{
    switch (message.size()) {
    case 2:
        if (message[0].isString() && message[1].isInt32()) {
            return MessageType::resetPosition;
        }
        return MessageType::invalid;
    case 7:
        if (message[0].isInt32()
            && std::all_of(message.begin() + 1, message.end(), [](juce::OSCArgument const & argument) {
                   return argument.isFloat32();
               })) {
            return MessageType::sourcePosition;
        }
        return MessageType::invalid;
    default:
        break;
    }
    jassertfalse;
    return MessageType::invalid;
}

//==============================================================================
void OscInput::oscMessageReceived(const juce::OSCMessage & message)
{
    auto const address{ message.getAddressPattern().toString() };
    if (address == SPAT_GRIS_OSC_ADDRESS) {
        switch (getMessageType(message)) {
        case MessageType::sourcePosition:
            processSourcePositionMessage(message);
            break;
        case MessageType::resetPosition:
            processSourceResetPositionMessage(message);
            break;
        case MessageType::invalid:
            jassertfalse;
            break;
        }
    }

    auto & oscMonitor{ mMainContentComponent.getOscMonitor() };
    if (oscMonitor) {
        oscMonitor->addMessage(message);
    }
}
