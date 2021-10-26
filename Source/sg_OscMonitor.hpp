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

#include "sg_LayoutComponent.hpp"

class MainContentComponent;
class GrisLookAndFeel;

//==============================================================================
class OscMonitorComponent final : public juce::Component
{
    juce::TextEditor mTextEditor{};

    juce::TextButton mRecordButton{ "Monitor" };

public:
    //==============================================================================
    OscMonitorComponent();
    ~OscMonitorComponent() override = default;
    //==============================================================================
    OscMonitorComponent(OscMonitorComponent const &) = delete;
    OscMonitorComponent(OscMonitorComponent &&) = delete;
    OscMonitorComponent & operator=(OscMonitorComponent const &) = delete;
    OscMonitorComponent & operator=(OscMonitorComponent &&) = delete;
    //==============================================================================
    void addMessage(juce::OSCMessage const & message);
    //==============================================================================
    void resized() override;

private:
    //==============================================================================
    JUCE_LEAK_DETECTOR(OscMonitorComponent)
};

//==============================================================================
class OscMonitorWindow final : public juce::DocumentWindow
{
    MainContentComponent & mMainContentComponent;
    OscMonitorComponent mComponent{};

public:
    //==============================================================================
    OscMonitorWindow(MainContentComponent & mainContentComponent, GrisLookAndFeel & lookAndFeel);
    ~OscMonitorWindow() override = default;
    //==============================================================================
    OscMonitorWindow(OscMonitorWindow const &) = delete;
    OscMonitorWindow(OscMonitorWindow &&) = delete;
    OscMonitorWindow & operator=(OscMonitorWindow const &) = delete;
    OscMonitorWindow & operator=(OscMonitorWindow &&) = delete;
    //==============================================================================
    void addMessage(juce::OSCMessage const & message);
    //==============================================================================
    void closeButtonPressed() override;

private:
    //==============================================================================
    JUCE_LEAK_DETECTOR(OscMonitorWindow)
};