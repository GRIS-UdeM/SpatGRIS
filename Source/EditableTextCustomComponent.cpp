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

    // auto const isXyzEditable{  };

    using col = EditSpeakersWindow::Cols;

    switch (mColumnId) {
    case col::DRAG_HANDLE:
    case col::OUTPUT_PATCH:
    case col::DIRECT_TOGGLE:
    case col::DELETE_BUTTON:
        return;
    case col::X:
    case col::Y:
    case col::Z:
        if (mOwner.getModeSelected() != SpatMode::lbap) {
            return;
        }
        break;
    case col::AZIMUTH:
    case col::ELEVATION:
    case col::DISTANCE:
    case col::GAIN:
    case col::HIGHPASS:
        break;
    default:
        jassertfalse;
    }

    std::optional<float> increment{};
    switch (mColumnId) {
    case col::X:
    case col::Y:
    case col::Z:
    case col::DISTANCE:
        increment = 0.01f;
        break;
    case col::GAIN:
        increment = 0.1f;
        break;
    case col::AZIMUTH:
    case col::ELEVATION:
    case col::HIGHPASS:
        increment = 1.0f;
        break;
    }

    auto const offset{ event.getDistanceFromDragStartY() };
    if (increment) {
        auto const diff{ offset < mLastOffset ? *increment : *increment * -1.0f };
        auto const val{ getText().getFloatValue() + diff };
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
    if (newColumn == EditSpeakersWindow::Cols::DRAG_HANDLE) {
        setColour(ColourIds::outlineColourId, juce::Colours::black.withAlpha(0.2f));
        setBorderSize(juce::BorderSize<int>{ 1 });
    }
    setText(mOwner.getText(mColumnId, mRow), juce::dontSendNotification);
}