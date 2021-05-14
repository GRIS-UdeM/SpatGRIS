#include "SpatSlider.h"

#include "GrisLookAndFeel.h"

static constexpr auto MIN_LABEL_WIDTH = 90;
static constexpr auto LABEL_HEIGHT = 20;
static constexpr auto ENTRY_BOX_HEIGHT = 20;
static constexpr auto ENTRY_BOX_WIDTH = 50;
static constexpr auto SLIDER_SIZE = 60;

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
    mLabel.setJustificationType(juce::Justification::centred);
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
    auto const height{ getHeight() };

    auto const labelY{ std::max(height - getMinHeight(), 0) / 2 };

    auto const sliderX{ std::max(width - SLIDER_SIZE, 0) / 2 };
    auto const sliderY{ labelY + LABEL_HEIGHT };

    mLabel.setBounds(0, labelY, width, LABEL_HEIGHT);
    mSlider.setBounds(sliderX, sliderY, SLIDER_SIZE, SLIDER_SIZE);
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
    return std::max(std::max(SLIDER_SIZE, ENTRY_BOX_WIDTH), MIN_LABEL_WIDTH);
}

//==============================================================================
int SpatSlider::getMinHeight() const noexcept
{
    JUCE_ASSERT_MESSAGE_THREAD;
    return SLIDER_SIZE + ENTRY_BOX_HEIGHT;
}