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

#include "sg_AbstractSliceComponent.hpp"
#include "sg_LogicStrucs.hpp"
#include "sg_SmallToggleButton.hpp"

class GrisLookAndFeel;
class SmallGrisLookAndFeel;

//==============================================================================
class StereoSliceComponent final
    : public AbstractSliceComponent
    , private SmallToggleButton::Listener
{
    SmallToggleButton mIdButton;

public:
    //==============================================================================
    StereoSliceComponent(juce::String const & id,
                         GrisLookAndFeel & lookAndFeel,
                         SmallGrisLookAndFeel & smallLookAndFeel);
    ~StereoSliceComponent() override = default;
    SG_DELETE_COPY_AND_MOVE(StereoSliceComponent)
    //==============================================================================
    void muteSoloButtonClicked(SliceState) override { jassertfalse; }
    void smallButtonClicked(SmallToggleButton * /*button*/, bool /*state*/, bool /*isLeftMouseButton*/) override {}

private:
    //==============================================================================
    JUCE_LEAK_DETECTOR(StereoSliceComponent)
};