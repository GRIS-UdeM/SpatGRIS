/*
 This file is part of SpatGRIS2.

 Developers: Samuel Béland, Olivier Bélanger, Nicolas Masson

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

#include "Box.h"
#include "GrisLookAndFeel.h"
#include "MainComponent.h"

//==============================================================================
JackClientListComponent::JackClientListComponent(MainContentComponent * parent, GrisLookAndFeel * feel)
    : mMainParent(parent)
    , mLookAndFeel(feel)
{
    mTableListClient.setModel(this);

    mTableListClient.setColour(juce::ListBox::outlineColourId, this->mLookAndFeel->getWinBackgroundColour());
    mTableListClient.setColour(juce::ListBox::backgroundColourId, this->mLookAndFeel->getWinBackgroundColour());
    mTableListClient.setOutlineThickness(1);

    mTableListClient.getHeader()
        .addColumn("Client", ColumnIds::CLIENT_NAME, 120, 70, 120, juce::TableHeaderComponent::notSortable);
    mTableListClient.getHeader()
        .addColumn("Start", ColumnIds::START, 60, 35, 70, juce::TableHeaderComponent::notSortable);
    mTableListClient.getHeader().addColumn("End", ColumnIds::END, 60, 35, 70, juce::TableHeaderComponent::notSortable);
    mTableListClient.getHeader()
        .addColumn("On/Off", ColumnIds::ON_OFF_TOGGLE, 62, 35, 70, juce::TableHeaderComponent::notSortable);

    mTableListClient.setMultipleSelectionEnabled(false);

    mTableListClient.updateContent();

    this->addAndMakeVisible(mTableListClient);
}

//==============================================================================
void JackClientListComponent::buttonClicked(juce::Button * button)
{
    this->mMainParent->getClientsLock().lock();
    bool connectedCli = !this->mMainParent->getClients().at(button->getName().getIntValue()).connected;
    this->mMainParent->connectionClientJack(this->mMainParent->getClients().at(button->getName().getIntValue()).name,
                                            connectedCli);
    updateContentCli();
    this->mMainParent->getClientsLock().unlock();
}

//==============================================================================
void JackClientListComponent::setBounds(int x, int y, int width, int height)
{
    this->juce::Component::setBounds(x, y, width, height);
    mTableListClient.setSize(width, height);
}

//==============================================================================
void JackClientListComponent::updateContentCli()
{
    mNumRows = (unsigned int)this->mMainParent->getClients().size();
    mTableListClient.updateContent();
    mTableListClient.repaint();
}

//==============================================================================
void JackClientListComponent::setValue(const int rowNumber, const int columnNumber, const int newRating)
{
    this->mMainParent->getClientsLock().lock();
    if (this->mMainParent->getClients().size() > (unsigned int)rowNumber) {
        switch (columnNumber) {
        case ColumnIds::START:
            this->mMainParent->getClients().at(rowNumber).portStart = newRating;
            this->mMainParent->getClients().at(rowNumber).initialized = true;
            break;
        case ColumnIds::END:
            this->mMainParent->getClients().at(rowNumber).portEnd = newRating;
            this->mMainParent->getClients().at(rowNumber).initialized = true;
            break;
        }
    }
    bool connectedCli = this->mMainParent->getClients().at(rowNumber).connected;
    this->mMainParent->connectionClientJack(this->mMainParent->getClients().at(rowNumber).name, connectedCli);
    this->mMainParent->getClientsLock().unlock();
}

//==============================================================================
int JackClientListComponent::getValue(const int rowNumber, const int columnNumber) const
{
    if ((unsigned int)rowNumber < this->mMainParent->getClients().size()) {
        switch (columnNumber) {
        case ColumnIds::START:
            return this->mMainParent->getClients().at(rowNumber).portStart;
        case ColumnIds::END:
            return this->mMainParent->getClients().at(rowNumber).portEnd;
        }
    }
    return -1;
}

//==============================================================================
juce::String JackClientListComponent::getText(const int columnNumber, const int rowNumber) const
{
    juce::String text = "?";
    if ((unsigned int)rowNumber < this->mMainParent->getClients().size()) {
        if (columnNumber == ColumnIds::CLIENT_NAME) {
            text = juce::String(this->mMainParent->getClients().at(rowNumber).name);
        }
    }
    return text;
}

//==============================================================================
void JackClientListComponent::paintRowBackground(juce::Graphics & g,
                                                 int const rowNumber,
                                                 int /*width*/,
                                                 int /*height*/,
                                                 bool /*rowIsSelected*/)
{
    if (rowNumber % 2) {
        g.fillAll(this->mLookAndFeel->getBackgroundColour().withBrightness(0.6f));
    } else {
        g.fillAll(this->mLookAndFeel->getBackgroundColour().withBrightness(0.7f));
    }
}

//==============================================================================
void JackClientListComponent::paintCell(juce::Graphics & g,
                                        int const rowNumber,
                                        int const columnId,
                                        int const width,
                                        int const height,
                                        bool const /*rowIsSelected*/)
{
    if (this->mMainParent->getClientsLock().try_lock()) {
        auto const jackClientListSize{ this->mMainParent->getClients().size() };
        if (static_cast<size_t>(rowNumber) < jackClientListSize) {
            if (columnId == ColumnIds::CLIENT_NAME) {
                juce::String text = getText(columnId, rowNumber);
                g.setColour(juce::Colours::black);
                g.setFont(12.0f);
                g.drawText(text, 2, 0, width - 4, height, juce::Justification::centredLeft, true);
            }
        }
        this->mMainParent->getClientsLock().unlock();
    }
    g.setColour(juce::Colours::black.withAlpha(0.2f));
    g.fillRect(width - 1, 0, 1, height);
}

//==============================================================================
juce::Component * JackClientListComponent::refreshComponentForCell(int const rowNumber,
                                                                   int const columnId,
                                                                   bool const /*isRowSelected*/,
                                                                   juce::Component * existingComponentToUpdate)
{
    if (columnId == ColumnIds::CLIENT_NAME) {
        return existingComponentToUpdate;
    }

    if (columnId == ColumnIds::ON_OFF_TOGGLE) {
        juce::TextButton * tbRemove = static_cast<juce::TextButton *>(existingComponentToUpdate);
        if (tbRemove == nullptr) {
            // TODO : naked new
            tbRemove = new juce::TextButton();
            tbRemove->setName(juce::String(rowNumber));
            tbRemove->setBounds(4, 404, 88, 22);
            tbRemove->addListener(this);
            tbRemove->setColour(juce::ToggleButton::textColourId, this->mLookAndFeel->getFontColour());
            tbRemove->setLookAndFeel(this->mLookAndFeel);
        }

        if (this->mMainParent->getClients().at(rowNumber).connected) {
            tbRemove->setButtonText("<->");
        } else {
            tbRemove->setButtonText("<X>");
        }

        return tbRemove;
    } else {
        ListIntOutComp * textLabel = static_cast<ListIntOutComp *>(existingComponentToUpdate);

        if (textLabel == nullptr) {
            // TODO : naked new
            textLabel = new ListIntOutComp(*this);
        }

        textLabel->setRowAndColumn(rowNumber, columnId);

        return textLabel;
    }
}

//==============================================================================
void JackClientListComponent::ListIntOutComp::setRowAndColumn(int newRow, int newColumn)
{
    this->mRow = newRow;
    this->mColumnId = newColumn;
    this->mComboBox.setSelectedId(this->mOwner.getValue(this->mRow, this->mColumnId), juce::dontSendNotification);
}

//==============================================================================
JackClientListComponent::ListIntOutComp::ListIntOutComp(JackClientListComponent & td) : mOwner(td)
{
    // Just put a combo box inside this component.
    this->addAndMakeVisible(mComboBox);
    for (int i = 1; i <= 256; ++i) {
        mComboBox.addItem(juce::String(i), i);
    }
    mComboBox.addListener(this);
    mComboBox.setWantsKeyboardFocus(false);
}