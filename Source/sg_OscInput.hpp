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

#pragma once

#include "Containers/sg_LogBuffer.hpp"
#include "Data/StrongTypes/sg_SourceIndex.hpp"
#include "tl/optional.hpp"

namespace gris
{
class MainContentComponent;

//==============================================================================
class OscInput final
    : private juce::OSCReceiver
    , private juce::OSCReceiver::Listener<juce::OSCReceiver::RealtimeCallback>
{
    enum class MessageType {
        invalid,
        sourcePosition,
        resetSourcePosition,
        sourceHybridMode,
        legacySourcePosition,
        legacyResetSourcePosition,
        sourceColour
    };

    MainContentComponent & mMainContentComponent;
    LogBuffer & mLogBuffer;

public:
    //==============================================================================
    OscInput(MainContentComponent & parent, LogBuffer & logBuffer)
        : mMainContentComponent(parent)
        , mLogBuffer(logBuffer)
    {
    }
    OscInput() = delete;
    ~OscInput() override;
    SG_DELETE_COPY_AND_MOVE(OscInput)
    //==============================================================================
    bool startConnection(int port);
    bool closeConnection() { return this->disconnect(); }

private:
    //==============================================================================
    void processSourcePositionMessage(juce::OSCMessage const & message) const noexcept;
    void processPolarRadianSourcePositionMessage(juce::OSCMessage const & message,
                                                 source_index_t sourceIndex,
                                                 float azimuthSpan,
                                                 float zenithSpan) const noexcept;
    void processPolarDegreeSourcePosition(juce::OSCMessage const & message,
                                          source_index_t sourceIndex,
                                          float azimuthSpan,
                                          float zenithSpan) const noexcept;
    void processCartesianSourcePositionMessage(juce::OSCMessage const & message,
                                               source_index_t sourceIndex,
                                               float horizontalSpan,
                                               float verticalSpan) const noexcept;
    void processLegacySourcePositionMessage(juce::OSCMessage const & message) const noexcept;
    void processSourceResetPositionMessage(juce::OSCMessage const & message) const noexcept;
    void processLegacySourceResetPositionMessage(juce::OSCMessage const & message) const noexcept;
    void processSourceHybridModeMessage(juce::OSCMessage const & message) const noexcept;
    void processSourceColourMessage(juce::OSCMessage const & message) const noexcept;
    MessageType getMessageType(juce::OSCMessage const & message) const noexcept;

    enum class SourceIndexBase { fromZero, fromOne };

    tl::optional<source_index_t> extractSourceIndex(juce::OSCArgument const & arg,
                                                    SourceIndexBase const base) const noexcept;
    //==============================================================================
    void addToBuffer(juce::String const & string) const;
    void addErrorToBuffer(juce::String const & string) const;
    //==============================================================================
    void oscMessageReceived(juce::OSCMessage const & message) override;
    void oscBundleReceived(juce::OSCBundle const & bundle) override;
    //==============================================================================
    JUCE_LEAK_DETECTOR(OscInput)
};

} // namespace gris
