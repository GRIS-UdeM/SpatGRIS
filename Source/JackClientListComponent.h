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

#pragma once

#include "macros.h"

DISABLE_WARNINGS
#include <JuceHeader.h>
ENABLE_WARNINGS

class Box;
class GrisLookAndFeel;
class MainContentComponent;

//==============================================================================
class JackClientListComponent final
    : public juce::Component
    , public juce::TableListBoxModel
    , public juce::ToggleButton::Listener
{
    struct ColumnIds {
        static constexpr int CLIENT_NAME = 1;
        static constexpr int START = 2;
        static constexpr int END = 3;
        static constexpr int ON_OFF_TOGGLE = 4;
    };
    //==============================================================================
    class ListIntOutComp final
        : public juce::Component
        , public juce::ComboBox::Listener
    {
    public:
        ListIntOutComp(JackClientListComponent & td);
        //==============================================================================
        void resized() final { comboBox.setBoundsInset(juce::BorderSize<int>(2)); }
        void setRowAndColumn(int newRow, int newColumn);
        void comboBoxChanged(juce::ComboBox const *) final { owner.setValue(row, columnId, comboBox.getSelectedId()); }

    private:
        //==============================================================================
        JackClientListComponent & owner;
        juce::ComboBox comboBox;

        int row;
        int columnId;
    };

public:
    JackClientListComponent(MainContentComponent * parent, GrisLookAndFeel * feel);
    ~JackClientListComponent() final = default;
    //==============================================================================
    void updateContentCli();
    void buttonClicked(juce::Button * button) final;
    void setBounds(int x, int y, int width, int height);
    void setValue(const int rowNumber, const int columnNumber, const int newRating);
    int getValue(const int rowNumber, const int columnNumber) const;
    int getNumRows() final { return numRows; }
    void paintRowBackground(juce::Graphics & g, int rowNumber, int /*width*/, int /*height*/, bool rowIsSelected) final;
    void
        paintCell(juce::Graphics & g, int rowNumber, int columnId, int width, int height, bool /*rowIsSelected*/) final;

    juce::String getText(const int columnNumber, const int rowNumber) const;
    juce::Component * refreshComponentForCell(int rowNumber,
                                              int columnId,
                                              bool /*isRowSelected*/,
                                              Component * existingComponentToUpdate) final;

private:
    //==============================================================================
    MainContentComponent * mainParent;
    GrisLookAndFeel * grisFeel;

    unsigned int numRows{};

    juce::TableListBox tableListClient;
    Box * box;
};
