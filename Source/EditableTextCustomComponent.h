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

#pragma once

#include <JuceHeader.h>

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
    //==============================================================================
    EditableTextCustomComponent(EditableTextCustomComponent const &) = delete;
    EditableTextCustomComponent(EditableTextCustomComponent &&) = delete;
    EditableTextCustomComponent & operator=(EditableTextCustomComponent const &) = delete;
    EditableTextCustomComponent & operator=(EditableTextCustomComponent &&) = delete;
    //==============================================================================
    void setRowAndColumn(int newRow, int newColumn);
    //==============================================================================
    // VIRTUALS
    void mouseDown(const juce::MouseEvent & event) override;
    void mouseDrag(const juce::MouseEvent & event) override;
    void textWasEdited() override;

private:
    //==============================================================================
    JUCE_LEAK_DETECTOR(EditableTextCustomComponent)
};
