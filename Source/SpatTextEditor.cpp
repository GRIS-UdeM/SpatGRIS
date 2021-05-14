#include "SpatTextEditor.h"

#include "GrisLookAndFeel.h"

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

    mLabel.setJustificationType(juce::Justification::centredRight);
    mLabel.setColour(juce::Label::ColourIds::textColourId, lookAndFeel.getFontColour());
    addAndMakeVisible(mLabel);

    mEditor.setTooltip(tooltip);
    mEditor.addListener(this);
    mEditor.setJustification(juce::Justification::centredLeft);
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
    mListener.textEditorChanged(editor.getText(), this);
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
