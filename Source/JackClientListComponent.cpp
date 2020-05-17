/*
 This file is part of SpatGRIS2.
 
 Developers: Samuel Béand, Olivier Belanger, Nicolas Masson
 
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

#include "JackClientListComponent.h"

#include "GrisLookAndFeel.h"
#include "MainComponent.h"
#include "UiComponent.h"

//==============================================================================
JackClientListComponent::JackClientListComponent(MainContentComponent *parent, GrisLookAndFeel *feel) 
    : mainParent(parent)
    , grisFeel(feel)
{
    tableListClient.setModel(this);

    tableListClient.setColour(ListBox::outlineColourId, this->grisFeel->getWinBackgroundColour());
    tableListClient.setColour(ListBox::backgroundColourId, this->grisFeel->getWinBackgroundColour());
    tableListClient.setOutlineThickness(1);

    tableListClient.getHeader().addColumn("Client",    1, 120, 70, 120, TableHeaderComponent::notSortable);
    tableListClient.getHeader().addColumn("Start",     2, 60, 35, 70, TableHeaderComponent::notSortable);
    tableListClient.getHeader().addColumn("End",       3, 60, 35, 70, TableHeaderComponent::notSortable);
    tableListClient.getHeader().addColumn("On/Off",    4, 62, 35, 70, TableHeaderComponent::notSortable);

    tableListClient.setMultipleSelectionEnabled (false);

    tableListClient.updateContent();

    this->addAndMakeVisible(tableListClient);
}

//==============================================================================
void JackClientListComponent::buttonClicked(Button *button) {
    this->mainParent->getLockClients().lock();
    bool connectedCli = !this->mainParent->getListClientjack().at(button->getName().getIntValue()).connected;
    this->mainParent->connectionClientJack(this->mainParent->getListClientjack().at(button->getName().getIntValue()).name, connectedCli);
    updateContentCli();
    this->mainParent->getLockClients().unlock();
}

//==============================================================================
void JackClientListComponent::setBounds(int x, int y, int width, int height) {
    this->juce::Component::setBounds(x, y, width, height);
    tableListClient.setSize(width, height);
}

//==============================================================================
void JackClientListComponent::updateContentCli() {
    numRows = (unsigned int)this->mainParent->getListClientjack().size();
    tableListClient.updateContent();
    tableListClient.repaint();
}

//==============================================================================
void JackClientListComponent::setValue(const int rowNumber, const int columnNumber, const int newRating) {
    this->mainParent->getLockClients().lock();
    if (this->mainParent->getListClientjack().size() > (unsigned int)rowNumber) {
        switch (columnNumber) {
            case 2:
                this->mainParent->getListClientjack().at(rowNumber).portStart = newRating;
                this->mainParent->getListClientjack().at(rowNumber).initialized = true;
                break;
            case 3:
                this->mainParent->getListClientjack().at(rowNumber).portEnd = newRating;
                this->mainParent->getListClientjack().at(rowNumber).initialized = true;
                break;
        }
    }
    bool connectedCli = this->mainParent->getListClientjack().at(rowNumber).connected;
    this->mainParent->connectionClientJack(this->mainParent->getListClientjack().at(rowNumber).name, connectedCli);
    this->mainParent->getLockClients().unlock();
}

//==============================================================================
int JackClientListComponent::getValue(const int rowNumber, const int columnNumber) const {
    if ((unsigned int)rowNumber < this->mainParent->getListClientjack().size()) {
        switch (columnNumber) {
            case 2:
                return this->mainParent->getListClientjack().at(rowNumber).portStart;
            case 3:
                return this->mainParent->getListClientjack().at(rowNumber).portEnd;
        }
    }
    return -1;
}

//==============================================================================
String JackClientListComponent::getText(const int columnNumber, const int rowNumber) const {
    String text = "?";
    if ((unsigned int)rowNumber < this->mainParent->getListClientjack().size()) {
        if (columnNumber == 1) {
            text = String(this->mainParent->getListClientjack().at(rowNumber).name);
        }
    }
    return text;
}

//==============================================================================
void JackClientListComponent::paintRowBackground(Graphics& g, int rowNumber, int /*width*/, int /*height*/, bool rowIsSelected) {
    if (rowNumber % 2) {
        g.fillAll(this->grisFeel->getBackgroundColour().withBrightness(0.6));
    } else {
        g.fillAll(this->grisFeel->getBackgroundColour().withBrightness(0.7));
    }
}

//==============================================================================
void JackClientListComponent::paintCell(Graphics& g, int rowNumber, int columnId, int width, int height, bool /*rowIsSelected*/) {
    g.setColour(Colours::black);
    g.setFont(12.0f);
    if (this->mainParent->getLockClients().try_lock()) {
        if ((unsigned int)rowNumber < this->mainParent->getListClientjack().size()) {
            if (columnId == 1) {
                String text = getText(columnId, rowNumber);
                g.drawText(text, 2, 0, width - 4, height, Justification::centredLeft, true);
            }
        }
        this->mainParent->getLockClients().unlock();
    }
    g.setColour(Colours::black.withAlpha (0.2f));
    g.fillRect(width - 1, 0, 1, height);
}

//==============================================================================
Component * JackClientListComponent::refreshComponentForCell(int rowNumber, int columnId, bool /*isRowSelected*/,
                                               Component *existingComponentToUpdate) {
    if (columnId == 1) {
        return existingComponentToUpdate;
    }

    if (columnId == 4) {
        TextButton *tbRemove = static_cast<TextButton*> (existingComponentToUpdate);
        if (tbRemove == nullptr) {
            tbRemove = new TextButton();
            tbRemove->setName(String(rowNumber));
            tbRemove->setBounds(4, 404, 88, 22);
            tbRemove->addListener(this);
            tbRemove->setColour(ToggleButton::textColourId, this->grisFeel->getFontColour());
            tbRemove->setLookAndFeel(this->grisFeel);
        }

        if (this->mainParent->getListClientjack().at(rowNumber).connected) {
            tbRemove->setButtonText("<->");
        } else {
            tbRemove->setButtonText("<X>");
        }

        return tbRemove;
    }

    ListIntOutComp *textLabel = static_cast<ListIntOutComp*> (existingComponentToUpdate);

    if (textLabel == nullptr)
        textLabel = new ListIntOutComp(*this);

    textLabel->setRowAndColumn(rowNumber, columnId);

    return textLabel;
}