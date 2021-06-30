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

//==============================================================================
class SpatButton final
    : public MinSizedComponent
    , private juce::TextButton::Listener
{
public:
    //==============================================================================
    class Listener
    {
    public:
        virtual ~Listener() = default;
        //==============================================================================
        virtual void buttonPressed(SpatButton * button) = 0;
    };

private:
    Listener & mListener;
    juce::TextButton mButton;
    int mWidth;
    int mHeight;

public:
    //==============================================================================
    SpatButton(juce::String const & text, juce::String const & tooltip, int width, int height, Listener & listener);
    ~SpatButton() override = default;
    //==============================================================================
    SpatButton(SpatButton const &) = delete;
    SpatButton(SpatButton &&) = delete;
    SpatButton & operator=(SpatButton const &) = delete;
    SpatButton & operator=(SpatButton &&) = delete;
    //==============================================================================
    [[nodiscard]] int getMinWidth() const noexcept override { return mWidth; }
    [[nodiscard]] int getMinHeight() const noexcept override { return mHeight; }
    void resized() override;

private:
    //==============================================================================
    void buttonClicked(juce::Button *) override;
    //==============================================================================
    JUCE_LEAK_DETECTOR(SpatButton)
};