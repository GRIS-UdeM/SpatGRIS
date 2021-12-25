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

#include "sg_OscLogWindow.hpp"

#include "sg_GrisLookAndFeel.hpp"
#include "sg_MainComponent.hpp"

namespace gris
{
//==============================================================================
OscLogWindow::OscLogWindow(juce::String const & name,
                           juce::Colour const backgroundColour,
                           int const buttonsNeeded,
                           MainContentComponent * parent,
                           GrisLookAndFeel * feel)
    : DocumentWindow(name, backgroundColour, buttonsNeeded)
    , mMainContentComponent(parent)
    , mLookAndFeel(feel)
    , mLogger(mCodeDocument, nullptr)
{
    this->mIndex = 0;
    this->mActivated = true;

    this->mLogger.setFont(this->mLogger.getFont().withPointHeight(this->mLogger.getFont().getHeightInPoints() + 3));
    this->mLogger.setBounds(5, 5, 490, 450);
    this->mLogger.setLookAndFeel(this->mLookAndFeel);
    this->juce::Component::addAndMakeVisible(this->mLogger);

    this->mStopButton.setButtonText("Stop");
    this->mStopButton.setClickingTogglesState(true);
    this->mStopButton.setToggleState(true, juce::NotificationType::dontSendNotification);
    this->mStopButton.setBounds(100, 470, 100, 22);
    this->mStopButton.addListener(this);
    this->mStopButton.setColour(juce::ToggleButton::textColourId, this->mLookAndFeel->getFontColour());
    this->mStopButton.setLookAndFeel(this->mLookAndFeel);
    this->juce::Component::addAndMakeVisible(this->mStopButton);

    this->mCloseButton.setButtonText("Close");
    this->mCloseButton.setBounds(300, 470, 100, 22);
    this->mCloseButton.addListener(this);
    this->mCloseButton.setColour(juce::ToggleButton::textColourId, this->mLookAndFeel->getFontColour());
    this->mCloseButton.setLookAndFeel(this->mLookAndFeel);
    this->juce::Component::addAndMakeVisible(this->mCloseButton);
}

//==============================================================================
OscLogWindow::~OscLogWindow()
{
    this->mMainContentComponent->closeOscMonitorWindow();
}

//==============================================================================
void OscLogWindow::addToLog(juce::String msg)
{
    if (this->mActivated) {
        this->mIndex++;

        const juce::MessageManagerLock mmLock;

        this->mLogger.insertTextAtCaret(msg);

        if (this->mIndex == 500) {
            this->mIndex = 0;
            this->mLogger.loadContent(juce::String(""));
        }
    }
}

//==============================================================================
void OscLogWindow::closeButtonPressed()
{
    this->mStopButton.setButtonText("Start");
    this->mActivated = false;
    this->mMainContentComponent->closeOscMonitorWindow();
    delete this;
}

//==============================================================================
void OscLogWindow::buttonClicked(juce::Button * button)
{
    if (button == &this->mStopButton) {
        if (button->getToggleState()) {
            this->mStopButton.setButtonText("Stop");
            this->mActivated = true;
        } else {
            this->mStopButton.setButtonText("Start");
            this->mActivated = false;
        }
    } else {
        this->closeButtonPressed();
    }
}

} // namespace gris