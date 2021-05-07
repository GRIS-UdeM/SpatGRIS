#pragma once

#include <JuceHeader.h>

#include "SpatMode.hpp"

//==============================================================================
class SpatModeComponent final
    : public juce::Component
    , private juce::TextButton::Listener
{
public:
    //==============================================================================
    class Listener
    {
    public:
        virtual ~Listener() = default;
        //==============================================================================
        virtual void handleSpatModeChanged(SpatMode spatMode) = 0;
    };

private:
    //==============================================================================
    SpatMode mSpatMode{};
    Listener & mListener;
    juce::OwnedArray<juce::Button> mButtons{};

public:
    //==============================================================================
    explicit SpatModeComponent(Listener & listener);
    ~SpatModeComponent() override = default;
    //==============================================================================
    SpatModeComponent(SpatModeComponent const &) = delete;
    SpatModeComponent(SpatModeComponent &&) = delete;
    SpatModeComponent & operator=(SpatModeComponent const &) = delete;
    SpatModeComponent & operator=(SpatModeComponent &&) = delete;
    //==============================================================================
    void setSpatMode(SpatMode spatMode);
    //==============================================================================
    void buttonClicked(juce::Button * button) override;
    void resized() override;

private:
    //==============================================================================
    JUCE_LEAK_DETECTOR(SpatModeComponent)
};