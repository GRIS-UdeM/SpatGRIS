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
    class ListIntOutComp
        : public juce::Component
        , public juce::ComboBox::Listener
    {
        JackClientListComponent & mOwner;
        juce::ComboBox mComboBox;

        int mRow;
        int mColumnId;

    public:
        //==============================================================================
        explicit ListIntOutComp(JackClientListComponent & td);
        ~ListIntOutComp() override = default;

        ListIntOutComp(ListIntOutComp const &) = delete;
        ListIntOutComp(ListIntOutComp &&) = delete;

        ListIntOutComp & operator=(ListIntOutComp const &) = delete;
        ListIntOutComp & operator=(ListIntOutComp &&) = delete;
        //==============================================================================
        void resized() override { mComboBox.setBoundsInset(juce::BorderSize<int>(2)); }
        void setRowAndColumn(int newRow, int newColumn);

        void comboBoxChanged(juce::ComboBox * /*comboBoxThatHasChanged*/) override
        {
            mOwner.setValue(mRow, mColumnId, mComboBox.getSelectedId());
        }

    private:
        JUCE_LEAK_DETECTOR(ListIntOutComp)
    }; // class ListIntOutComp

    //==============================================================================
    MainContentComponent * mMainParent;
    GrisLookAndFeel * mLookAndFeel;

    unsigned int mNumRows{};

    juce::TableListBox mTableListClient;
    Box * mBox;

public:
    JackClientListComponent(MainContentComponent * parent, GrisLookAndFeel * feel);
    ~JackClientListComponent() override = default;
    //==============================================================================
    void updateContentCli();
    void buttonClicked(juce::Button * button) override;
    void setBounds(int x, int y, int width, int height); // TODO : this hides juce::Component::setBounds()
    void setValue(int rowNumber, int columnNumber, int newRating);
    [[nodiscard]] int getValue(int rowNumber, int columnNumber) const;
    [[nodiscard]] int getNumRows() override { return mNumRows; }
    void paintRowBackground(juce::Graphics & g,
                            int rowNumber,
                            int /*width*/,
                            int /*height*/,
                            bool rowIsSelected) override;
    void paintCell(juce::Graphics & g, int rowNumber, int columnId, int width, int height, bool /*rowIsSelected*/)
        override;

    [[nodiscard]] juce::String getText(int columnNumber, int rowNumber) const;
    [[nodiscard]] juce::Component * refreshComponentForCell(int rowNumber,
                                                            int columnId,
                                                            bool /*isRowSelected*/,
                                                            Component * existingComponentToUpdate) override;

private:
    JUCE_LEAK_DETECTOR(JackClientListComponent)
}; // JackClientListComponent
