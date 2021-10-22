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

#include "sg_MinSizedComponent.hpp"

class SmallGrisLookAndFeel;

//==============================================================================
class SmallToggleButton final
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
        Listener(Listener const &) = default;
        Listener(Listener &&) = default;
        Listener & operator=(Listener const &) = default;
        Listener & operator=(Listener &&) = default;
        //==============================================================================
        virtual void smallButtonClicked(SmallToggleButton * button, bool state) = 0;
    };

private:
    //==============================================================================
    Listener & mListener;
    SmallGrisLookAndFeel & mLookAndFeel;

    juce::Label mLabel;
    juce::TextButton mButton;

public:
    //==============================================================================
    SmallToggleButton(juce::String const & text, Listener & listener, SmallGrisLookAndFeel & lookAndFeel);
    ~SmallToggleButton() override = default;
    //==============================================================================
    SmallToggleButton(SmallToggleButton const &) = delete;
    SmallToggleButton(SmallToggleButton &&) = delete;
    SmallToggleButton & operator=(SmallToggleButton const &) = delete;
    SmallToggleButton & operator=(SmallToggleButton &&) = delete;
    //==============================================================================
    void setToggleState(bool state);
    //==============================================================================
    [[nodiscard]] int getMinWidth() const noexcept override;
    [[nodiscard]] int getMinHeight() const noexcept override;

private:
    //==============================================================================
    void buttonClicked(juce::Button * button) override;
    //==============================================================================
    JUCE_LEAK_DETECTOR(SmallToggleButton)
};