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

#include "sg_OscInput.hpp"

#include "sg_MainComponent.hpp"

namespace gris
{
static juce::String const SPAT_GRIS_OSC_ADDRESS = "/spat/serv";

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
    source_index_t const sourceIndex{ message[1].getInt32() };
    if (!LEGAL_SOURCE_INDEX_RANGE.contains(sourceIndex)) {
        jassertfalse;
        return;
    }

    auto const azimuthSpan{ message[5].getFloat32() };
    auto const zenithSpan{ message[6].getFloat32() };

    auto const coordinateType{ message[0].getString() };
    if (coordinateType == "pol") {
        processPolarRadianSourcePositionMessage(message, sourceIndex, azimuthSpan, zenithSpan);
        return;
    }
    if (coordinateType == "deg") {
        processPolarDegreeSourcePosition(message, sourceIndex, azimuthSpan, zenithSpan);
        return;
    }
    if (coordinateType == "car") {
        processCartesianSourcePositionMessage(message, sourceIndex, azimuthSpan, zenithSpan);
        return;
    }
    jassertfalse;
}

//==============================================================================
void OscInput::processPolarRadianSourcePositionMessage(juce::OSCMessage const & message,
                                                       source_index_t const sourceIndex,
                                                       float const azimuthSpan,
                                                       float const zenithSpan) const noexcept
{
    auto const azimuth{ HALF_PI - radians_t{ message[2].getFloat32() } };
    radians_t const zenith{ message[3].getFloat32() };
    float const radius{ message[4].getFloat32() };

    Position const position{ PolarVector{ azimuth.balanced(), zenith.balanced(), radius } };
    mMainContentComponent.setSourcePosition(sourceIndex, position, azimuthSpan, zenithSpan);
}

//==============================================================================
void OscInput::processPolarDegreeSourcePosition(juce::OSCMessage const & message,
                                                source_index_t const sourceIndex,
                                                float const azimuthSpan,
                                                float const zenithSpan) const noexcept
{
    auto const azimuth{ HALF_PI - radians_t{ degrees_t{ message[2].getFloat32() } } };
    radians_t const zenith{ degrees_t{ message[3].getFloat32() } };
    float const radius{ message[4].getFloat32() };

    Position const position{ PolarVector{ azimuth.balanced(), zenith.balanced(), radius } };
    mMainContentComponent.setSourcePosition(sourceIndex, position, azimuthSpan, zenithSpan);
}

//==============================================================================
void OscInput::processCartesianSourcePositionMessage(juce::OSCMessage const & message,
                                                     source_index_t const sourceIndex,
                                                     float const horizontalSpan,
                                                     float const verticalSpan) const noexcept
{
    auto const x{ message[2].getFloat32() };
    auto const y{ message[3].getFloat32() };
    auto const z{ message[4].getFloat32() };

    Position const position{ CartesianVector{ x, y, z } };
    mMainContentComponent.setSourcePosition(sourceIndex, position, horizontalSpan, verticalSpan);
}

//==============================================================================
void OscInput::processLegacySourcePositionMessage(juce::OSCMessage const & message) const noexcept
{
    // int id, float azi [0, 2pi], float ele [0, pi], float azispan [0, 2],
    // float elespan [0, 0.5], float distance [0, 1], float gain [0, 1].
    source_index_t const sourceIndex{ message[0].getInt32() + 1 };
    if (!LEGAL_SOURCE_INDEX_RANGE.contains(sourceIndex)) {
        jassertfalse;
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

    mMainContentComponent.setLegacySourcePosition(sourceIndex, azimuth, zenith, length, azimuthSpan, zenithSpan);
}

//==============================================================================
void OscInput::processSourceResetPositionMessage(juce::OSCMessage const & message) const noexcept
{
    jassert(message[0].getString() == "clr");

    source_index_t const sourceIndex{ message[1].getInt32() };
    if (!LEGAL_SOURCE_INDEX_RANGE.contains(sourceIndex)) {
        jassertfalse;
        return;
    }

    mMainContentComponent.resetSourcePosition(sourceIndex);
}

//==============================================================================
void OscInput::processLegacySourceResetPositionMessage(juce::OSCMessage const & message) const noexcept
{
    if (message[0].getString() == juce::String{ "reset" }) {
        // string "reset", int voice_to_reset.
        source_index_t const sourceIndex{ message[1].getInt32() + 1 };
        if (!LEGAL_SOURCE_INDEX_RANGE.contains(sourceIndex)) {
            jassertfalse;
            return;
        }

        mMainContentComponent.resetSourcePosition(sourceIndex);
    }
}

//==============================================================================
void OscInput::processSourceHybridModeMessage(juce::OSCMessage const & message) const noexcept
{
    jassert(message[0].isString() && message[1].isInt32() && message[2].isString());

    if (message[0].getString() != "alg") {
        jassertfalse;
        return;
    }

    source_index_t const sourceIndex{ message[1].getInt32() };
    if (!LEGAL_SOURCE_INDEX_RANGE.contains(sourceIndex)) {
        jassertfalse;
        return;
    }

    static auto constexpr filter_spat_mode = [](SpatMode const spatMode) -> tl::optional<SpatMode> {
        switch (spatMode) {
        case SpatMode::hybrid:
            return tl::nullopt;
        case SpatMode::lbap:
        case SpatMode::vbap:
            return spatMode;
        }
        jassertfalse;
        return {};
    };

    auto const spatMode{ stringToSpatMode(message[2].getString()).and_then(filter_spat_mode) };

    if (!spatMode) {
        jassertfalse;
        return;
    }

    // Some side-effects of setSourceHybridSpatMode() expect to be visited only by the message thread.
    // MessageManager::callAsync() is pretty inefficient but it's no big deal since we only call this when we want to
    // change a source's hybrid spat mode, which shouldn't be too often.
    juce::MessageManager::callAsync([this, sourceIndex, spatMode = *spatMode] {
        this->mMainContentComponent.setSourceHybridSpatMode(sourceIndex, spatMode);
    });
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
    static auto constexpr OSC_ARGUMENT_IS_INT = [](juce::OSCArgument const & argument) { return argument.isInt32(); };
    static auto constexpr OSC_ARGUMENT_IS_STRING
        = [](juce::OSCArgument const & argument) { return argument.isString(); };
    static auto constexpr OSC_ARGUMENT_IS_FLOAT
        = [](juce::OSCArgument const & argument) { return argument.isFloat32(); };

    switch (message.size()) {
    case 2:
        if (!OSC_ARGUMENT_IS_STRING(message[0]) || !OSC_ARGUMENT_IS_INT(message[1])) {
            break;
        }
        if (message[0].getString() == "reset") {
            return MessageType::legacyResetSourcePosition;
        }
        if (message[0].getString() == "clr") {
            return MessageType::resetSourcePosition;
        }
        break;
    case 3:
        if (OSC_ARGUMENT_IS_STRING(message[0]) && OSC_ARGUMENT_IS_INT(message[1])
            && OSC_ARGUMENT_IS_STRING(message[2])) {
            return MessageType::sourceHybridMode;
        }
        break;
    case 7:
        if (OSC_ARGUMENT_IS_STRING(message[0]) && OSC_ARGUMENT_IS_INT(message[1])
            && std::all_of(message.begin() + 2, message.end(), OSC_ARGUMENT_IS_FLOAT)) {
            return MessageType::sourcePosition;
        }
        if (OSC_ARGUMENT_IS_INT(message[0]) && std::all_of(message.begin() + 1, message.end(), OSC_ARGUMENT_IS_FLOAT)) {
            return MessageType::legacySourcePosition;
        }
        break;
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
        case MessageType::legacySourcePosition:
            processLegacySourcePositionMessage(message);
            break;
        case MessageType::legacyResetSourcePosition:
            processLegacySourceResetPositionMessage(message);
            break;
        case MessageType::sourcePosition:
            processSourcePositionMessage(message);
            break;
        case MessageType::resetSourcePosition:
            processSourceResetPositionMessage(message);
            break;
        case MessageType::sourceHybridMode:
            processSourceHybridModeMessage(message);
            break;
        case MessageType::invalid:
            jassertfalse;
            break;
        default:;
        }
    }

    if (mMainContentComponent.getOscMonitor()) {
        juce::MessageManagerLock const mml{};
        const auto & oscMonitor{ mMainContentComponent.getOscMonitor() };
        if (oscMonitor) {
            oscMonitor->addMessage(message);
        }
    }
}

} // namespace gris