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

#include "sg_EditableTextCustomComponent.hpp"

#include "sg_AudioProcessor.hpp"

#if USE_OLD_SPEAKER_SETUP_VIEW
namespace gris
{
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
#if USE_OLD_SPEAKER_SETUP_VIEW
    if (event.mods.isRightButtonDown()) {
        mOwner.mSpeakersTableListBox.deselectAllRows();
    } else {
        mOwner.mSpeakersTableListBox.selectRowsBasedOnModifierKeys(mRow, event.mods, false);
    }
#endif
    mLastOffset = 0;
    Label::mouseDown(event);
}

//==============================================================================
void EditableTextCustomComponent::mouseDrag(const juce::MouseEvent & event)
{
    if (event.mods.isShiftDown() || event.mods.isCtrlDown() || event.mods.isCommandDown()) {
        return;
    }

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
    case col::DISTANCE:
    case col::AZIMUTH:
    case col::ELEVATION:
    case col::GAIN:
    case col::HIGHPASS:
        break;
    default:
        jassertfalse;
    }

    tl::optional<float> increment{};
    switch (mColumnId) {
    case col::X:
    case col::Y:
    case col::Z:
    case col::DISTANCE:
        increment = 0.02f;
        break;
    case col::GAIN:
        increment = 0.1f;
        break;
    case col::AZIMUTH:
    case col::ELEVATION:
    case col::HIGHPASS:
        increment = 1.0f;
        break;
    default:
        jassertfalse;
    }

    auto const offset{ event.getDistanceFromDragStartY() };
    if (increment) {
#if USE_OLD_SPEAKER_SETUP_VIEW
        auto const diff{ *increment * narrow<float>(mLastOffset - offset) / 2.5f };
        auto const val{ getText().getFloatValue() + diff };
        mOwner.setText(mColumnId, mRow, juce::String(val), event.mods.isAltDown());
#endif
    }
    mLastOffset = offset;
}

//==============================================================================
void EditableTextCustomComponent::textWasEdited()
{
#if USE_OLD_SPEAKER_SETUP_VIEW
    mOwner.setText(mColumnId, mRow, getText());
#endif
    mOwner.computeSpeakers();
}

//==============================================================================
void EditableTextCustomComponent::setRowAndColumn(const int newRow, const int newColumn)
{
    mRow = newRow;
    mColumnId = newColumn;
    if (newColumn == EditSpeakersWindow::Cols::DRAG_HANDLE) {
        setColour(outlineColourId, juce::Colours::black.withAlpha(0.2f));
        setBorderSize(juce::BorderSize<int>{ 1 });
    }
#if USE_OLD_SPEAKER_SETUP_VIEW
    setText(mOwner.getText(mColumnId, mRow), juce::dontSendNotification);
#endif
}

//==============================================================================
void EditableTextCustomComponent::setColor(bool isEditable)
{
    if (!isEditable) {
        setColour(textColourId, juce::Colours::darkgrey);
        return;
    }
    setColour(textColourId, juce::Colours::black);
}

} // namespace gris
#endif
