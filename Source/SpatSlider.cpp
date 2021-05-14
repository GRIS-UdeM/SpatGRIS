#include "SpatSlider.h"

//==============================================================================
SpatSlider::SpatSlider(float const minValue,
                       float const maxValue,
                       float const step,
                       juce::String const & suffix,
                       int const minSize,
                       int const maxSize,
                       Listener & listener)
    : mListener(listener)
    , mSlider(juce::Slider::SliderStyle::RotaryHorizontalVerticalDrag, juce::Slider::TextEntryBoxPosition::TextBoxBelow)
    , mMinSize(minSize)
    , mMaxSize(maxSize)
{
    JUCE_ASSERT_MESSAGE_THREAD;

    mSlider.setRange(minValue, maxValue, step);
    mSlider.setTextValueSuffix(suffix);
    mSlider.addListener(this);
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

    auto const smallestSize{ std::min(getWidth(), getHeight()) };
    auto const size{ std::clamp(smallestSize, mMinSize, mMaxSize) };
    auto const x{ std::max(getWidth() - size, 0) / 2 };
    auto const y{ std::max(getHeight() - size, 0) / 2 };

    mSlider.setBounds(x, y, size, size);
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
    return mMinSize;
}

//==============================================================================
int SpatSlider::getMinHeight() const noexcept
{
    JUCE_ASSERT_MESSAGE_THREAD;
    return mMinSize;
}