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

#ifndef EDITABLETEXTCUSTOMCOMPONENT_H
#define EDITABLETEXTCUSTOMCOMPONENT_H

#include "../JuceLibraryCode/JuceHeader.h"

class EditSpeakersWindow;

//==============================================================================
class EditableTextCustomComponent final : public juce::Label
{
public:
    //==============================================================================
    // DEFAULTS
    EditableTextCustomComponent(EditSpeakersWindow &editSpeakersWindow);
    ~EditableTextCustomComponent() final = default;
    //==============================================================================
    void setRowAndColumn(const int newRow, const int newColumn);
    //==============================================================================
    // VIRTUALS
    void mouseDown(const juce::MouseEvent &event) final;
    void mouseDrag(const juce::MouseEvent &event) final;
    void textWasEdited() final;
private:
    //==============================================================================
    EditSpeakersWindow &owner;
    
    int row;
    int columnId;
    int lastOffset;
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EditableTextCustomComponent);
};

#endif // EDITABLETEXTCUSTOMCOMPONENT_H