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

#pragma once

#include "AlgoGRIS/Data/sg_Macros.hpp"
#include "sg_EditSpeakersWindow.hpp"

#include <JuceHeader.h>
#if USE_OLD_SPEAKER_SETUP_VIEW
namespace gris
{
class EditSpeakersWindow;

//==============================================================================
class EditableTextCustomComponent final : public juce::Label
{
    EditSpeakersWindow & mOwner;
    int mRow{};
    int mColumnId{};
    int mLastOffset;

public:
    //==============================================================================
    // DEFAULTS
    explicit EditableTextCustomComponent(EditSpeakersWindow & editSpeakersWindow);
    ~EditableTextCustomComponent() override = default;
    SG_DELETE_COPY_AND_MOVE(EditableTextCustomComponent)
    //==============================================================================
    void setRowAndColumn(int newRow, int newColumn);
    void setColor(bool isEditable);
    //==============================================================================
    // VIRTUALS
    void mouseDown(const juce::MouseEvent & event) override;
    void mouseDrag(const juce::MouseEvent & event) override;
    void textWasEdited() override;

private:
    //==============================================================================
    JUCE_LEAK_DETECTOR(EditableTextCustomComponent)
};

} // namespace gris
#endif
