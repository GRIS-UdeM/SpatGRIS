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

#include "MinSizedComponent.hpp"

class GrisLookAndFeel;

//==============================================================================
class SpatSlider final
    : public MinSizedComponent
    , juce::Slider::Listener
{
public:
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
        virtual void sliderMoved(float value, SpatSlider * slider) = 0;

    private:
        //==============================================================================
        JUCE_LEAK_DETECTOR(Listener)
    };

private:
    //==============================================================================
    Listener & mListener;
    juce::Label mLabel{};
    juce::Slider mSlider{};

public:
    //==============================================================================
    SpatSlider(float minValue,
               float maxValue,
               float step,
               juce::String const & suffix,
               juce::String const & label,
               juce::String const & tooltip,
               Listener & listener,
               GrisLookAndFeel & lookAndFeel);
    ~SpatSlider() override = default;
    //==============================================================================
    SpatSlider(SpatSlider const &) = delete;
    SpatSlider(SpatSlider &&) = delete;
    SpatSlider & operator=(SpatSlider const &) = delete;
    SpatSlider & operator=(SpatSlider &&) = delete;
    //==============================================================================
    void setValue(float value);
    //==============================================================================
    void resized() override;
    void sliderValueChanged(juce::Slider * slider) override;
    [[nodiscard]] int getMinWidth() const noexcept override;
    [[nodiscard]] int getMinHeight() const noexcept override;

private:
    //==============================================================================
    JUCE_LEAK_DETECTOR(SpatSlider)
};