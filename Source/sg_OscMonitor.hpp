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

#include "sg_LogBuffer.hpp"

namespace gris
{
class MainContentComponent;
class GrisLookAndFeel;

//==============================================================================
class OscMonitorComponent final
    : public juce::Component
    , public LogBuffer::Listener
    , private juce::TextButton::Listener
{
    LogBuffer & mLogBuffer;

    juce::TextEditor mTextEditor{};
    juce::TextButton mStartStopButton{};

public:
    //==============================================================================
    explicit OscMonitorComponent(LogBuffer & logBuffer);
    OscMonitorComponent() = delete;
    ~OscMonitorComponent() override;
    SG_DELETE_COPY_AND_MOVE(OscMonitorComponent)
    //==============================================================================
    void buttonClicked(juce::Button * button) override;
    void oscEventReceived(juce::String const & event) override;
    void resized() override;

private:
    //==============================================================================
    JUCE_LEAK_DETECTOR(OscMonitorComponent)
};

//==============================================================================
class OscMonitorWindow final : public juce::DocumentWindow
{
    MainContentComponent & mMainContentComponent;
    OscMonitorComponent mComponent;

public:
    //==============================================================================
    OscMonitorWindow(LogBuffer & logBuffer, MainContentComponent & mainContentComponent, GrisLookAndFeel & lookAndFeel);
    ~OscMonitorWindow() override = default;
    SG_DELETE_COPY_AND_MOVE(OscMonitorWindow)
    //==============================================================================
    void closeButtonPressed() override;

private:
    //==============================================================================
    JUCE_LEAK_DETECTOR(OscMonitorWindow)
};

} // namespace gris