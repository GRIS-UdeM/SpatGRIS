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

#include "sg_SpatSlider.hpp"

#include "sg_GrisLookAndFeel.hpp"

namespace
{
constexpr auto LABEL_WIDTH = 78;
constexpr auto LABEL_HEIGHT = 18;
constexpr auto PADDING = 0;
constexpr auto ENTRY_BOX_WIDTH = 50;
constexpr auto ENTRY_BOX_HEIGHT = 20;
constexpr auto SLIDER_SIZE = 60;
} // namespace

namespace gris
{
//==============================================================================
SpatSlider::SpatSlider(float const minValue,
                       float const maxValue,
                       float const step,
                       juce::String const & suffix,
                       juce::String const & label,
                       juce::String const & tooltip,
                       Listener & listener,
                       GrisLookAndFeel & lookAndFeel)
    : mListener(listener)
    , mLabel("", label)
    , mSlider(juce::Slider::SliderStyle::Rotary, juce::Slider::TextEntryBoxPosition::TextBoxBelow)
{
    JUCE_ASSERT_MESSAGE_THREAD;

    mLabel.setColour(juce::Label::ColourIds::textColourId, lookAndFeel.getFontColour());
    mLabel.setJustificationType(juce::Justification::centredTop);
    mLabel.setInterceptsMouseClicks(false, false);

    addAndMakeVisible(mLabel);

    mSlider.setRange(minValue, maxValue, step);
    mSlider.setTextValueSuffix(suffix);
    mSlider.addListener(this);
    mSlider.setTooltip(tooltip);
    mSlider.setRotaryParameters(juce::MathConstants<float>::pi * 1.3f, juce::MathConstants<float>::pi * 2.7f, true);
    mSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, ENTRY_BOX_WIDTH, ENTRY_BOX_HEIGHT);
    mSlider.setLookAndFeel(&lookAndFeel);

    addAndMakeVisible(mSlider);
}

//==============================================================================
void SpatSlider::setValue(float const value)
{
    JUCE_ASSERT_MESSAGE_THREAD;

    mSlider.setValue(value, juce::dontSendNotification);
}

//==============================================================================
void SpatSlider::resized()
{
    JUCE_ASSERT_MESSAGE_THREAD;

    auto const width{ getWidth() };

    mLabel.setBounds(0, 0, width, LABEL_HEIGHT);

    auto const x{ std::max((width - SLIDER_SIZE) / 2, 0) };
    auto const y{ LABEL_HEIGHT + PADDING };

    mSlider.setBounds(x, y, SLIDER_SIZE, SLIDER_SIZE);
}

//==============================================================================
void SpatSlider::sliderValueChanged(juce::Slider * slider)
{
    JUCE_ASSERT_MESSAGE_THREAD;
    jassert(slider == &mSlider);
    mListener.sliderMoved(static_cast<float>(slider->getValue()), this);
}

//==============================================================================
int SpatSlider::getMinWidth() const noexcept
{
    JUCE_ASSERT_MESSAGE_THREAD;
    static constexpr auto MIN_WIDTH{ std::max(std::max(SLIDER_SIZE, ENTRY_BOX_WIDTH), LABEL_WIDTH) };
    return MIN_WIDTH;
}

//==============================================================================
int SpatSlider::getMinHeight() const noexcept
{
    JUCE_ASSERT_MESSAGE_THREAD;

    static constexpr auto CORRECTION = 5;

    return LABEL_HEIGHT + PADDING + std::max(SLIDER_SIZE, ENTRY_BOX_HEIGHT) - CORRECTION;
}

} // namespace gris
