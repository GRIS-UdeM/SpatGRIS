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

#include "sg_OutputPatch.hpp"
#include "sg_SmallToggleButton.hpp"

class SmallGrisLookAndFeel;

//==============================================================================
class SpeakerIdButton final
    : public MinSizedComponent
    , private SmallToggleButton::Listener
{
public:
    //==============================================================================
    class Listener
    {
    public:
        Listener() = default;
        virtual ~Listener() = default;
        Listener(Listener const &) = delete;
        Listener(Listener &&) = delete;
        Listener & operator=(Listener const &) = delete;
        Listener & operator=(Listener &&) = delete;
        //==============================================================================
        virtual void speakerIdButtonClicked(SpeakerIdButton * button) = 0;
    };

private:
    //==============================================================================
    Listener & mListener;
    SmallGrisLookAndFeel & mLookAndFeel;
    SmallToggleButton mButton;

public:
    //==============================================================================
    SpeakerIdButton(output_patch_t outputPatch, Listener & listener, SmallGrisLookAndFeel & lookAndFeel);
    ~SpeakerIdButton() override = default;
    //==============================================================================
    SpeakerIdButton(SpeakerIdButton const &) = delete;
    SpeakerIdButton(SpeakerIdButton &&) = delete;
    SpeakerIdButton & operator=(SpeakerIdButton const &) = delete;
    SpeakerIdButton & operator=(SpeakerIdButton &&) = delete;
    //==============================================================================
    void setSelected(bool state);
    //==============================================================================
    [[nodiscard]] int getMinWidth() const noexcept override { return 0; }
    [[nodiscard]] int getMinHeight() const noexcept override { return 0; }
    void resized() override;

private:
    //==============================================================================
    void smallButtonClicked(SmallToggleButton * button, bool state, bool isLeftMouseButton) override;
    //==============================================================================
    JUCE_LEAK_DETECTOR(SpeakerIdButton)
};