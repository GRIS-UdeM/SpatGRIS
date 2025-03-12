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

#include "sg_SpatTextEditor.hpp"

#include "sg_GrisLookAndFeel.hpp"

namespace
{
constexpr auto LABEL_HEIGHT = 18;
constexpr auto PADDING = 0;
constexpr auto BOX_WIDTH = 43;
constexpr auto BOX_HEIGHT = 22;

} // namespace

namespace gris
{
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
    mEditor.setJustification(juce::Justification::centredTop);
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

    auto const width{ getWidth() };

    mLabel.setBounds(0, 0, width, LABEL_HEIGHT);

    auto const x{ std::max((width - BOX_WIDTH) / 2, 0) };
    auto const y{ LABEL_HEIGHT + PADDING };

    mEditor.setBounds(x, y, BOX_WIDTH, BOX_HEIGHT);
}

//==============================================================================
int SpatTextEditor::getMinWidth() const noexcept
{
    juce::GlyphArrangement ga;
    ga.addLineOfText(mLabel.getFont(), mLabel.getText(), 0, 0);

    return std::max(static_cast<int>(ga.getBoundingBox(0, -1, true).getWidth()), BOX_WIDTH);
}


//==============================================================================
int SpatTextEditor::getMinHeight() const noexcept
{
    return LABEL_HEIGHT + PADDING + BOX_HEIGHT;
}

} // namespace gris
