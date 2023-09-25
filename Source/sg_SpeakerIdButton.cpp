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

#include "sg_SpeakerIdButton.hpp"

#include "sg_GrisLookAndFeel.hpp"

namespace gris
{
//==============================================================================
SpeakerIdButton::SpeakerIdButton(output_patch_t const outputPatch,
                                 Listener & listener,
                                 SmallGrisLookAndFeel & lookAndFeel)
    : mListener(listener)
    , mLookAndFeel(lookAndFeel)
    , mButton(false, juce::String{ outputPatch.get() }, "Select speaker", *this, lookAndFeel)
{
    addAndMakeVisible(mButton);
}

//==============================================================================
void SpeakerIdButton::setSelected(bool const state)
{
    if (state) {
        mButton.setButtonColor(juce::TextButton::textColourOnId, mLookAndFeel.getWinBackgroundColour());
        mButton.setButtonColor(juce::TextButton::textColourOffId, mLookAndFeel.getWinBackgroundColour());
        mButton.setButtonColor(juce::TextButton::buttonColourId, mLookAndFeel.getOnColour());
    } else {
        mButton.setButtonColor(juce::TextButton::textColourOnId, mLookAndFeel.getFontColour());
        mButton.setButtonColor(juce::TextButton::textColourOffId, mLookAndFeel.getFontColour());
        mButton.setButtonColor(juce::TextButton::buttonColourId, mLookAndFeel.getBackgroundColour());
    }
    // repaint();
}

//==============================================================================
void SpeakerIdButton::resized()
{
    mButton.setBounds(getLocalBounds());
}

//==============================================================================
void SpeakerIdButton::smallButtonClicked([[maybe_unused]] SmallToggleButton * button,
                                         bool /*state*/,
                                         bool /*isLeftMouseButton*/)
{
    mListener.speakerIdButtonClicked(this);
}

} // namespace gris
