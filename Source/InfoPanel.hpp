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

#pragma once

#include "LayoutComponent.hpp"

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