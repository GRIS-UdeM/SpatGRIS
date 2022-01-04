/*
 This file is part of SpatGRIS.

 Developers: Samuel Béland, Olivier Bélanger, Nicolas Masson

 SpatGRIS is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 SpatGRIS is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with SpatGRIS.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "sg_InfoPanel.hpp"

#include "sg_GrisLookAndFeel.hpp"
#include "sg_MainComponent.hpp"

namespace
{
constexpr auto MIN_WIDTH = 400;
constexpr auto MIN_HEIGHT = 25;
auto const COLOR_1 = juce::Colours::blue.withBrightness(0.3f).withSaturation(0.2f);
auto const COLOR_2 = juce::Colours::blue.withBrightness(0.2f).withSaturation(0.2f);

} // namespace

namespace gris
{
//==============================================================================
InfoPanel::InfoPanel(MainContentComponent & mainContentComponent, GrisLookAndFeel const & lookAndFeel)
    : mMainContentComponent(mainContentComponent)

{
    auto const primeLabel = [&](juce::Label & label) {
        label.setJustificationType(juce::Justification::centred);
        label.setColour(juce::Label::ColourIds::textColourId, lookAndFeel.getFontColour());
        label.addMouseListener(this, false);
        addAndMakeVisible(label);
    };

    auto const labels = getLabels();

    for (auto * label : labels) {
        primeLabel(*label);
    }

    setComponentsColors(labels);
}

//==============================================================================
void InfoPanel::setCpuLoad(double const percentage)
{
    JUCE_ASSERT_MESSAGE_THREAD;

    static juce::String const PREFIX{ "Cpu usage: " };
    static juce::String const SUFFIX{ " %" };

    auto const isPeaking{ percentage >= 100.0 };

    if (isPeaking != mCpuIsCurrentlyPeaking) {
        if (isPeaking) {
            mCpuPeaked = true;
        }
        mCpuIsCurrentlyPeaking = isPeaking;
        setComponentsColors(getLabels());
    }

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
    auto const labels = getLabels();
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
void InfoPanel::mouseDown(juce::MouseEvent const & event)
{
    if (event.eventComponent != &mCpuLabel) {
        mMainContentComponent.handleShowPreferences();
        return;
    }

    if (!mCpuPeaked) {
        return;
    }

    mCpuPeaked = false;
    setComponentsColors(getLabels());
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

//==============================================================================
void InfoPanel::setComponentsColors(juce::Array<juce::Label *> const & labels)
{
    int index{};
    for (auto * label : labels) {
        auto const & color{ index++ % 2 ? COLOR_1 : COLOR_2 };
        label->setColour(juce::Label::ColourIds::backgroundColourId, color);
        label->setColour(juce::Label::ColourIds::outlineColourId, color);
    }

    static auto const PEAK_COLOR{ juce::Colours::red };

    if (mCpuPeaked) {
        mCpuLabel.setColour(juce::Label::ColourIds::outlineColourId, PEAK_COLOR);
    }
    if (mCpuIsCurrentlyPeaking) {
        mCpuLabel.setColour(juce::Label::ColourIds::backgroundColourId, PEAK_COLOR);
    }
}

} // namespace gris
