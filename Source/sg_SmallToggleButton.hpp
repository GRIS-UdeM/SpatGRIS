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
class SmallToggleButton final : public MinSizedComponent
{
public:
    //==============================================================================
    class Listener
    {
    public:
        Listener() = default;
        virtual ~Listener() = default;
        SG_DEFAULT_COPY_AND_MOVE(Listener)
        //==============================================================================
        virtual void smallButtonClicked(SmallToggleButton * button, bool state, bool isLeftMouseButton) = 0;
    };

private:
    //==============================================================================
    Listener & mListener;

    juce::Label mLabel;
    juce::TextButton mButton;

public:
    //==============================================================================
    SmallToggleButton(bool isToggle,
                      juce::String const & text,
                      juce::String const & toolTip,
                      Listener & listener,
                      SmallGrisLookAndFeel & lookAndFeel);
    ~SmallToggleButton() override = default;
    SG_DELETE_COPY_AND_MOVE(SmallToggleButton)
    //==============================================================================
    [[nodiscard]] juce::Colour getButtonColor() const;
    void setToggleState(bool state);
    void setButtonColor(int colorId, juce::Colour colour);
    void setLabelColour(int colorId, juce::Colour colour);
    //==============================================================================
    [[nodiscard]] int getMinWidth() const noexcept override;
    [[nodiscard]] int getMinHeight() const noexcept override;
    void resized() override;

private:
    //==============================================================================
    void mouseUp(const juce::MouseEvent & event) override;
    //==============================================================================
    JUCE_LEAK_DETECTOR(SmallToggleButton)
};