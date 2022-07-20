/*
 This file is part of SpatGRIS.

 Developers: Gaël Lane Lépine, Samuel Béland, Olivier Bélanger, Nicolas Masson

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
namespace
{
// see juce::OSCTypes
constexpr auto INT_TAG = 'i';
constexpr auto FLOAT_TAG = 'f';
constexpr auto STRING_TAG = 's';

auto constexpr IS_INT = [](juce::OSCArgument const & arg) -> bool { return arg.getType() == INT_TAG; };
auto constexpr IS_FLOAT = [](juce::OSCArgument const & arg) -> bool { return arg.getType() == FLOAT_TAG; };
auto constexpr IS_STRING = [](juce::OSCArgument const & arg) -> bool { return arg.getType() == STRING_TAG; };

juce::String const SPAT_GRIS_OSC_ADDRESS = "/spat/serv";

//==============================================================================
juce::String argumentToString(juce::OSCArgument const & argument) noexcept
{
    if (IS_FLOAT(argument)) {
        return juce::String{ argument.getFloat32() };
    }
    if (IS_INT(argument)) {
        return juce::String{ argument.getInt32() };
    }
    if (IS_STRING(argument)) {
        return argument.getString();
    }
    return "<INVALID TYPE>";
}

//==============================================================================
juce::String messageToString(juce::OSCMessage const & message)
{
    static constexpr auto SEPARATOR{ ", " };

    auto result{ juce::String{ "[" } + message.getAddressPattern().toString() + "] " };

    for (auto const & argument : message) {
        result += SEPARATOR + argumentToString(argument);
    }

    return result;
}

} // namespace

//==============================================================================
OscInput::~OscInput()
{
    disconnect();
}

//==============================================================================
bool OscInput::startConnection(int const port)
{
    auto const success = connect(port);
    addListener(this);
    return success;
}

//==============================================================================
void OscInput::processSourcePositionMessage(juce::OSCMessage const & message) const noexcept
{
    auto const sourceIndex{ extractSourceIndex(message[1], SourceIndexBase::fromOne) };
    if (!sourceIndex) {
        return;
    }

    auto const azimuthSpan{ message[5].getFloat32() };
    auto const zenithSpan{ message[6].getFloat32() };

    auto const coordinateType{ message[0].getString() };
    if (coordinateType == "pol") {
        processPolarRadianSourcePositionMessage(message, *sourceIndex, azimuthSpan, zenithSpan);
        return;
    }
    if (coordinateType == "deg") {
        processPolarDegreeSourcePosition(message, *sourceIndex, azimuthSpan, zenithSpan);
        return;
    }
    if (coordinateType == "car") {
        processCartesianSourcePositionMessage(message, *sourceIndex, azimuthSpan, zenithSpan);
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
    auto const sourceIndex{ extractSourceIndex(message[0], SourceIndexBase::fromZero) };
    if (!sourceIndex) {
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

    mMainContentComponent.setLegacySourcePosition(*sourceIndex, azimuth, zenith, length, azimuthSpan, zenithSpan);
}

//==============================================================================
void OscInput::processSourceResetPositionMessage(juce::OSCMessage const & message) const noexcept
{
    auto const sourceIndex{ extractSourceIndex(message[1], SourceIndexBase::fromOne) };
    if (sourceIndex) {
        mMainContentComponent.resetSourcePosition(*sourceIndex);
    }
}

//==============================================================================
void OscInput::processLegacySourceResetPositionMessage(juce::OSCMessage const & message) const noexcept
{
    // string "reset", int voice_to_reset.
    auto const sourceIndex{ extractSourceIndex(message[0], SourceIndexBase::fromZero) };
    if (sourceIndex) {
        mMainContentComponent.resetSourcePosition(*sourceIndex);
    }
}

//==============================================================================
void OscInput::processSourceHybridModeMessage(juce::OSCMessage const & message) const noexcept
{
    auto const sourceIndex{ extractSourceIndex(message[1], SourceIndexBase::fromOne) };

    if (!sourceIndex) {
        return;
    }

    static auto constexpr filter_spat_mode = [](SpatMode const spatMode) -> tl::optional<SpatMode> {
        switch (spatMode) {
        case SpatMode::hybrid:
        case SpatMode::invalid:
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
        addErrorToBuffer("unrecognized hybrid spat mode.");
        return;
    }

    // Some side-effects of setSourceHybridSpatMode() expect to be visited only by the message thread.
    // MessageManager::callAsync() is pretty inefficient but it's no big deal since we only call this when we want to
    // change a source's hybrid spat mode, which shouldn't be too often.
    juce::MessageManager::callAsync([this, sourceIndex = *sourceIndex, spatMode = *spatMode] {
        this->mMainContentComponent.setSourceHybridSpatMode(sourceIndex, spatMode);
    });
}

//==============================================================================
void OscInput::addToBuffer(juce::String const & string) const
{
    if (mLogBuffer.isActive()) {
        mLogBuffer.add(string);
    }
}

//==============================================================================
void OscInput::addErrorToBuffer(juce::String const & string) const
{
    jassertfalse;
    if (mLogBuffer.isActive()) {
        mLogBuffer.add(juce::String{ "ERROR : " } + string);
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
OscInput::MessageType OscInput::getMessageType(juce::OSCMessage const & message) const noexcept
{
    if (message.getAddressPattern().toString() != SPAT_GRIS_OSC_ADDRESS) {
        addErrorToBuffer("wrong OSC address.");
        return MessageType::invalid;
    }

    if (message.size() < 2) {
        addErrorToBuffer("messages need at least 2 arguments.");
        return MessageType::invalid;
    }

    if (!IS_STRING(message[0])) {
        if (message.size() < 6) {
            addErrorToBuffer("expected legacy source position message to have at least 6 arguments.");
            return MessageType::invalid;
        }
        if (!std::all_of(message.begin() + 1, message.end(), IS_FLOAT)) {
            addErrorToBuffer("expected arguments 2 to 6 of legacy source position message to be floats.");
            return MessageType::invalid;
        }
        return MessageType::legacySourcePosition;
    }

    auto const firstArg{ message[0].getString() };
    if (firstArg == "pol" || firstArg == "deg" || firstArg == "car") {
        if (message.size() != 7) {
            addErrorToBuffer("expected source position message to be exactly 7 arguments long.");
            return MessageType::invalid;
        }
        if (!std::all_of(message.begin() + 2, message.end(), IS_FLOAT)) {
            addErrorToBuffer("expected arguments 2 to 7 of source position message to be floats.");
            return MessageType::invalid;
        }
        return MessageType::sourcePosition;
    }

    if (firstArg == "clr") {
        if (message.size() != 2) {
            addErrorToBuffer("expected clear message to be exactly 2 arguments long.");
            return MessageType::invalid;
        }
        return MessageType::resetSourcePosition;
    }

    if (firstArg == "alg") {
        if (message.size() != 3) {
            addErrorToBuffer("expected source hybrid mode message to be exactly 3 arguments long.");
            return MessageType::invalid;
        }
        if (!IS_STRING(message[2])) {
            addErrorToBuffer("expected the 3rd argument of a source hybrid mode message to be a string.");
            return MessageType::invalid;
        }
        return MessageType::sourceHybridMode;
    }

    if (firstArg == "reset") {
        if (message.size() != 2) {
            addErrorToBuffer("expected a legacy source reset position message to be exactly 2 arguments long.");
            return MessageType::invalid;
        }
        return MessageType::legacyResetSourcePosition;
    }

    addErrorToBuffer(juce::String{ "unknown command \"" } + firstArg + "\".");
    return MessageType::invalid;
}

//==============================================================================
tl::optional<source_index_t> OscInput::extractSourceIndex(juce::OSCArgument const & arg,
                                                          SourceIndexBase const base) const noexcept
{
    auto const offset{ base == SourceIndexBase::fromZero ? 1 : 0 };
    source_index_t result;
    if (IS_INT(arg)) {
        result = source_index_t{ arg.getInt32() + offset };
    } else if (IS_FLOAT(arg)) {
        result = source_index_t{ narrow<source_index_t::type>(std::round(arg.getFloat32())) + offset };
    } else {
        addErrorToBuffer("source index should be either an int or a float.");
        return tl::nullopt;
    }
    if (!LEGAL_SOURCE_INDEX_RANGE.contains(result)) {
        addErrorToBuffer("source index out of range.");
        return tl::nullopt;
    }

    return result;
}

//==============================================================================
void OscInput::oscMessageReceived(juce::OSCMessage const & message)
{
    addToBuffer(messageToString(message));

    switch (getMessageType(message)) {
    case MessageType::legacySourcePosition:
        processLegacySourcePositionMessage(message);
        return;
    case MessageType::legacyResetSourcePosition:
        processLegacySourceResetPositionMessage(message);
        return;
    case MessageType::sourcePosition:
        processSourcePositionMessage(message);
        return;
    case MessageType::resetSourcePosition:
        processSourceResetPositionMessage(message);
        return;
    case MessageType::sourceHybridMode:
        processSourceHybridModeMessage(message);
        return;
    case MessageType::invalid:
        break;
    }
    jassertfalse;
}

} // namespace gris
