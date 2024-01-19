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

#include "sg_SourceIdButton.hpp"

namespace gris
{
//==============================================================================
SourceIdButton::SourceIdButton(source_index_t const sourceIndex,
                               juce::Colour const color,
                               Listener & listener,
                               SmallGrisLookAndFeel & lookAndFeel)
    : mListener(listener)

    , mButton(false, juce::String{ sourceIndex.get() }, "Change color", *this, lookAndFeel)
    , mSourceIndexCallBox{ nullptr }
{
    JUCE_ASSERT_MESSAGE_THREAD;

    setColor(color);
    addAndMakeVisible(mButton);
}

//==============================================================================
void SourceIdButton::setColor(juce::Colour const & color)
{
    JUCE_ASSERT_MESSAGE_THREAD;

    mButton.setButtonColor(juce::TextButton::buttonColourId, color);
    mButton.setLabelColour(juce::Label::textColourId, color.contrasting(1.0f));
}

//==============================================================================
void SourceIdButton::resized()
{
    mButton.setBounds(getLocalBounds());
}

//==============================================================================
void SourceIdButton::changeListenerCallback(juce::ChangeBroadcaster * source)
{
    JUCE_ASSERT_MESSAGE_THREAD;

    auto * colorSelector{ dynamic_cast<juce::ColourSelector *>(source) };
    jassert(colorSelector);
    if (colorSelector == nullptr) {
        jassertfalse;
        return;
    }

    mListener.sourceIdButtonColorChanged(this, colorSelector->getCurrentColour());
}

//==============================================================================
void SourceIdButton::smallButtonClicked([[maybe_unused]] SmallToggleButton * button,
                                        bool /*state*/,
                                        bool const isLeftMouseButton)
{
    JUCE_ASSERT_MESSAGE_THREAD;
    jassert(button == &mButton);

    if (!isLeftMouseButton) {
        mListener.sourceIdButtonCopyColorToNextSource(this, mButton.getButtonColor());
        return;
    }

    auto const mods{ juce::ModifierKeys::getCurrentModifiers() };
    if (isLeftMouseButton && mods.isAltDown()) {
        auto sourceIndexTxtEditor{ std::make_unique<juce::TextEditor>() };

        sourceIndexTxtEditor->setJustification(juce::Justification::centred);
        sourceIndexTxtEditor->setInputRestrictions(3, "1234567890");
        sourceIndexTxtEditor->addListener(this);
        sourceIndexTxtEditor->setSize(30, 20);

        mSourceIndexCallBox
            = &juce::CallOutBox::launchAsynchronously(std::move(sourceIndexTxtEditor), getScreenBounds(), nullptr);
        return;
    }

    auto colourSelector{ std::make_unique<juce::ColourSelector>(juce::ColourSelector::showColourAtTop
                                                                    | juce::ColourSelector::showSliders
                                                                    | juce::ColourSelector::showColourspace,
                                                                4,
                                                                4) };
    colourSelector->setName("background");
    colourSelector->setCurrentColour(mButton.getButtonColor());
    colourSelector->addChangeListener(this);
    colourSelector->setColour(juce::ColourSelector::backgroundColourId, juce::Colours::transparentBlack);
    colourSelector->setSize(300, 400);

    juce::CallOutBox::launchAsynchronously(std::move(colourSelector), getScreenBounds(), nullptr);
}

//==============================================================================
void SourceIdButton::textEditorReturnKeyPressed(juce::TextEditor & textEditor)
{
    JUCE_ASSERT_MESSAGE_THREAD;

    auto const srcIdx = textEditor.getText().getIntValue();

    if (srcIdx < 1 || srcIdx > MAX_NUM_SOURCES) {
        return;
    }

    mSourceIndexCallBox->dismiss();
    mSourceIndexCallBox = nullptr;

    mListener.sourceIdButtonSourceIndexChanged(this, static_cast<source_index_t>(srcIdx));
}

//==============================================================================
void SourceIdButton::textEditorEscapeKeyPressed(juce::TextEditor & textEditor)
{
    JUCE_ASSERT_MESSAGE_THREAD;

    mSourceIndexCallBox->dismiss();
    mSourceIndexCallBox = nullptr;
}

//==============================================================================
void SourceIdButton::textEditorFocusLost(juce::TextEditor & textEditor)
{
    JUCE_ASSERT_MESSAGE_THREAD;

    mSourceIndexCallBox->dismiss();
    mSourceIndexCallBox = nullptr;
}

} // namespace gris
