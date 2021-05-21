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

#include "SpatTextEditor.hpp"

#include "GrisLookAndFeel.hpp"

static constexpr auto BOX_WIDTH = 43;
static constexpr auto BOX_HEIGHT = 22;

//==============================================================================
SpatTextEditor::SpatTextEditor(juce::String const & label,
                               juce::String const & tooltip,
                               Listener & listener,
                               GrisLookAndFeel & lookAndFeel)
    : mListener(listener)
    , mLabel("", label)
{
    JUCE_ASSERT_MESSAGE_THREAD;

    setWantsKeyboardFocus(true);

    mLabel.setJustificationType(juce::Justification::centredRight);
    mLabel.setColour(juce::Label::ColourIds::textColourId, lookAndFeel.getFontColour());
    addAndMakeVisible(mLabel);

    mEditor.setTooltip(tooltip);
    mEditor.addListener(this);
    mEditor.setJustification(juce::Justification::centredLeft);
    mEditor.setSelectAllWhenFocused(true);
    addAndMakeVisible(mEditor);
}

//==============================================================================
void SpatTextEditor::setText(juce::String const & text)
{
    JUCE_ASSERT_MESSAGE_THREAD;
    mEditor.setText(text, false);
}

//==============================================================================
void SpatTextEditor::textEditorFocusLost(juce::TextEditor & editor)
{
    JUCE_ASSERT_MESSAGE_THREAD;
    jassert(&editor == &mEditor);
    mListener.textEditorChanged(editor.getText(), this);
}

//==============================================================================
void SpatTextEditor::textEditorReturnKeyPressed(juce::TextEditor & /*editor*/)
{
    JUCE_ASSERT_MESSAGE_THREAD;
    grabKeyboardFocus();
}

//==============================================================================
void SpatTextEditor::resized()
{
    JUCE_ASSERT_MESSAGE_THREAD;

    auto const extraWidth{ std::max(getWidth() - getMinWidth(), 0) };
    auto const labelWidth{ std::max(getWidth() - BOX_WIDTH - extraWidth / 2, 0) };

    auto const yOffset{ std::max(getHeight() - getMinHeight(), 0) / 2 };

    mLabel.setBounds(0, yOffset, labelWidth, BOX_HEIGHT);
    mEditor.setBounds(labelWidth, yOffset, BOX_WIDTH, BOX_HEIGHT);
}

//==============================================================================
int SpatTextEditor::getMinWidth() const noexcept
{
    return mLabel.getFont().getStringWidth(mLabel.getText()) + BOX_WIDTH;
}

//==============================================================================
int SpatTextEditor::getMinHeight() const noexcept
{
    return BOX_HEIGHT;
}
