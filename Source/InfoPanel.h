#pragma once

#include "LayoutComponent.h"

class GrisLookAndFeel;

//==============================================================================
class InfoPanel final : public MinSizedComponent
{
    GrisLookAndFeel & mLookAndFeel;

    juce::Label mCpuLabel{};
    juce::Label mSampleRateLabel{};
    juce::Label mBufferSizeLabel{};
    juce::Label mNumInputsLabel{};
    juce::Label mNumOutputsLabel{};

public:
    //==============================================================================
    explicit InfoPanel(GrisLookAndFeel & lookAndFeel);
    ~InfoPanel() override = default;
    //==============================================================================
    InfoPanel(InfoPanel const &) = delete;
    InfoPanel(InfoPanel &&) = delete;
    InfoPanel & operator=(InfoPanel const &) = delete;
    InfoPanel & operator=(InfoPanel &&) = delete;
    //==============================================================================
    void setCpuLoad(double percentage);
    void setSampleRate(double sampleRate);
    void setBufferSize(int bufferSize);
    void setNumInputs(int numInputs);
    void setNumOutputs(int numOutputs);
    //==============================================================================
    void resized() override;
    [[nodiscard]] int getMinWidth() const noexcept override;
    [[nodiscard]] int getMinHeight() const noexcept override;

private:
    //==============================================================================
    [[nodiscard]] juce::Array<juce::Label *> getLabels() noexcept;
    //==============================================================================
    JUCE_LEAK_DETECTOR(InfoPanel)
};