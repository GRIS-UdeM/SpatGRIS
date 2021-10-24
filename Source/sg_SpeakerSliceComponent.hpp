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
#include "sg_SpeakerIdButton.hpp"

class GrisLookAndFeel;
class SmallGrisLookAndFeel;

//==============================================================================
class SpeakerSliceComponent final
    : public AbstractSliceComponent
    , private SpeakerIdButton::Listener
{
public:
    //==============================================================================
    class Listener
    {
    public:
        Listener() = default;
        virtual ~Listener() = default;
        //==============================================================================
        Listener(Listener const &) = default;
        Listener(Listener &&) = default;
        Listener & operator=(Listener const &) = default;
        Listener & operator=(Listener &&) = default;
        //==============================================================================
        virtual void setSelectedSpeakers(juce::Array<output_patch_t> selection) = 0;
        virtual void setSpeakerState(output_patch_t outputPatch, PortState state) = 0;
    };

private:
    //==============================================================================
    Listener & mOwner;
    output_patch_t mOutputPatch{};

    SpeakerIdButton mIdButton;

public:
    //==============================================================================
    SpeakerSliceComponent(output_patch_t outputPatch,
                          Listener & owner,
                          GrisLookAndFeel & lookAndFeel,
                          SmallGrisLookAndFeel & smallLookAndFeel);
    ~SpeakerSliceComponent() override = default;
    //==============================================================================
    SpeakerSliceComponent(SpeakerSliceComponent const &) = delete;
    SpeakerSliceComponent(SpeakerSliceComponent &&) = delete;
    SpeakerSliceComponent & operator=(SpeakerSliceComponent const &) = delete;
    SpeakerSliceComponent & operator=(SpeakerSliceComponent &&) = delete;
    //==============================================================================
    void setSelected(bool value);
    //==============================================================================
    void speakerIdButtonClicked(SpeakerIdButton * button) override;
    void muteSoloButtonClicked(PortState state) override;

private:
    //==============================================================================
    JUCE_LEAK_DETECTOR(SpeakerSliceComponent)
}; // class LevelComponent