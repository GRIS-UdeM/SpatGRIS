#pragma once

#include <JuceHeader.h>

class MinSizedComponent : public juce::Component
{
public:
    MinSizedComponent() = default;
    ~MinSizedComponent() override = default;
    //==============================================================================
    MinSizedComponent(MinSizedComponent const &) = delete;
    MinSizedComponent(MinSizedComponent &&) = delete;
    MinSizedComponent & operator=(MinSizedComponent const &) = delete;
    MinSizedComponent & operator=(MinSizedComponent &&) = delete;
    //==============================================================================
    [[nodiscard]] virtual int getMinWidth() const noexcept = 0;
    [[nodiscard]] virtual int getMinHeight() const noexcept = 0;

private:
    //==============================================================================
    JUCE_LEAK_DETECTOR(MinSizedComponent)
};