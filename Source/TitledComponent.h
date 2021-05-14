/*
  ==============================================================================

    TitledComponent.h
    Created: 13 May 2021 9:37:58pm
    Author:  samue

  ==============================================================================
*/

#pragma once

#include "MinSizedComponent.hpp"

class GrisLookAndFeel;

//==============================================================================
class TitledComponent final : public MinSizedComponent
{
    static constexpr auto TITLE_HEIGHT = 18;

    juce::String mTitle{};
    MinSizedComponent * mContentComponent;
    GrisLookAndFeel & mLookAndFeel;

public:
    //==============================================================================
    TitledComponent(juce::String title, MinSizedComponent * contentComponent, GrisLookAndFeel & lookAndFeel);
    ~TitledComponent() override = default;
    //==============================================================================
    TitledComponent(TitledComponent const &) = delete;
    TitledComponent(TitledComponent &&) = delete;
    TitledComponent & operator=(TitledComponent const &) = delete;
    TitledComponent & operator=(TitledComponent &&) = delete;
    //==============================================================================
    void resized() override;
    void paint(juce::Graphics & g) override;

    [[nodiscard]] int getMinWidth() const noexcept override { return mContentComponent->getMinWidth(); }
    [[nodiscard]] int getMinHeight() const noexcept override
    {
        return mContentComponent->getMinHeight() + TITLE_HEIGHT;
    }

private:
    //==============================================================================
    JUCE_LEAK_DETECTOR(TitledComponent)
};