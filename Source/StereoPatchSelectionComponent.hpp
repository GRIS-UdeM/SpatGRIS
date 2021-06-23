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

#include "AttenuationSettingsComponent.hpp"
#include "LogicStrucs.hpp"
#include "MinSizedComponent.hpp"
#include "SpatMode.hpp"

class GrisLookAndFeel;

//==============================================================================
class StereoPatchSelectionComponent final
    : public MinSizedComponent
    , private juce::ComboBox::Listener
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
        virtual void handleStereoRoutingChanged(StereoRouting const & routing) = 0;

    private:
        //==============================================================================
        JUCE_LEAK_DETECTOR(Listener)
    };

private:
    //==============================================================================
    Listener & mListener;

    juce::Label mLeftLabel{};
    juce::ComboBox mLeftCombobox{};

    juce::Label mRightLabel{};
    juce::ComboBox mRightCombobox{};

public:
    //==============================================================================
    explicit StereoPatchSelectionComponent(Listener & listener, GrisLookAndFeel & lookAndFeel);
    ~StereoPatchSelectionComponent() override = default;
    //==============================================================================
    StereoPatchSelectionComponent(StereoPatchSelectionComponent const &) = delete;
    StereoPatchSelectionComponent(StereoPatchSelectionComponent &&) = delete;
    StereoPatchSelectionComponent & operator=(StereoPatchSelectionComponent const &) = delete;
    StereoPatchSelectionComponent & operator=(StereoPatchSelectionComponent &&) = delete;
    //==============================================================================
    void setStereoRouting(StereoRouting const & routing);
    void updateSpeakers(SpeakersOrdering speakers, StereoRouting const & routing);
    //==============================================================================
    void comboBoxChanged(juce::ComboBox * comboBoxThatHasChanged) override;
    void resized() override;
    [[nodiscard]] int getMinWidth() const noexcept override;
    [[nodiscard]] int getMinHeight() const noexcept override;

private:
    //==============================================================================
    void updateEnabledItems();
    //==============================================================================
    JUCE_LEAK_DETECTOR(StereoPatchSelectionComponent)
};