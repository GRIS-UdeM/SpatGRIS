#pragma once

#include "MinSizedComponent.hpp"

//==============================================================================
class SpatSlider final
    : public MinSizedComponent
    , juce::Slider::Listener
{
public:
    class Listener
    {
    public:
        Listener() = default;
        virtual ~Listener() = default;
        //==============================================================================
        Listener(Listener const &) = delete;
        Listener(Listener &&) = delete;
        Listener & operator=(Listener const &) = delete;
        Listener & operator=(Listener &&) = delete;
        //==============================================================================
        virtual void sliderMoved(float value, SpatSlider * slider) = 0;

    private:
        //==============================================================================
        JUCE_LEAK_DETECTOR(Listener)
    };

private:
    //==============================================================================
    Listener & mListener;
    juce::Slider mSlider{};
    int mMinSize{};
    int mMaxSize{};

public:
    //==============================================================================
    SpatSlider(float minValue,
               float maxValue,
               float step,
               juce::String const & suffix,
               int minSize,
               int maxSize,
               Listener & listener);
    ~SpatSlider() override = default;
    //==============================================================================
    SpatSlider(SpatSlider const &) = delete;
    SpatSlider(SpatSlider &&) = delete;
    SpatSlider & operator=(SpatSlider const &) = delete;
    SpatSlider & operator=(SpatSlider &&) = delete;
    //==============================================================================
    void setValue(float value);
    //==============================================================================
    void resized() override;
    void sliderValueChanged(juce::Slider * slider) override;
    [[nodiscard]] int getMinWidth() const noexcept override;
    [[nodiscard]] int getMinHeight() const noexcept override;

private:
    //==============================================================================
    JUCE_LEAK_DETECTOR(SpatSlider)
};