#include "InfoPanel.h"

#include "GrisLookAndFeel.h"

static constexpr auto MIN_WIDTH = 400;
static constexpr auto MIN_HEIGHT = 25;

static auto const COLOR_1 = juce::Colours::blue.withBrightness(0.3f).withSaturation(0.2f);
static auto const COLOR_2 = juce::Colours::blue.withBrightness(0.2f).withSaturation(0.2f);

//==============================================================================
InfoPanel::InfoPanel(GrisLookAndFeel & lookAndFeel) : mLookAndFeel(lookAndFeel)
{
    auto const primeLabel = [&](juce::Label & label, int const index) {
        label.setJustificationType(juce::Justification::centred);
        label.setColour(juce::Label::ColourIds::textColourId, lookAndFeel.getFontColour());
        label.setColour(juce::Label::ColourIds::backgroundColourId, index % 2 ? COLOR_1 : COLOR_2);
        addAndMakeVisible(label);
    };

    auto const labels{ getLabels() };

    for (int i{}; i < labels.size(); ++i) {
        primeLabel(*labels[i], i);
    }
}

//==============================================================================
void InfoPanel::setCpuLoad(double const percentage)
{
    JUCE_ASSERT_MESSAGE_THREAD;

    static juce::String const PREFIX{ "Cpu usage: " };
    static juce::String const SUFFIX{ " %" };

    auto const intValue{ narrow<int>(std::round(percentage)) };
    auto const newString{ PREFIX + juce::String{ intValue } + SUFFIX };
    mCpuLabel.setText(newString, juce::dontSendNotification);
}

//==============================================================================
void InfoPanel::setSampleRate(double const sampleRate)
{
    static juce::String const SUFFIX{ " Hz" };

    auto const intValue{ static_cast<int>(sampleRate) };
    auto const string{ juce::String{ intValue } + SUFFIX };
    mSampleRateLabel.setText(string, juce::dontSendNotification);
}

//==============================================================================
void InfoPanel::setBufferSize(int const bufferSize)
{
    static juce::String const SUFFIX{ " samples" };
    auto const string{ juce::String{ bufferSize } + SUFFIX };
    mBufferSizeLabel.setText(string, juce::dontSendNotification);
}

//==============================================================================
void InfoPanel::setNumInputs(int const numInputs)
{
    static juce::String const SUFFIX{ " inputs" };
    auto const string{ juce::String{ numInputs } + SUFFIX };
    mNumInputsLabel.setText(string, juce::dontSendNotification);
}

//==============================================================================
void InfoPanel::setNumOutputs(int const numOutputs)
{
    static juce::String const SUFFIX{ " outputs" };
    auto const string{ juce::String{ numOutputs } + SUFFIX };
    mNumOutputsLabel.setText(string, juce::dontSendNotification);
}

//==============================================================================
void InfoPanel::resized()
{
    auto const labels{ getLabels() };
    auto const availableWidth{ narrow<float>(getWidth()) };
    auto const labelWidthFloat{ availableWidth / narrow<float>(labels.size()) };
    auto const labelWidthInt{ narrow<int>(std::round(labelWidthFloat)) };
    auto const height{ getHeight() };

    float xOffset{};
    for (auto * label : labels) {
        auto const x{ narrow<int>(std::round(xOffset)) };
        label->setBounds(x, 0, labelWidthInt, height);
        xOffset += labelWidthFloat;
    }
}

//==============================================================================
int InfoPanel::getMinWidth() const noexcept
{
    return MIN_WIDTH;
}

//==============================================================================
int InfoPanel::getMinHeight() const noexcept
{
    return MIN_HEIGHT;
}

//==============================================================================
juce::Array<juce::Label *> InfoPanel::getLabels() noexcept
{
    return juce::Array<juce::Label *>{ &mCpuLabel,
                                       &mSampleRateLabel,
                                       &mBufferSizeLabel,
                                       &mNumInputsLabel,
                                       &mNumOutputsLabel };
}
