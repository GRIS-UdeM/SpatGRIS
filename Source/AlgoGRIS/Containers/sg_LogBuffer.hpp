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

#pragma once

#include "../Data/sg_Macros.hpp"

#include <JuceHeader.h>

namespace gris
{
//==============================================================================
class LogBuffer final : private juce::AsyncUpdater
{
public:
    //==============================================================================
    class Listener
    {
    public:
        Listener() = default;
        virtual ~Listener() = default;
        SG_DEFAULT_COPY_AND_MOVE(Listener)
        //==============================================================================
        virtual void oscEventReceived(juce::String const & event) = 0;
    };

private:
    //==============================================================================
    juce::StringArray mBuffer{};
    juce::CriticalSection mMutex{};
    std::atomic<bool> mActive{};
    juce::ListenerList<Listener> mListeners{};

public:
    //==============================================================================
    LogBuffer() = default;
    ~LogBuffer() override = default;
    SG_DELETE_COPY_AND_MOVE(LogBuffer)
    //==============================================================================
    void addListener(Listener * l);
    void removeListener(Listener * l);
    void add(juce::String const & event);
    void start();
    void stop();
    [[nodiscard]] bool isActive() const noexcept;

private:
    //==============================================================================
    juce::StringArray stealData();
    void handleAsyncUpdate() override;
    //==============================================================================
    JUCE_LEAK_DETECTOR(LogBuffer)
};

} // namespace gris
