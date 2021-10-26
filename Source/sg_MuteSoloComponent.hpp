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

#include "sg_LayoutComponent.hpp"
#include "sg_LogicStrucs.hpp"
#include "sg_SmallToggleButton.hpp"

class GrisLookAndFeel;
class SmallGrisLookAndFeel;

//==============================================================================
class MuteSoloComponent final
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
        Listener(Listener const &) = default;
        Listener(Listener &&) = default;
        Listener & operator=(Listener const &) = default;
        Listener & operator=(Listener &&) = default;
        //==============================================================================
        virtual void muteSoloButtonClicked(PortState state) = 0;
    };

private:
    //==============================================================================
    Listener & mListener;
    GrisLookAndFeel & mLookAndFeel;
    SmallGrisLookAndFeel & mSmallLookAndFeel;

    LayoutComponent mLayout{ LayoutComponent::Orientation::horizontal, false, false, mLookAndFeel };
    SmallToggleButton mMuteButton{ true, "m", "mute", *this, mSmallLookAndFeel };
    SmallToggleButton mSoloButton{ true, "s", "solo", *this, mSmallLookAndFeel };

public:
    //==============================================================================
    MuteSoloComponent(Listener & listener, GrisLookAndFeel & lookAndFeel, SmallGrisLookAndFeel & smallLookAndFeel);
    ~MuteSoloComponent() override = default;
    //==============================================================================
    MuteSoloComponent(MuteSoloComponent const &) = delete;
    MuteSoloComponent(MuteSoloComponent &&) = delete;
    MuteSoloComponent & operator=(MuteSoloComponent const &) = delete;
    MuteSoloComponent & operator=(MuteSoloComponent &&) = delete;
    //==============================================================================
    void setPortState(PortState state);
    //==============================================================================
    [[nodiscard]] int getMinWidth() const noexcept override;
    [[nodiscard]] int getMinHeight() const noexcept override;
    void resized() override;

private:
    //==============================================================================
    void smallButtonClicked(SmallToggleButton * button, bool state, bool isLeftMouseButton) override;
    //==============================================================================
    JUCE_LEAK_DETECTOR(MuteSoloComponent)
};