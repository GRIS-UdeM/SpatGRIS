/*
 This file is part of SpatGRIS2.
 
 Developers: Samuel Béland, Nicolas Masson
 
 SpatGRIS2 is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.
 
 SpatGRIS2 is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with SpatGRIS2.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "OscLogWindow.h"

#include "GrisLookAndFeel.h"
#include "MainComponent.h"

//==============================================================================
OscLogWindow::OscLogWindow( juce::String const& name,
                            juce::Colour const backgroundColour,
                            int const buttonsNeeded,
                            MainContentComponent* parent,
                            GrisLookAndFeel* feel )
    : DocumentWindow(name, backgroundColour, buttonsNeeded)
    , logger(codeDocument, 0)
    , mainParent(parent)
    , grisFeel(feel)
{
    this->index = 0;
    this->activated = true;

    this->logger.setFont(this->logger.getFont().withPointHeight(this->logger.getFont().getHeightInPoints() + 3));
    this->logger.setBounds(5, 5, 490, 450);
    this->logger.setLookAndFeel(this->grisFeel);
    this->juce::Component::addAndMakeVisible(this->logger);

    this->stop.setButtonText("Stop");
    this->stop.setClickingTogglesState(true);
    this->stop.setToggleState(true, NotificationType::dontSendNotification);
    this->stop.setBounds(100, 470, 100, 22);
    this->stop.addListener(this);
    this->stop.setColour(ToggleButton::textColourId, this->grisFeel->getFontColour());
    this->stop.setLookAndFeel(this->grisFeel);
    this->juce::Component::addAndMakeVisible(this->stop);

    this->close.setButtonText("Close");
    this->close.setBounds(300, 470, 100, 22);
    this->close.addListener(this);
    this->close.setColour(ToggleButton::textColourId, this->grisFeel->getFontColour());
    this->close.setLookAndFeel(this->grisFeel);
    this->juce::Component::addAndMakeVisible(this->close);
}

//==============================================================================
OscLogWindow::~OscLogWindow() {
    this->mainParent->closeOscLogWindow();
}

//==============================================================================
void OscLogWindow::addToLog(String msg) {
    if (this->activated) {
        this->index++;

        const MessageManagerLock mmLock;

        this->logger.insertTextAtCaret(msg);

        if (this->index == 500) {
            this->index = 0;
            this->logger.loadContent(String(""));
        }
    }
}

//==============================================================================
void OscLogWindow::closeButtonPressed() {
    this->stop.setButtonText("Start");
    this->activated = false;
    this->mainParent->closeOscLogWindow();
    delete this;
}

//==============================================================================
void OscLogWindow::buttonClicked(Button *button) {
    if (button == &this->stop) {
        if (button->getToggleState()) {
            this->stop.setButtonText("Stop");
            this->activated = true;
        } else {
            this->stop.setButtonText("Start");
            this->activated = false;
        }
    } else {
        this->closeButtonPressed();
    }
}
