#include "SpatTextEditor.h"

//==============================================================================
SpatTextEditor::SpatTextEditor(juce::String const & tooltip, int const width, int const height, Listener & listener)
    : mListener(listener)
    , mWidth(width)
    , mHeight(height)
{
    JUCE_ASSERT_MESSAGE_THREAD;
    mEditor.setTooltip(tooltip);
    mEditor.addListener(this);
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

    auto const extraWidth{ std::max(getWidth() - mWidth, 0) };
    auto const extraHeight{ std::max(getHeight() - mHeight, 0) };

    mEditor.setBounds(extraWidth / 2, extraHeight / 2, mWidth, mHeight);
}
