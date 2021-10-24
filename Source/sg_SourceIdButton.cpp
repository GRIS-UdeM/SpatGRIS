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

#include "sg_SourceIdButton.hpp"

//==============================================================================
SourceIdButton::SourceIdButton(source_index_t const sourceIndex,
                               juce::Colour const color,
                               Listener & listener,
                               SmallGrisLookAndFeel & lookAndFeel)
    : mListener(listener)
    , mLookAndFeel(lookAndFeel)
    , mButton(false, juce::String{ sourceIndex.get() }, "Change color", *this, lookAndFeel)
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
