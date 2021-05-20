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
#include "SpatMode.hpp"

class GrisLookAndFeel;

//==============================================================================
class SpatModeComponent final
    : public MinSizedComponent
    , private juce::TextButton::Listener
{
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
    juce::Label mLabel{};
    juce::OwnedArray<juce::Button> mButtons{};

public:
    //==============================================================================
    explicit SpatModeComponent(Listener & listener, GrisLookAndFeel & lookAndFeel);
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
    [[nodiscard]] int getMinWidth() const noexcept override;
    [[nodiscard]] int getMinHeight() const noexcept override;

private:
    //==============================================================================
    JUCE_LEAK_DETECTOR(SpatModeComponent)
};