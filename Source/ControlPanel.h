#pragma once

#include "Box.h"
#include "LayoutComponent.h"
#include "RecordButton.h"
#include "SpatModeComponent.h"
#include "SpatSlider.h"
#include "SpatTextEditor.h"
#include "StrongTypes.hpp"
#include "TitledComponent.h"
#include "constants.hpp"

//==============================================================================
class ControlPanel final
    : public MinSizedComponent
    , public SpatModeComponent::Listener
    , public SpatSlider::Listener
    , public SpatTextEditor::Listener
    , public RecordButton::Listener
{
    static constexpr auto PADDING = 5;

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
        virtual void masterGainChanged(dbfs_t gain) = 0;
        virtual void interpolationChanged(float interpolation) = 0;
        virtual void spatModeChanged(SpatMode spatMode) = 0;
        virtual void numSourcesChanged(int numSources) = 0;
        virtual void recordButtonPressed() = 0;

    private:
        //==============================================================================
        JUCE_LEAK_DETECTOR(Listener)
    };

private:
    //==============================================================================
    Listener & mListener;
    GrisLookAndFeel & mLookAndFeel;
    LayoutComponent mLayout{ LayoutComponent::Orientation::horizontal, true, false, mLookAndFeel };
    TitledComponent mSection{ "Controls", &mLayout, mLookAndFeel };

    SpatSlider mMasterGainSlider{
        LEGAL_MASTER_GAIN_RANGE.getStart().get(), LEGAL_MASTER_GAIN_RANGE.getEnd().get(), 0.1f, " db", 100, 150, *this
    };
    SpatSlider mInterpolationSlider{ 0.0f, 1.0f, 0.01f, "", 100, 150, *this };
    SpatModeComponent mSpatModeComponent{ *this };
    SpatTextEditor mNumSourcesTextEditor{ "Number of available sources", 43, 22, *this };
    RecordButton mRecordButton{ *this };

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
    void setRecordButtonState(RecordButton::State state);
    //==============================================================================
    void resized() override;
    int getMinWidth() const noexcept override { return mLayout.getMinWidth(); }
    int getMinHeight() const noexcept override { return mLayout.getMinHeight(); }
    void handleSpatModeChanged(SpatMode spatMode) override;
    void sliderMoved(float value, SpatSlider * slider) override;
    void textEditorChanged(juce::String const & value, SpatTextEditor * editor) override;
    void recordButtonPressed() override;

private:
    //==============================================================================
    JUCE_LEAK_DETECTOR(ControlPanel)
};