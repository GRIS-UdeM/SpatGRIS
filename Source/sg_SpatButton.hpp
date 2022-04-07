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
//==============================================================================
class SpatButton final
    : public MinSizedComponent
    , juce::TextButton::Listener
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
    SG_DELETE_COPY_AND_MOVE(SpatButton)
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

} // namespace gris