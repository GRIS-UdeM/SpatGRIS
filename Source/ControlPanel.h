#pragma once

#include "Box.h"
#include "LayoutComponent.h"
#include "SpatModeComponent.h"
#include "StrongTypes.hpp"
#include "constants.hpp"

//==============================================================================
class ControlPanel final
    : public MinSizedComponent
    , public SpatModeComponent::Listener
    , public SpatSlider::Listener
{
    static constexpr auto PADDING = 5;

public:
    //==============================================================================
    class Listener
    {
        virtual void masterGainChanged(dbfs_t gain) = 0;
        virtual void interpolationChanged(float interpolation) = 0;
        virtual void spatModeChanged(SpatMode spatMode) = 0;
        virtual void numSourcesChanged(int numSources) = 0;
        virtual void recordButtonPressed(bool state) = 0;
    };

private:
    //==============================================================================
    Listener & mListener;
    GrisLookAndFeel & mLookAndFeel;
    LayoutComponent mLayout{ LayoutComponent::Orientation::horizontal, true, false, mLookAndFeel };
    MainUiSection mSection{ "Controls", &mLayout, mLookAndFeel };

    SpatSlider mMasterGainSlider{
        LEGAL_MASTER_GAIN_RANGE.getStart().get(), LEGAL_MASTER_GAIN_RANGE.getEnd().get(), 0.1f, " db", 50, 100, *this
    };
    SpatSlider mInterpolationSlider{ 0.0f, 1.0f, 0.01f, "", 50, 100, *this };
    SpatModeComponent mSpatModeComponent{ *this };
    juce::TextEditor mNumSourcesTextEditor{};
    juce::TextButton mRecordButton{ "", "Start recording to disk" };
    juce::Label mTimeRecordedLabel{};

public:
    //==============================================================================
    ControlPanel(Listener & listener, GrisLookAndFeel & lookAndFeel);
    ~ControlPanel() override = default;
    //==============================================================================
    ControlPanel(ControlPanel const &) = delete;
    ControlPanel(ControlPanel &&) = delete;
    ControlPanel & operator=(ControlPanel const &) = delete;
    ControlPanel & operator=(ControlPanel &&) = delete;
    //==============================================================================
    void setMasterGain(dbfs_t gain);
    void setInterpolation(float interpolation);
    void setSpatMode(SpatMode spatMode);
    void setNumSources(int numSources);
    void setRecordButtonState(bool state);
    //==============================================================================
    int getMinWidth() const noexcept override;
    int getMinHeight() const noexcept override;
    void resized() override;

private:
    //==============================================================================
    JUCE_LEAK_DETECTOR(ControlPanel)
};