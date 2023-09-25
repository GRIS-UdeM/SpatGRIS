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

#include "sg_OscMonitor.hpp"

#include "sg_GrisLookAndFeel.hpp"
#include "sg_MainComponent.hpp"

namespace gris
{
namespace
{
constexpr auto DEFAULT_WIDTH = 800;
constexpr auto DEFAULT_HEIGHT = 500;
constexpr auto MAX_TEXT_LENGTH = 10000;

} // namespace

//==============================================================================
OscMonitorComponent::OscMonitorComponent(LogBuffer & logBuffer) : mLogBuffer(logBuffer)
{
    mTextEditor.setCaretVisible(false);
    mTextEditor.setReadOnly(true);
    mTextEditor.setBorder(juce::BorderSize<int>{ 3 });
    mTextEditor.setMultiLine(true, false);
    mTextEditor.setScrollbarsShown(true);
    addAndMakeVisible(mTextEditor);

    mStartStopButton.setButtonText("Stop");
    mStartStopButton.setClickingTogglesState(true);
    mStartStopButton.setToggleState(true, juce::NotificationType::dontSendNotification);
    mStartStopButton.addListener(this);
    addAndMakeVisible(mStartStopButton);

    logBuffer.addListener(this);
    logBuffer.start();
}

//==============================================================================
OscMonitorComponent::~OscMonitorComponent()
{
    mLogBuffer.removeListener(this);
    mLogBuffer.stop();
}

//==============================================================================
void OscMonitorComponent::buttonClicked([[maybe_unused]] juce::Button * button)
{
    jassert(button == &mStartStopButton);
    if (mStartStopButton.getToggleState()) {
        mLogBuffer.start();
        mStartStopButton.setButtonText("Stop");
        return;
    }
    mLogBuffer.stop();
    mStartStopButton.setButtonText("Start");
}

//==============================================================================
void OscMonitorComponent::oscEventReceived(juce::String const & event)
{
    auto const text{ mTextEditor.getText() + event + "\n" };
    auto const shortenText{ text.substring(std::max(text.length() - MAX_TEXT_LENGTH, 0)) };

    mTextEditor.setText(shortenText);
    mTextEditor.setCaretPosition(shortenText.length());
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
    mStartStopButton.setBounds(recordButtonBounds);
}

//==============================================================================
OscMonitorWindow::OscMonitorWindow(LogBuffer & logBuffer,
                                   MainContentComponent & mainContentComponent,
                                   GrisLookAndFeel & lookAndFeel)
    : DocumentWindow("OSC monitor", lookAndFeel.getBackgroundColour(), allButtons)
    , mMainContentComponent(mainContentComponent)
    , mComponent(logBuffer)
{
    setUsingNativeTitleBar(true);
    setContentNonOwned(&mComponent, false);
    centreAroundComponent(&mainContentComponent, DEFAULT_WIDTH, DEFAULT_HEIGHT);
    DocumentWindow::setVisible(true);
}

//==============================================================================
void OscMonitorWindow::closeButtonPressed()
{
    mMainContentComponent.closeOscMonitorWindow();
}

} // namespace gris
