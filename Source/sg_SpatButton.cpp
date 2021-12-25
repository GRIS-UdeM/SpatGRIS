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

#include "sg_SpatButton.hpp"

namespace gris
{
//==============================================================================
SpatButton::SpatButton(juce::String const & text,
                       juce::String const & tooltip,
                       int const width,
                       int const height,
                       Listener & listener)
    : mListener(listener)
    , mButton(text, tooltip)
    , mWidth(width)
    , mHeight(height)
{
    mButton.setColour(juce::TextButton::ColourIds::buttonColourId, juce::Colours::white.withAlpha(0.3f));
    mButton.addListener(this);
    addAndMakeVisible(mButton);
}

//==============================================================================
void SpatButton::resized()
{
    mButton.setBounds(0, 0, getWidth(), getHeight());
}

//==============================================================================
void SpatButton::buttonClicked(juce::Button *)
{
    mListener.buttonPressed(this);
}

} // namespace gris