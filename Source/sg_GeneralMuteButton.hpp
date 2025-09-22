/*
 This file is part of SpatGRIS.

 Developers: Gaël Lane Lépine, Samuel Béland, Olivier Bélanger, Nicolas Masson

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

namespace gris
{
class GrisLookAndFeel;

//==============================================================================
class GeneralMuteButton final
    : public MinSizedComponent
    , juce::Timer
{
public:
    enum class State { allUnmuted, allMuted };
    //==============================================================================
    class Listener
    {
    public:
        Listener() = default;
        virtual ~Listener() = default;
        SG_DEFAULT_COPY_AND_MOVE(Listener)
        //==============================================================================
        virtual void generalMuteButtonPressed() = 0;

    private:
        //==============================================================================
        JUCE_LEAK_DETECTOR(Listener)
    };

private:
    //==============================================================================
    GrisLookAndFeel & mLookAndFeel;
    Listener & mListener;
    State mState{ State::allUnmuted };
    bool mBlinkState{};
    juce::Rectangle<int> mActiveBounds{};

public:
    //==============================================================================
    explicit GeneralMuteButton(Listener & listener, GrisLookAndFeel & lookAndFeel);
    ~GeneralMuteButton() override = default;
    SG_DELETE_COPY_AND_MOVE(GeneralMuteButton)
    //==============================================================================
    void setState(State state);
    State getGeneralMuteButtonState();
    //==============================================================================
    void paint(juce::Graphics & g) override;
    void resized() override;
    void mouseUp(juce::MouseEvent const & event) override;
    void mouseMove(const juce::MouseEvent & event) override;
    void mouseExit(const juce::MouseEvent & event) override;
    [[nodiscard]] int getMinWidth() const noexcept override;
    [[nodiscard]] int getMinHeight() const noexcept override;

private:
    //==============================================================================
    void timerCallback() override;
    //==============================================================================
    JUCE_LEAK_DETECTOR(GeneralMuteButton)
};

} // namespace gris
