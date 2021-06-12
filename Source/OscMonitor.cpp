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

#include "OscMonitor.hpp"

#include "GrisLookAndFeel.hpp"
#include "MainComponent.hpp"

static constexpr auto DEFAULT_WIDTH = 500;
static constexpr auto DEFAULT_HEIGHT = 500;

juce::String messageToString(juce::OSCMessage const & message)
{
    static juce::String const INVALID = "<INVALID FORMAT>\n";
    static constexpr auto SEPARATOR{ '\t' };

    auto const isValidPosition{ message.size() == 7 && message.begin()->isInt32()
                                && std::all_of(message.begin() + 1, message.end(), [](juce::OSCArgument const & arg) {
                                       return arg.isFloat32();
                                   }) };

    auto const isValidResetCommand{ message.size() == 2 && message[0].isString() && message[1].isInt32() };

    if (!isValidPosition && !isValidResetCommand) {
        return INVALID;
    }

    if (isValidResetCommand) {
        return message[0].getString() + SEPARATOR + juce::String{ message[1].getInt32() } + '\n';
    }

    auto const extract = [&](int const index) { return juce::String{ message[index].getFloat32() }; };

    return juce::String{ message[0].getInt32() } + extract(1) + extract(2) + extract(3) + extract(4) + extract(5)
           + extract(6) + '\n';
}

//==============================================================================
OscMonitorComponent::OscMonitorComponent()
{
    mTextEditor.setCaretVisible(false);
    mTextEditor.setBorder(juce::BorderSize<int>{ 3 });
    mTextEditor.setMultiLine(true, false);
    mTextEditor.setScrollbarsShown(true);
    mTextEditor.setWantsKeyboardFocus(false);
    addAndMakeVisible(mTextEditor);
}

//==============================================================================
void OscMonitorComponent::addMessage(juce::OSCMessage const & message)
{
    juce::MessageManagerLock const mml{};

    mTextEditor.setCaretPosition(mTextEditor.getText().length());
    mTextEditor.insertTextAtCaret(messageToString(message));
}

//==============================================================================
void OscMonitorComponent::resized()
{
    mTextEditor.setBounds(0, 0, getWidth(), getHeight());
}

//==============================================================================
OscMonitorWindow::OscMonitorWindow(MainContentComponent & mainContentComponent, GrisLookAndFeel & lookAndFeel)
    : DocumentWindow("OSC monitor", lookAndFeel.getBackgroundColour(), juce::DocumentWindow::allButtons)
    , mMainContentComponent(mainContentComponent)
{
    setContentNonOwned(&mComponent, false);
    centreAroundComponent(&mainContentComponent, DEFAULT_WIDTH, DEFAULT_HEIGHT);
    DocumentWindow::setVisible(true);
}

//==============================================================================
void OscMonitorWindow::addMessage(juce::OSCMessage const & message)
{
    mComponent.addMessage(message);
}

//==============================================================================
void OscMonitorWindow::closeButtonPressed()
{
    mMainContentComponent.closeOscMonitorWindow();
}
