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

#include "sg_OscMonitor.hpp"

#include "sg_GrisLookAndFeel.hpp"
#include "sg_MainComponent.hpp"

namespace gris
{
namespace
{
constexpr auto DEFAULT_WIDTH = 800;
constexpr auto DEFAULT_HEIGHT = 500;

} // namespace

//==============================================================================
juce::String argumentToString(juce::OSCArgument const & argument) noexcept
{
    if (argument.isFloat32()) {
        return juce::String{ argument.getFloat32() };
    }
    if (argument.isInt32()) {
        return juce::String{ argument.getInt32() };
    }
    if (argument.isString()) {
        return argument.getString();
    }
    return "<INVALID TYPE>";
}

//==============================================================================
juce::String messageToString(juce::OSCMessage const & message)
{
    static constexpr auto SEPARATOR{ ", " };

    auto const currentTime{ juce::String{ "[" } + juce::String{ juce::Time::currentTimeMillis() } + "ms] " };

    auto result{ currentTime + message.getAddressPattern().toString() };

    for (auto const & argument : message) {
        result += SEPARATOR + argumentToString(argument);
    }

    return result;
}

//==============================================================================
OscMonitorComponent::OscMonitorComponent()
{
    mTextEditor.setCaretVisible(false);
    mTextEditor.setBorder(juce::BorderSize<int>{ 3 });
    mTextEditor.setMultiLine(true, false);
    mTextEditor.setScrollbarsShown(true);
    addAndMakeVisible(mTextEditor);

    mRecordButton.setClickingTogglesState(true);
    addAndMakeVisible(mRecordButton);
}

//==============================================================================
void OscMonitorComponent::addMessage(juce::OSCMessage const & message)
{
    if (!mRecordButton.getToggleState()) {
        return;
    }

    juce::MessageManagerLock const mml{};

    mTextEditor.setCaretPosition(mTextEditor.getText().length());
    mTextEditor.insertTextAtCaret(messageToString(message) + '\n');
}

//==============================================================================
void OscMonitorComponent::resized()
{
    static auto constexpr BUTTON_WIDTH = 100;
    static auto constexpr BUTTON_HEIGHT = 30;
    static auto constexpr PADDING = 5;

    juce::Rectangle<int> const textEditorBounds{ PADDING,
                                                 PADDING,
                                                 DEFAULT_WIDTH - PADDING * 2,
                                                 DEFAULT_HEIGHT - BUTTON_HEIGHT - PADDING * 3 };
    juce::Rectangle<int> const recordButtonBounds{ DEFAULT_WIDTH - PADDING - BUTTON_WIDTH,
                                                   textEditorBounds.getBottom() + PADDING,
                                                   BUTTON_WIDTH,
                                                   BUTTON_HEIGHT };

    mTextEditor.setBounds(textEditorBounds);
    mRecordButton.setBounds(recordButtonBounds);
}

//==============================================================================
OscMonitorWindow::OscMonitorWindow(MainContentComponent & mainContentComponent, GrisLookAndFeel & lookAndFeel)
    : DocumentWindow("OSC monitor", lookAndFeel.getBackgroundColour(), allButtons)
    , mMainContentComponent(mainContentComponent)
{
    setUsingNativeTitleBar(true);
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

} // namespace gris