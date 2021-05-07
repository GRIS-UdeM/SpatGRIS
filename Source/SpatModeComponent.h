#pragma once

#include <JuceHeader.h>

#include "SpatMode.hpp"

//==============================================================================
class SpatModeComponent final
    : public juce::Component
    , private juce::TextButton::Listener
{
    static constexpr auto NUM_COLS = 2;
    static constexpr auto NUM_ROWS = 2;
    static constexpr auto SPACING_TO_REMOVE = 1;
    static constexpr auto TOTAL_HORIZONTAL_SPACING_TO_REMOVE = SPACING_TO_REMOVE * 2 * NUM_COLS;
    static constexpr auto TOTAL_VERTICAL_SPACING_TO_REMOVE = SPACING_TO_REMOVE * 2 * NUM_ROWS;

public:
    //==============================================================================
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
        virtual void handleSpatModeChanged(SpatMode spatMode) = 0;

    private:
        //==============================================================================
        JUCE_LEAK_DETECTOR(Listener)
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