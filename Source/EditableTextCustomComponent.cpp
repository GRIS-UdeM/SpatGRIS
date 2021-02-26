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

#include "EditableTextCustomComponent.h"

#include "AudioProcessor.h"
#include "EditSpeakersWindow.h"

//==============================================================================
EditableTextCustomComponent::EditableTextCustomComponent(EditSpeakersWindow & editSpeakersWindow)
    : mOwner(editSpeakersWindow)
{
    setEditable(false, true, false);
    setColour(textColourId, juce::Colours::black);
    mLastOffset = 0;
}

//==============================================================================
void EditableTextCustomComponent::mouseDown(const juce::MouseEvent & event)
{
    if (event.mods.isRightButtonDown()) {
        mOwner.mSpeakersTableListBox.deselectAllRows();
    } else {
        mOwner.mSpeakersTableListBox.selectRowsBasedOnModifierKeys(mRow, event.mods, false);
    }
    Label::mouseDown(event);
}

//==============================================================================
void EditableTextCustomComponent::mouseDrag(const juce::MouseEvent & event)
{
    if (event.mods.isShiftDown() || event.mods.isCtrlDown() || event.mods.isCommandDown()) {
        return;
    }

    if (mOwner.getModeSelected() == SpatMode::lbap || mOwner.getDirectOutForSpeakerRow(mRow)) {
        if (mColumnId < 2) {
            return;
        }
    } else {
        if (mColumnId < 5) {
            return;
        }
    }

    bool ok = false;
    int offset = event.getDistanceFromDragStartY();
    float val = getText().getFloatValue();
    switch (mColumnId) {
    case 2:
    case 3:
    case 4:
        if (offset < mLastOffset)
            val += 0.01f; // up
        if (offset > mLastOffset)
            val -= 0.01f; // down
        ok = true;
        break;
    case 5:
    case 6:
    case 10:
        if (offset < mLastOffset)
            val += 1.0f; // up
        if (offset > mLastOffset)
            val -= 1.0f; // down
        ok = true;
        break;
    case 7:
        if (offset < mLastOffset)
            val += 0.01f; // up
        if (offset > mLastOffset)
            val -= 0.01f; // down
        ok = true;
        break;
    case 9:
        if (offset < mLastOffset)
            val += 0.1f; // up
        if (offset > mLastOffset)
            val -= 0.1f; // down
        ok = true;
        break;
    }
    if (ok) {
        mOwner.setText(mColumnId, mRow, juce::String(val), event.mods.isAltDown());
    }
    mLastOffset = offset;
}

//==============================================================================
void EditableTextCustomComponent::textWasEdited()
{
    mOwner.setText(mColumnId, mRow, getText());
}

//==============================================================================
void EditableTextCustomComponent::setRowAndColumn(const int newRow, const int newColumn)
{
    mRow = newRow;
    mColumnId = newColumn;
    setText(mOwner.getText(mColumnId, mRow), juce::dontSendNotification);
}