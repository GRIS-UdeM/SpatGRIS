/*
 This file is part of SpatGRIS.

 Developers: Gaël Lane Lépine

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

#include "sg_BinauralInfosComponent.hpp"
#include "sg_GrisLookAndFeel.hpp"

namespace gris
{
//==============================================================================
BinauralInfosComponent::BinauralInfosComponent(GrisLookAndFeel & lookAndFeel) : mLookAndFeel(lookAndFeel)
{
    addAndMakeVisible(mSpinningWheel);
    mSpinningWheel.setVisible(false);
}

//==============================================================================
void BinauralInfosComponent::resized()
{
    mSpinningWheel.setBounds(0, 2, 14, 14);
    mSpinningWheel.setColour(juce::ProgressBar::ColourIds::backgroundColourId, mLookAndFeel.getWinBackgroundColour());
    mSpinningWheel.setColour(juce::ProgressBar::ColourIds::foregroundColourId, mLookAndFeel.getOnColour());
    repaint();
}

//==============================================================================
void BinauralInfosComponent::paint(juce::Graphics & g)
{
    if (mTitle != "") {
        g.setColour(mLookAndFeel.getFontColour());
        g.drawText(mTitle, 16, 0, getWidth(), TITLE_HEIGHT + 2, juce::Justification::left);
        if (mShowSucceedCheck) {
            g.setColour(mLookAndFeel.getOnColour());
            g.drawText(juce::String::charToString(0x2713), 3, 0, getWidth(), TITLE_HEIGHT + 2, juce::Justification::left);
        }
    }
}

//==============================================================================
void BinauralInfosComponent::timerCallback()
{
}

//==============================================================================
void BinauralInfosComponent::setBinauralFileName(juce::String fileName)
{
    mTitle = fileName;
    repaint();
}

//==============================================================================
void BinauralInfosComponent::showSpinningWheel(bool showSpinningWheel)
{
    mSpinningWheel.setVisible(showSpinningWheel);
}

//==============================================================================
void BinauralInfosComponent::showCheckSign()
{
    mShowSucceedCheck = true;
    startTimer(2000);
    callAfterDelay(2000, [this]() {
        stopTimer();
        mShowSucceedCheck = false;
        repaint();
    });
}

} // namespace gris