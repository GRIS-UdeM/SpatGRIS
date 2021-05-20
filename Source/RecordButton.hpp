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
class RecordButton final
    : public MinSizedComponent
    , private juce::Timer
{
public:
    enum class State { ready, recording };
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
        virtual void recordButtonPressed() = 0;

    private:
        //==============================================================================
        JUCE_LEAK_DETECTOR(Listener)
    };

private:
    //==============================================================================
    Listener & mListener;
    State mState{ State::ready };
    bool mBlinkState{};
    bool mIsButtonDown{};
    juce::Rectangle<int> mActiveBounds{};
    juce::Label mRecordedTime{};
    juce::int64 mTimeRecordingStarted{};

public:
    //==============================================================================
    explicit RecordButton(Listener & listener, GrisLookAndFeel & lookAndFeel);
    ~RecordButton() override = default;
    //==============================================================================
    RecordButton(RecordButton const &) = delete;
    RecordButton(RecordButton &&) = delete;
    RecordButton & operator=(RecordButton const &) = delete;
    RecordButton & operator=(RecordButton &&) = delete;
    //==============================================================================
    void setState(State state);
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
    void updateRecordedTime();
    //==============================================================================
    void timerCallback() override;
    //==============================================================================
    JUCE_LEAK_DETECTOR(RecordButton)
};